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

static const char *TAG = "EMG8x";

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;


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

static void emg8x_app_start(void)
{
    int dmaChan             = 2 ;
    spi_device_handle_t spi_dev ;
    esp_err_t ret           = 0 ;

    spi_bus_config_t buscfg = {
        .miso_io_num        = GPIO_NUM_19,
        .mosi_io_num        = GPIO_NUM_23,
        .sclk_io_num        = GPIO_NUM_18,
        .quadwp_io_num      = -1,
        .quadhd_io_num      = -1,
        .max_transfer_sz    = 256
    } ;
    spi_device_interface_config_t devcfg={
        .clock_speed_hz     = 1*1000*1000,            // Clock out at 1 MHz
        .mode               = 0,                      // SPI mode 0
        .spics_io_num       = GPIO_NUM_5,             // CS pin
        .queue_size         = 2,                      // We want to be able to queue 2 transactions at a time
    } ;
    //Initialize the SPI bus
    ret                     = spi_bus_initialize(VSPI_HOST, &buscfg, dmaChan ) ;
    ESP_ERROR_CHECK(ret) ;
    //Attach the LCD to the SPI bus
    ret                     = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_dev ) ;
    ESP_ERROR_CHECK(ret) ;
    ESP_LOGI(TAG, "SPI driver initialized") ;
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
