# Baseline code for ADS1299 initialization

### Hardware Required

This example can be executed on EMG8x Board.

### Configure the project

Для сборки микропрограммного обеспечения необходимо установить ESP-IDF [Инструкция](http://rf-lab.org/news/2020/04/04/esp-idf.html). Для настройки параметров прошивки используется команда 
```
idf.py menuconfig
```
В разделе WiFi Configuration следует указать режим. В режиме SoftAP платформа работает в режиме точки доступа. 

Внимание! Пароль в режиме SoftAP должен быть не менее 8 символов!!!

Если опция SoftAP выключена, то платформа работает в режиме станции. В этом случае SSID и пароль необходимо установить для доступа к вашей WiFi сети.

![WiFiConfig](https://drive.google.com/uc?export=view&id=1MJfj9EwQQmBjOCEw-MoydRnb6mCN9Bni)

В разделе EMG8x доступна опция ADS1299 Emulation, которая позволяет запустить процессор без платы АЦП. При этом вместо реальных данных подставляется тестовый периодический сигнал.

![WiFiConfig](https://drive.google.com/uc?export=view&id=1yztndch84vDITJ4aMiv7DsKqSIPqdQcS)

В режиме ADS1299 Emulation опция ADC Sampling frequency code управляет скоростью генерации тестовых данных. В любом режиме скорость сэмплирования определяется по формуле SPS=16000/(2^freq_code) Гц. Например, если ADC Sampling frequency code = 5, то сигнал генерируется со скоростью 16000/(2^5)=500 отсчетов в секунду. 
Остальные параметры должны быть установлены одинаково для аппаратной платформы и для клиентского скрипта [emg8x_tcp_client.py](https://github.com/RF-Lab/emg_platform/blob/master/source/python/hwtools/emg8x_tcp_client.py)

Также следует проверить слудующие настройки, и при необходимости установить их в соответсвии с примерами ниже:

![ClockConfig](https://drive.google.com/uc?export=view&id=1eZYpX7af0FeQneOZydpKUNTEvS1AI8eY)

![ClockConfig](https://drive.google.com/uc?export=view&id=1fnWqT9L5hfGXn0zA2qqtULJ1NKOukP4W)

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

