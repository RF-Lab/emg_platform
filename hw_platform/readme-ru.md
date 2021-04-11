# Аппаратная платформа с открытым исходным кодом для захвата сигналов электромиографии (ЭМГ)

Платформа использует 8-канальный АЦП [ADS1299](https://www.ti.com/product/ADS1299) производства компании Texas Instruments. Для реализации интерфейсов BLE и WiFi используется двухядерный 
процессор ESP32 [Espressif](https://www.espressif.com/). Текущая версия платформы [Rev.4](https://github.com/RF-Lab/emg_platform/tree/master/hw_platform/ADS1299EMG8xR2) использует шесть 
аналоговых интерфейсов на основе инструментального усилителя INA331.

* Схема принципиальная. Лист 1.

![image](https://i.ibb.co/jMTmMg3/emg8x-schematic-page1.png)

* Схема принципиальная. Лист 2.

![image](https://i.ibb.co/vLCs59N/emg8x-schematic-page2.png)

* 3D визуализация PCB:

![Image](https://i.ibb.co/WxPzTH6/emg-8-6-x-R4.png)

## Другие примеры реализации интерфейсов для ЭМГ
1. [Analog EMG Sensor by OYMotion SKU:SEN0240](https://www.dfrobot.com/wiki/index.php/Analog_EMG_Sensor_by_OYMotion_SKU:SEN0240).
2. [Olimex SHIELD-EKG-EMG](https://www.olimex.com/Products/Duino/Shields/SHIELD-EKG-EMG/open-source-hardware).
3. [biometrics](http://www.biometricsltd.com/wireless-sensors.htm)
 
## Примеры принципиальных схем интерфейсов ЭМГ
1. [Схема Olimex SHIELD-EKG-EMG](https://www.olimex.com/Products/Duino/Shields/SHIELD-EKG-EMG/resources/SHIELD-EKG-EMG-REV-B-SCHEMATIC.pdf).
2. [Схема Grove - EMG Detector](https://static.chipdip.ru/lib/843/DOC003843068.pdf)



