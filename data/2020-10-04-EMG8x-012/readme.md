# Тестовый эксперимент по сбору EMG сигналов для проверки EMG8x

## Настройки АЦП

```
// Configure channels
ad1299_wreg( spi_dev, AD1299_ADDR_CH1SET, 0x05 ) ;      // CH1: Test signal,    PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH2SET, 0x03 ) ;      // CH2: Measure VDD,    PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH3SET, 0x00 ) ;      // CH3: Normal,         PGA_Gain=1
ad1299_wreg( spi_dev, AD1299_ADDR_CH4SET, 0x00 ) ;      // CH4: Normal,         PGA_Gain=24
ad1299_wreg( spi_dev, AD1299_ADDR_CH5SET, 0x30 ) ;      // CH5: Normal,         PGA_Gain=24
ad1299_wreg( spi_dev, AD1299_ADDR_CH6SET, 0x00 ) ;      // CH6: Normal,         PGA_Gain=24
ad1299_wreg( spi_dev, AD1299_ADDR_CH7SET, 0x00 ) ;      // CH7: Normal,         PGA_Gain=24
ad1299_wreg( spi_dev, AD1299_ADDR_CH8SET, 0x00 ) ;      // CH8: Normal,         PGA_Gain=24
```
