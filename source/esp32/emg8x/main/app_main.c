#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"

static const char *TAG                          = "EMG8x" ;

// Pinout mapping
// Use pinout rules from here! (https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_19 ;
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_32 ;
static const gpio_num_t     AD1299_START_PIN    = GPIO_NUM_21 ;

// SPI comands
static const uint8_t        AD1299_CMD_RREG     = 0x20 ;
static const uint8_t        AD1299_CMD_WREG     = 0x40 ;
static const uint8_t        AD1299_CMD_SDATAC   = 0x11 ;

// Register addresses available through SPI
static const uint8_t        AD1299_ADDR_ID      = 0x00 ;
static const uint8_t        AD1299_ADDR_CONFIG1 = 0x01 ;
static const uint8_t        AD1299_ADDR_CONFIG2 = 0x02 ;
static const uint8_t        AD1299_ADDR_CONFIG3 = 0x03 ;
//static const uint8_t        AD1299_ADDR_LOFF    = 0x04 ;
static const uint8_t        AD1299_ADDR_CH1SET  = 0x05 ;
/*
static const uint8_t        AD1299_ADDR_CH2SET  = 0x06 ;
static const uint8_t        AD1299_ADDR_CH3SET  = 0x07 ;
static const uint8_t        AD1299_ADDR_CH4SET  = 0x08 ;
static const uint8_t        AD1299_ADDR_CH5SET  = 0x09 ;
static const uint8_t        AD1299_ADDR_CH6SET  = 0x0a ;
static const uint8_t        AD1299_ADDR_CH7SET  = 0x0b ;
static const uint8_t        AD1299_ADDR_CH8SET  = 0x0c ;
*/

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT  = BIT0;


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect() ;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT) ;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT) ;
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init() ;
    wifi_event_group        = xEventGroupCreate() ;
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL)) ;
    wifi_init_config_t cfg  = WIFI_INIT_CONFIG_DEFAULT() ;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)) ;
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)) ;
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    } ;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)) ;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)) ;
    ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_WIFI_SSID) ;
    ESP_ERROR_CHECK(esp_wifi_start()) ;
    ESP_LOGI(TAG, "Waiting for wifi") ;
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY) ;
}

// send 8bit command
void ad1299_send_cmd8(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset(&t, 0, sizeof(spi_transaction_t)) ;      //Zero out the transaction
    t.flags         = 0 ;                           //Bitwise OR of SPI_TRANS_* flags
    t.length        = 8 ;                           //Total data length, in bits
    t.user          = (void*)0 ;                    //User-defined variable. Can be used to store eg transaction ID.
    t.tx_buffer     = &cmd ;                        //Pointer to transmit buffer, or NULL for no MOSI phase
    t.rx_buffer     = NULL ;                        //Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.

    int icmd = cmd ;
    ESP_LOGI( TAG, "ad1299_cmd8: send command:0x%02X", icmd ) ;

    ret = spi_device_polling_transmit( spi, &t ) ;  // send command
    if (ret==ESP_OK)
    {
        ESP_LOGI( TAG, "Sent successfuly" ) ;
    }
    ESP_ERROR_CHECK(ret) ;            //Should have had no issues.
}

// Write to ad1299 register
void ad1299_wreg(spi_device_handle_t spi, const uint8_t addr, const uint8_t value)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset(&t, 0, sizeof(spi_transaction_t)) ;      //Zero out the transaction
    t.flags         = SPI_TRANS_USE_TXDATA ;        //Bitwise OR of SPI_TRANS_* flags
    t.length        = 8*3 ;                         //Total data length, in bits
    t.user          = (void*)0 ;                    //User-defined variable. Can be used to store eg transaction ID.
    t.tx_data[0]    = AD1299_CMD_WREG|addr ;
    t.tx_data[1]    = 0 ;                           // 1 register to read
    t.tx_data[2]    = value ;                       // value to write
    t.tx_data[3]    = 0 ;                           // NOP
    t.rx_buffer     = NULL ;                        //Pointer to receive buffer, or NULL for no MISO phase. Written by 4 bytes-unit if DMA is used.

    ESP_LOGI( TAG, "ad1299_wreg :0x%02X to REG:0x%02X", value, addr ) ;
    ret = spi_device_polling_transmit(spi, &t) ;    //Transmit!
    if (ret==ESP_OK)
    {
        ESP_LOGI( TAG, "Sent successfuly" ) ;
    }
    ESP_ERROR_CHECK(ret) ;            //Should have had no issues.
}

uint8_t ad1299_rreg(spi_device_handle_t spi, const uint8_t addr)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset( &t, 0, sizeof(spi_transaction_t) ) ;    //Zero out the transaction
    t.flags         = SPI_TRANS_USE_RXDATA|
                      SPI_TRANS_USE_TXDATA ;        //Bitwise OR of SPI_TRANS_* flags
    t.length        = 8*2 ;                         //Total data length, in bits
    t.rxlength      = 8 ;                           //Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``)
    t.user          = (void*)0 ;                    //User-defined variable. Can be used to store eg transaction ID.

    t.tx_data[0]    = AD1299_CMD_RREG|addr ;
    t.tx_data[1]    = 0 ;                           // 1 register to read
    t.tx_data[2]    = 0 ;                           // NOP
    t.tx_data[3]    = 0 ;                           // NOP

    ESP_LOGI( TAG, "ad1299_rreg: read from REG:0x%02X", addr ) ;
    ret = spi_device_polling_transmit(spi, &t) ;
    if (ret==ESP_OK)
    {
        ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[0] ) ;
        ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[1] ) ;
        ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[2] ) ;
        ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[3] ) ;
    }
    ESP_ERROR_CHECK(ret) ;                          //Should have had no issues.

    return (t.rx_data[0]) ;

}

static void emg8x_app_start(void)
{
    int dmaChan             = 0 ;  // disable dma
    spi_device_handle_t spi_dev ;
    esp_err_t ret           = 0 ;

    ESP_LOGI(TAG, "Initialize GPIO lines") ;

    // Initialize GPIO pins
    gpio_reset_pin( AD1299_PWDN_PIN ) ;
    gpio_set_direction( AD1299_PWDN_PIN, GPIO_MODE_OUTPUT ) ;

    gpio_reset_pin( AD1299_RESET_PIN ) ; 
    gpio_set_direction( AD1299_RESET_PIN, GPIO_MODE_OUTPUT ) ;

    gpio_reset_pin( AD1299_START_PIN ) ;
    gpio_set_direction( AD1299_START_PIN, GPIO_MODE_OUTPUT ) ;

    // See 10.1.2 Setting the Device for Basic Data Capture (ADS1299 Datasheet)
    ESP_LOGI(TAG, "Set PWDN & RESET to 1") ;
    gpio_set_level(AD1299_PWDN_PIN, 1 ) ;
    gpio_set_level(AD1299_RESET_PIN, 1 ) ;
    gpio_set_level(AD1299_START_PIN, 0 ) ;
    
    ESP_LOGI(TAG, "Wait for 20 tclk") ;

    //vTaskDelay( 10 / portTICK_RATE_MS ) ;
    ets_delay_us( 15 ) ; //~30 clock periods @2MHz

    // Reset pulse
    ESP_LOGI(TAG, "Reset ad1299") ;
    gpio_set_level(AD1299_RESET_PIN, 0 ) ;
    ets_delay_us( 15 ) ;  //~20 clock periods @2MHz
    gpio_set_level(AD1299_RESET_PIN, 1 ) ;

    vTaskDelay( 500 / portTICK_RATE_MS ) ;

    ESP_LOGI(TAG, "Initialize SPI driver...") ;
    // SEE esp-idf/components/driver/include/driver/spi_common.h
    spi_bus_config_t buscfg = {
        .miso_io_num        = GPIO_NUM_19,
        .mosi_io_num        = GPIO_NUM_23,
        .sclk_io_num        = GPIO_NUM_18,
        .quadwp_io_num      = -1,
        .quadhd_io_num      = -1,
        .flags              = 0,                        // Abilities of bus to be checked by the driver. Or-ed value of ``SPICOMMON_BUSFLAG_*`` flags.
        .intr_flags         = 0,
        .max_transfer_sz    = 0                         // maximum data size in bytes, 0 means 4094
    } ;

    // see esp-idf/components/driver/include/driver/spi_master.h
    spi_device_interface_config_t devcfg = {
        .command_bits       = 0,                        // 0-16
        .address_bits       = 0,                        // 0-64
        .dummy_bits         = 0,                        // Amount of dummy bits to insert between address and data phase 
        .clock_speed_hz     = 200000,                   // Clock speed, divisors of 80MHz, in Hz. See ``SPI_MASTER_FREQ_*``.
        .mode               = 1,                        // SPI mode 0
        .flags              = SPI_DEVICE_HALFDUPLEX,    // Bitwise OR of SPI_DEVICE_* flags
        .input_delay_ns     = 0,                        // The time required between SCLK and MISO
        .spics_io_num       = GPIO_NUM_5,               // CS pin
        .queue_size         = 1,                        // No queued transactions
        .cs_ena_pretrans    = 1,                        // 0 not used
        .cs_ena_posttrans   = 0,                        // 0 not used                        
    } ;
    
    //Initialize the SPI bus
    ret                     = spi_bus_initialize(VSPI_HOST, &buscfg, dmaChan ) ;
    ESP_ERROR_CHECK(ret) ;
    //Attach the LCD to the SPI bus
    ret                     = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_dev ) ;
    ESP_ERROR_CHECK(ret) ;
    ESP_LOGI(TAG, "SPI driver initialized") ;

    // Send SDATAC / Stop Read Data Continuously Mode
    ESP_LOGI(TAG, "Send SDATAC") ;
    
    ad1299_send_cmd8( spi_dev, AD1299_CMD_SDATAC ) ;
    vTaskDelay( 500 / portTICK_RATE_MS ) ;

    // Set internal reference
    // WREG CONFIG3 E0h
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG3, 0xE0 ) ;

    //Set device for DR=fmod/4096
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG1, 0x96 ) ;
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG2, 0xc0 ) ;

    // Set All Channels to Input Short
    for (int i=0;i<8;i++)
    {
        ad1299_wreg( spi_dev, AD1299_ADDR_CH1SET+i, 0x01 ) ;
    }

    // Activate Conversion
    // After This Point DRDY Toggles at
    // fCLK / 8192
    gpio_set_level(AD1299_START_PIN, 1 ) ;


    while(1)
    {
    // RREG id
    ESP_LOGI(TAG, "Read chip Id from Reg#0:") ;
    uint8_t valueu8 = ad1299_rreg( spi_dev, AD1299_ADDR_ID ) ;

    uint8_t ad1299_rev_id   = valueu8>>5 ;
    uint8_t ad1299_check_bit= (valueu8>>4) & 0x01 ;
    uint8_t ad1299_dev_id   = (valueu8>>2) & 0x03 ;
    uint8_t ad1299_num_ch   = (valueu8) & 0x03 ;
    
    ESP_LOGI(TAG, "ad1299_rev_id:       0x%02X",  ad1299_rev_id ) ;
    ESP_LOGI(TAG, "ad1299_check_bit:    %1d (should be 1)",  ad1299_check_bit ) ;
    ESP_LOGI(TAG, "ad1299_dev_id:       0x%02X",  ad1299_dev_id ) ;
    ESP_LOGI(TAG, "ad1299_num_ch:       0x%02X",  ad1299_num_ch ) ;
    vTaskDelay( 500 / portTICK_RATE_MS ) ;
    }

}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init() ;
    wifi_init() ;
    emg8x_app_start() ;
}
