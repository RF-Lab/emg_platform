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
static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_19 ;
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_1 ;

static const uint8_t        AD1299_CMD_RREG     = 0x20 ;
static const uint8_t        AD1299_CMD_WREG     = 0x40 ;
static const uint8_t        AD1299_CMD_SDATAC   = 0x11 ;

static const uint8_t        AD1299_ADDR_CONFIG3 = 0x03 ;

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

void ad1299_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset(&t, 0, sizeof(spi_transaction_t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    int icmd = cmd ;
    ESP_LOGI( TAG, "Send command:0x%02X", icmd ) ;
    ret = spi_device_polling_transmit(spi, &t);  //Transmit!
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

    ad1299_cmd(spi, AD1299_CMD_RREG|addr ) ;

    memset(&t, 0, sizeof(spi_transaction_t)) ;       //Zero out the transaction
    t.flags = SPI_TRANS_USE_RXDATA ;
    t.user = (void*)1 ;
    ESP_LOGI( TAG, "Read from REG:0x%02X", addr ) ;
    ret = spi_device_polling_transmit(spi, &t) ;
    if (ret==ESP_OK)
    {
        ESP_LOGI( TAG, "Read successfuly: 0x%02X", *(uint8_t*)t.rx_data ) ;
    }
    ESP_ERROR_CHECK(ret) ;            //Should have had no issues.
    return ( *(uint8_t*)t.rx_data ) ;
}


void ad1299_wreg(spi_device_handle_t spi, const uint8_t addr, const uint8_t value)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    uint8_t tx_buf[3] ;
    memset(&t, 0, sizeof(spi_transaction_t));       //Zero out the transaction
    t.length=8*3;                     //Command is 8 bits
    tx_buf[0] = AD1299_CMD_WREG | addr ;
    tx_buf[1] = 0x00 ; // single register
    tx_buf[2] = value ;
    t.tx_buffer=tx_buf ;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ESP_LOGI( TAG, "Write :0x%02X to REG:0x%02X", value, addr ) ;
    ret = spi_device_polling_transmit(spi, &t);  //Transmit!
    if (ret==ESP_OK)
    {
        ESP_LOGI( TAG, "Sent successfuly" ) ;
    }
    ESP_ERROR_CHECK(ret) ;            //Should have had no issues.
}

static void emg8x_app_start(void)
{
    int dmaChan             = 2 ;
    spi_device_handle_t spi_dev ;
    esp_err_t ret           = 0 ;
    uint8_t cmd             = 0 ;

    ESP_LOGI(TAG, "Initialize GPIO lines") ;

    // Initialize GPIO pins
    gpio_reset_pin( AD1299_PWDN_PIN ) ;
    gpio_set_direction( AD1299_PWDN_PIN, GPIO_MODE_OUTPUT ) ;

    //gpio_reset_pin( AD1299_RESET_PIN ) ; // Do not reset this pin!!! (https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
    gpio_set_direction( AD1299_RESET_PIN, GPIO_MODE_OUTPUT ) ;

    // See 10.1.2 Setting the Device for Basic Data Capture (ADS1299 Datasheet)
    ESP_LOGI(TAG, "Set PWDN & RESET to 1") ;
    gpio_set_level(AD1299_PWDN_PIN, 1 ) ;
    gpio_set_level(AD1299_RESET_PIN, 1 ) ;
    
    ESP_LOGI(TAG, "Wait for 20 tclk") ;

    //vTaskDelay( 10 / portTICK_RATE_MS ) ;
    ets_delay_us( 10 ) ; //~20 clock periods @2MHz

    // Reset pulse
    ESP_LOGI(TAG, "Reset ad1299") ;
    gpio_set_level(AD1299_RESET_PIN, 0 ) ;
    ets_delay_us( 10 ) ;  //~20 clock periods @2MHz
    gpio_set_level(AD1299_RESET_PIN, 1 ) ;

    ESP_LOGI(TAG, "Initialize SPI driver...") ;
    spi_bus_config_t buscfg = {
        .miso_io_num        = GPIO_NUM_19,
        .mosi_io_num        = GPIO_NUM_23,
        .sclk_io_num        = GPIO_NUM_18,
        .quadwp_io_num      = -1,
        .quadhd_io_num      = -1,
        .max_transfer_sz    = 256
    } ;
    spi_device_interface_config_t devcfg={
        .clock_speed_hz     = 100*1000,                 // Clock out at 100 KHz
        .mode               = 0,                        // SPI mode 0
        .spics_io_num       = GPIO_NUM_5,               // CS pin
        .queue_size         = 2,                        // We want to be able to queue 2 transactions at a time
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
    
    ad1299_cmd( spi_dev, AD1299_CMD_SDATAC ) ;

    // RREG CONFIG3
    ad1299_rreg( spi_dev, AD1299_ADDR_CONFIG3 ) ;

    // WREG CONFIG3 E0h
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG3, 0xE0 ) ;

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
