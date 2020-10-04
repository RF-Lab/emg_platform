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
