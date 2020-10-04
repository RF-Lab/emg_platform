# Тестовый эксперимент по сбору EMG сигналов для проверки EMG8x

## Настройки АЦП

```
// Set internal reference
ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG3, 0xE8 ) ;
    
//Set device for DR=fmod/4096
// Enable clk output
ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG1, 0xf6 ) ;     // Default 0x96 (see power up sequence)

// Configure test signal parameters
ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG2, 0xb5 ) ;

// Configure channels
ad1299_wreg( spi_dev, AD1299_ADDR_CH1SET, 0x05 ) ;      // CH1: Test signal,    PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH2SET, 0x03 ) ;      // CH2: Measure VDD,    PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH3SET, 0x00 ) ;      // CH3: Normal,         PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH4SET, 0x00 ) ;      // CH4: Normal,         PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH5SET, 0x30 ) ;      // CH5: Normal,         PGA_Gain=6
ad1299_wreg( spi_dev, AD1299_ADDR_CH6SET, 0x00 ) ;      // CH6: Normal,         PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH7SET, 0x00 ) ;      // CH7: Normal,         PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH8SET, 0x00 ) ;      // CH8: Normal,         PGA_Gain=1
```
## Описание эксперимента:
* Использовался один канал номер 5.
* Скорость оцифровки 250 SPS.
* Три непрерывных записи по 30 секунд:
    * 0 - Сжатие кисти.
    * 1 - Кисть вверх.
    * 2 - Щелчек указательным и большим пальцем.

## Обработка
В данных присутсвует очень сильная помеха 50 Гц. Измерения проводились при питании от ноутбука с отключением от питающей сети 220в. Но наводки все равно остались. Возможно причина в длинных проводниках на плате между АЦП и разъемом.

Для подавления 50Гц и гармоники 100Гц применялся фильтр вида:

```
SPS                 = 250
hflt                = signal.firls( 151, [0, 40,  45, 55, 60, 90, 95, 105, 115, 125],   [1.0, 1.0,  0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0 ],fs = SPS)
npFiltSamples       = np.convolve( hflt, npSamples, 'same' )
```

## Формат хранения сигналов
Каждый файл содержит одну непрерывную выборку ~30сек. 
Формат файла - текстовый - в каждой строке один сэмпл. 

Запись производилась командой:
np.savetxt('emg_raw_data_n.txt',npRawSamples) - для сырых данных (как есть).
np.savetxt('emg_filt_data_n.txt',npRawSamples) - для фильтрованных данных.

Для чтения файла следует использовать команду:
npSamples = np.loadtxt('emg_filt_data_n.txt')

## Список файлов:
emg_raw_data_0.txt      - сырой файл со сжатием кисти.
emg_raw_data_1.txt      - сырой файл с подъемом кисти вверх.
emg_raw_data_2.txt      - сырой файл со щелчком указательным и большим пальцами.

emg_filt_data_0.txt      - фильтрованный файл со сжатием кисти.
emg_filt_data_1.txt      - фильтрованный файл с подъемом кисти вверх.
emg_filt_data_2.txt      - фильтрованный файл со щелчком указательным и большим пальцами.

