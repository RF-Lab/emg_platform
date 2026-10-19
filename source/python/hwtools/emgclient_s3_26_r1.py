# pip install PyQt6 pyqtgraph numpy
import sys
import socket
import struct
import numpy as np
from PyQt6 import QtWidgets, QtCore
import pyqtgraph as pg

# === НАСТРОЙКИ СЕТИ И ПРОТОКОЛА ИЗ КОДА ПРОШИВКИ ===
# Замените IP на адрес вашей платы (точка доступа или выданный роутером)
ESP_IP = "192.168.1.141"  
ESP_PORT = 3000

# Параметры из app_main.c
HEADER_SIZE = 16          # CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE
SAMPLES_PER_BLOCK = 128    # CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK
NUM_CHANNELS = 8          # AD1299_NUM_CH
VALUES_PER_SAMPLE = NUM_CHANNELS + 1  # STAT + 8 каналов

# Общий размер одного ожидаемого TCP-пакета в байтах
# 16 байт заголовка + (64 сэмпла * 9 значений * 4 байта) = 16 + 2304 = 2320 байт
BLOCK_SIZE_BYTES = HEADER_SIZE + (SAMPLES_PER_BLOCK * VALUES_PER_SAMPLE * 4)

# Настройки отображения
DISPLAY_WINDOW_SIZE = 2000  # Сколько точек хранить в истории для каждого канала


class WifiEMGViewer(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("8-канальный ЭМГ Визуализатор (Wi-Fi/TCP)")
        self.resize(1000, 700)

        # Буфер данных: 8 каналов по DISPLAY_WINDOW_SIZE точек
        self.emg_data = np.zeros((NUM_CHANNELS, DISPLAY_WINDOW_SIZE))

        # Настройка графического окна pyqtgraph
        self.graph_widget = pg.PlotWidget()
        self.setCentralWidget(self.graph_widget)
        self.graph_widget.setBackground('k')
        self.graph_widget.showGrid(x=True, y=True)
        self.graph_widget.setLabel('bottom', 'Отсчеты')
        self.graph_widget.setLabel('left', 'Амплитуда')

        # Создаем линии для каждого канала с разным смещением (чтобы графики не сливались)
        self.lines = []
        self.offsets = np.arange(NUM_CHANNELS) * 500000  # Шаг смещения между каналами
        
        # Генерация цветов для каналов
        colors = [
            (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0),
            (255, 0, 255), (0, 255, 255), (255, 128, 0), (128, 0, 255)
        ]
        
        for i in range(NUM_CHANNELS):
            pen = pg.mkPen(color=colors[i], width=1)
            # Инициализируем линию со смещением
            line = self.graph_widget.plot(self.emg_data[i] + self.offsets[i], pen=pen)
            self.lines.append(line)

        # Настройка TCP Сокета
        self.sock = None
        self.tcp_buffer = bytearray()
        self.connect_to_esp()

        # Таймер опроса сокета и обновления интерфейса (~60 FPS)
        self.timer = QtCore.QTimer()
        self.timer.setInterval(16)
        self.timer.timeout.connect(self.main_loop)
        self.timer.start()

    def connect_to_esp(self):
        """Установка неблокирующего TCP-соединения с платой"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((ESP_IP, ESP_PORT))
            self.sock.setblocking(False)  # Делаем сокет неблокирующим для GUI
            print(f"Успешное подключение к ЭМГ плате ({ESP_IP}:{ESP_PORT})")
        except Exception as e:
            print(f"Не удалось подключиться к {ESP_IP}:{ESP_PORT}. Ошибка: {e}")
            print("Убедитесь, что вы подключены к Wi-Fi сети платы.")
            self.sock = None

    def main_loop(self):
        """Чтение данных из сети и перерисовка графиков"""
        if not self.sock:
            return

        # Читаем все доступные данные из сетевого буфера ОС
        try:
            while True:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                self.tcp_buffer.extend(chunk)
        except BlockingIOError:
            # Данные в буфере ОС закончились — переходим к обработке накопленного
            pass
        except Exception as e:
            print(f"Ошибка сети: {e}")
            self.sock = None
            return

        # Разбираем пакеты, если накопилось достаточно байт для целого блока
        updated = False
        while len(self.tcp_buffer) >= BLOCK_SIZE_BYTES:
            # Извлекаем один блок данных
            block = self.tcp_buffer[:BLOCK_SIZE_BYTES]
            del self.tcp_buffer[:BLOCK_SIZE_BYTES]

            # Валидация заголовка (проверяем сигнатуру "EMG8x")
            if block[0:5] != b"EMG8x":
                # Если синхронизация нарушена, ищем следующий заголовок
                idx = block.find(b"EMG8x", 1)
                if idx != -1:
                    del self.tcp_buffer[:idx]
                else:
                    self.tcp_buffer.clear()
                continue

            # Парсим счетчик пакетов (находится по смещению 8 байт, uint32)
            pkt_count = struct.unpack("<I", block[8:12])[0]
            
            # Полезная нагрузка начинается после 16 байт заголовка
            payload = block[HEADER_SIZE:]
            
            # Количество 32-битных чисел во всем блоке данных
            num_elements = SAMPLES_PER_BLOCK * VALUES_PER_SAMPLE
            # Распаковываем бинарные int32 (формат 'i')
            raw_values = struct.unpack(f"<{num_elements}i", payload)
            
            # Превращаем в матрицу (64 строки, 9 колонок)
            matrix = np.array(raw_values).reshape((SAMPLES_PER_BLOCK, VALUES_PER_SAMPLE))
            
            # Выделяем только каналы ЭМГ (пропуская колонку 0, которая является STAT)
            channels_data = matrix[:, 1:].T  # Результат: форма (8, 64)

            # Добавляем новые 64 сэмпла в кольцевой буфер отображения
            self.emg_data = np.roll(self.emg_data, -SAMPLES_PER_BLOCK, axis=1)
            self.emg_data[:, -SAMPLES_PER_BLOCK:] = channels_data
            updated = True

        # Если данные обновились, перерисовываем графики
        if updated:
            for i in range(NUM_CHANNELS):
                # Отрисовка данных канала со смещением по оси Y
                self.lines[i].setData(self.emg_data[i] + self.offsets[i])

    def closeEvent(self, event):
        """Закрытие сокета при выходе"""
        if self.sock:
            self.sock.close()
        event.accept()


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    viewer = WifiEMGViewer()
    viewer.show()
    sys.exit(app.exec())
