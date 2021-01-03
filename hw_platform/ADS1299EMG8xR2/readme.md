# EMG hardware platform EMG8x Revision2

## Общие характеристики

| Field      | Value       |
| ------------- | ------------- |
| Название      | EMG8xR2       |
| Дата размещения| 08.11.2020   |
| Номер заказа в Резонит| N 1323360 |
| Статус заказа в Резонит| Отправлены в производство 10.11.2020 |
|Номер платы |TIM 820174|
| Размеры| 96x100 |
| Питание| USB или 2 LiIon батареи 18650 |
| АЦП | ADS1299 |
|Скорость сэмплирования|250SPS-16kSPS|
|Разрядность АЦП|24 бита|
| Кол-во каналов | 8 |
| Интерфейс| WiFi, BLE |
| Процессор| Espressif ESP32 |
| Критические ошибки| Нет информации|

## Конфигурация

Для сборки микропрограммного обеспечения необходимо установить ESP-IDF. Для настройки параметров прошивки используется команда 
```
idf.py menuconfig
```
В разделе WiFi Configuration следует указать режим. В режиме SoftAP платформа работает в режиме точки доступа. 

Внимание! Пароль в режиме SoftAP должен быть не менее 8 символов!!!

Если опция SoftAP выключена, то платформа работает в режиме станции. В этом случае SSID и пароль необходимо установить для доступа к вашей WiFi сети.

![WiFiConfig](https://drive.google.com/uc?export=view&id=1MJfj9EwQQmBjOCEw-MoydRnb6mCN9Bni)

В разделе EMG8x доступна опция ADS1299 Emulation, которая позволяет запустить процессор без платы АЦП. При этом вместо реальных данных подставляется тестовый периодический сигнал.

![WiFiConfig](https://drive.google.com/uc?export=view&id=1yztndch84vDITJ4aMiv7DsKqSIPqdQcS)

В режиме ADS1299 Emulation опция ADC Sampling frequency code управляет скоростью генерации тестовых данных. В любом режиме сокрость сэмплирования определяется по формуле SPS=16000/(2^freq_code) Гц. Например, если ADC Sampling frequency code = 5, то сигнал генерируется со скоростью 16000/(2^5)=500 отсчетов в секунду. 
Остальные параметры должны быть установлены одинаково для аппаратной платформы и для клиентского скрипта [emg8x_tcp_client.py](https://github.com/RF-Lab/emg_platform/blob/master/source/python/hwtools/emg8x_tcp_client.py)

## Схема

![Схема (KiCAD Eeschema)](https://drive.google.com/uc?export=view&id=1L_DrWvEDwm783Buy2m62GdEQSulPrG8b)

![Front gerber](https://drive.google.com/uc?export=view&id=1hN0poB4uc8N73exoMtfLxHJGHW3u5dZl)

![Back gerber](https://drive.google.com/uc?export=view&id=1BsafmD12kwoHVkJh0ndohjoeSAEMBt_H)

![Front 3d](https://drive.google.com/uc?export=view&id=1ftYCHDupvvZYecgCGAcaD7Ab4m_XAcu0)

![Back 3d](https://drive.google.com/uc?export=view&id=1MH8trmyGndIPEdMWTS9q-ZSTEHRyNE4C)


