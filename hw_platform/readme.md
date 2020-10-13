# EMG hardware platform
## EMG8x (Revision 1)
Первый вариант платы на основе 8-канального АЦП ADS1299. На данный момент собран один экземпляр платы. Цифровая часть полностью работает. По аналоговой части есть ряд ошибок значительно снижающий общие характеристики платы.

![Схема (KiCAD Eeschema)](https://drive.google.com/uc?export=view&id=1ILe61-I7x9Qac2lrjNZa3rqXznOtPNTw)

![Схема (KiCAD Eeschema)](https://drive.google.com/uc?export=view&id=1uYkZH1ljI7POEj6VSd-617ZXaz0Fbder)

## EMG8x (Revision 2)
* Исправлены ошибки, допущенные в Revision 1.
* Полностью дифференциальный интерфейс с раздельными входными фильтрами (как в [EEG Front-End Performance Demonstration Kit](https://www.ti.com/lit/ug/slau443b/slau443b.pdf?ts=1602258946892&ref_url=https%253A%252F%252Fwww.ti.com%252Ftool%252FADS1299EEGFE-PDK))
* Полностью дифференциальный интерфейс с общим фильтром (как в [Datasheet ADS1299](https://www.ti.com/lit/ds/symlink/ads1299.pdf?ts=1602417721400))
* Третий электрод теперь можно завести на GNDA, на сигнал смещения или на общий электрод.
* Интерфейс с одним общим электродом (меньше проводов).
* Питание от двух аккумуляторов Li-Ion 18650.
![Схема (KiCAD Eeschema)](https://drive.google.com/uc?export=view&id=18xvGgrcY3SdtwbyLzkZvpchL3RI9LBVP)
## Общедоступные платы других производителей EMG Shields
1. [Analog EMG Sensor by OYMotion SKU:SEN0240](https://www.dfrobot.com/wiki/index.php/Analog_EMG_Sensor_by_OYMotion_SKU:SEN0240).
2. [Olimex SHIELD-EKG-EMG](https://www.olimex.com/Products/Duino/Shields/SHIELD-EKG-EMG/open-source-hardware).
3. [Схема Grove - EMG Detector](https://static.chipdip.ru/lib/843/DOC003843068.pdf)
## Схемы EMG Shields 
1. [Схема Olimex SHIELD-EKG-EMG](https://www.olimex.com/Products/Duino/Shields/SHIELD-EKG-EMG/resources/SHIELD-EKG-EMG-REV-B-SCHEMATIC.pdf).
2. [Схема Grove - EMG Detector](https://static.chipdip.ru/lib/843/DOC003843068.pdf)

Примеры коммерческих модулей: [biometrics](http://www.biometricsltd.com/wireless-sensors.htm)

