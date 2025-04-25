
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_wps.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include "soc/gpio_num.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include  "esp_chip_info.h"

static const char *TAG                          = "EMG8x" ;

// DEBUG purpose pins
static const gpio_num_t         ESP_LED1          = GPIO_NUM_2 ;         // Debug pin #1


#if CONFIG_EMG8X_BOARD_EMULATION == 0

#if CONFIG_EMG8X_BOARD_REV == 1

static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_22 ;         // ADS<--ESP Power down pin (active low)
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_32 ;         // ADS<--ESP Reset pin (active low)
static const gpio_num_t     AD1299_DRDY_PIN     = GPIO_NUM_33 ;         // ADS-->ESP DRDY pin (active low)
static const gpio_num_t     AD1299_START_PIN    = GPIO_NUM_21 ;         // ADS<--ESP Start data conversion pint (active high)

#elif CONFIG_EMG8X_BOARD_REV == 2

static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_22 ;         // ADS<--ESP Power down pin (active low)
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_32 ;         // ADS<--ESP Reset pin (active low)
static const gpio_num_t     AD1299_DRDY_PIN     = GPIO_NUM_4 ;          // ADS-->ESP DRDY pin (active low)
static const gpio_num_t     AD1299_START_PIN    = GPIO_NUM_21 ;         // ADS<--ESP Start data conversion pint (active high)

#elif CONFIG_EMG8X_BOARD_REV == 3

// Same as Rev 2
static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_22 ;         // ADS<--ESP Power down pin (active low)
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_32 ;         // ADS<--ESP Reset pin (active low)
static const gpio_num_t     AD1299_DRDY_PIN     = GPIO_NUM_4 ;          // ADS-->ESP DRDY pin (active low)
static const gpio_num_t     AD1299_START_PIN    = GPIO_NUM_21 ;         // ADS<--ESP Start data conversion pint (active high)

#elif CONFIG_EMG8X_BOARD_REV == 4

// Same as Rev 2,3
static const gpio_num_t     AD1299_PWDN_PIN     = GPIO_NUM_22 ;         // ADS<--ESP Power down pin (active low)
static const gpio_num_t     AD1299_RESET_PIN    = GPIO_NUM_32 ;         // ADS<--ESP Reset pin (active low)
static const gpio_num_t     AD1299_DRDY_PIN     = GPIO_NUM_4 ;          // ADS-->ESP DRDY pin (active low)
static const gpio_num_t     AD1299_START_PIN    = GPIO_NUM_21 ;         // ADS<--ESP Start data conversion pint (active high)
static const gpio_num_t     BOARD_LED1          = GPIO_NUM_25 ;         // Extra led1
static const gpio_num_t     BOARD_LED2          = GPIO_NUM_26 ;         // Extra led2
static const gpio_num_t     BOARD_LED3          = GPIO_NUM_27 ;         // Extra led3

#else

#endif

// SPI comands (see https://www.ti.com/lit/ds/symlink/ads1299.pdf?ts=1599826124971)
static const uint8_t        AD1299_CMD_RREG     = 0x20 ;                // Read register
static const uint8_t        AD1299_CMD_WREG     = 0x40 ;                // Write to register
static const uint8_t        AD1299_CMD_RDATAC   = 0x10 ;                // Start continouous mode
static const uint8_t        AD1299_CMD_SDATAC   = 0x11 ;                // Stop continuous mode

// Register addresses available through SPI (see https://www.ti.com/lit/ds/symlink/ads1299.pdf?ts=1599826124971)
static const uint8_t        AD1299_ADDR_ID      = 0x00 ;                // ID register
static const uint8_t        AD1299_ADDR_CONFIG1 = 0x01 ;                // CONFIG1 register
static const uint8_t        AD1299_ADDR_CONFIG2 = 0x02 ;                // CONFIG2 register
static const uint8_t        AD1299_ADDR_CONFIG3 = 0x03 ;                // CONFIG3 register
static const uint8_t        AD1299_ADDR_LEADOFF = 0x04 ;
static const uint8_t        AD1299_ADDR_CH1SET  = 0x05 ;
static const uint8_t        AD1299_ADDR_CH2SET  = 0x06 ;
static const uint8_t        AD1299_ADDR_CH3SET  = 0x07 ;
static const uint8_t        AD1299_ADDR_CH4SET  = 0x08 ;
static const uint8_t        AD1299_ADDR_CH5SET  = 0x09 ;
static const uint8_t        AD1299_ADDR_CH6SET  = 0x0a ;
static const uint8_t        AD1299_ADDR_CH7SET  = 0x0b ;
static const uint8_t        AD1299_ADDR_CH8SET  = 0x0c ;

#endif

// AD1299 constants (see https://www.ti.com/lit/ds/symlink/ads1299.pdf?ts=1599826124971)
// Number of analog channels
#define                     AD1299_NUM_CH                   8


IRAM_ATTR void spi_data_pump_task( void* pvParameter ) ;

DRAM_ATTR spi_device_handle_t         spi_dev ;

DRAM_ATTR TaskHandle_t      tcpTaskHandle ;
DRAM_ATTR TaskHandle_t      datapumpTaskHandle ;

// DRDY signal interrupt context
typedef struct
{
    SemaphoreHandle_t       xSemaphore ;
} type_drdy_isr_context ;
DRAM_ATTR type_drdy_isr_context       drdy_isr_context ;

// ADC data variables combined into the structure
typedef struct
{

    uint8_t                 spiReadBuffer [27] ;            // Buffer to receive raw data from ADS1299 th SPI transaction

    uint32_t                adcStat32 ;                     // STAT field of last transaction in 32bit form (only 24bits used)

    // ISR to thread data queue
    int                     head ;          // 0 to CONFIG_EMG8X_TRANSPORT_QUE_SIZE-1
    int                     tail ;          // 0 to CONFIG_EMG8X_TRANSPORT_QUE_SIZE-1

    int                     sampleCount ;   // 0 to CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK-1

    uint32_t                blockCounter ;  // Number of received from ADC data blocks (num of full transport blocks)

    // read by thread, written by isr
    int32_t                 adcDataQue[CONFIG_EMG8X_TRANSPORT_QUE_SIZE][(CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE)/sizeof(int32_t)+(AD1299_NUM_CH+1)*(CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK)] ;

    // Counting semaphore represents number of transport blocks queued for
    // TCP delivery task
    SemaphoreHandle_t       xDataQueueSemaphore ;

} type_drdy_thread_context ;
DRAM_ATTR type_drdy_thread_context    drdy_thread_context ;

// TCP Server context
typedef struct
{

    // Pointer to ADC data
    type_drdy_thread_context*   pDrdyThreadContext ;
    char                        serverIpAddr[128] ;

} type_tcp_server_thread_context ;

DRAM_ATTR type_tcp_server_thread_context tcp_server_thread_context ;

/* Группа событий FreeRTOS для отслеживания состояния подключения к WiFi */
static EventGroupHandle_t s_wifi_event_group ;
// Биты состояния WiFi
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Количество попыток подключения
#define EXAMPLE_ESP_MAXIMUM_RETRY 3
// Режим работы WiFi
#define WPS_MODE WPS_TYPE_PBC
// Счетчик попыток подключения
static int s_retry_num = 0 ;

// Порт для сервера поиска
#define PORT 33333

// Конфигурация wps по умолчанию
static esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(WPS_MODE);
// Максимальное количество учетных данных для подключений к WiFi
static wifi_config_t wps_ap_creds[MAX_WPS_AP_CRED];
static int s_ap_creds_num = 0;

// Обработчик событий подключения к WiFi с помощью WPS
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    static int ap_idx = 1;

    switch (event_id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
            } else if (ap_idx < s_ap_creds_num) {
                /* Try the next AP credential if first one fails */

                if (ap_idx < s_ap_creds_num) {
                    ESP_LOGI(TAG, "Connecting to SSID: %s, Passphrase: %s",
                             wps_ap_creds[ap_idx].sta.ssid, wps_ap_creds[ap_idx].sta.password);
                    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wps_ap_creds[ap_idx++]) );
                    esp_wifi_connect();
                }
                s_retry_num = 0;
            } else {
                ESP_LOGI(TAG, "Failed to connect!");
            }

            break;
        case WIFI_EVENT_STA_WPS_ER_SUCCESS:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_SUCCESS");
         
            wifi_event_sta_wps_er_success_t *evt = (wifi_event_sta_wps_er_success_t *)event_data;
            int i;

            if (evt) {

                s_ap_creds_num = evt->ap_cred_cnt;
                for (i = 0; i < s_ap_creds_num; i++) {
                        
                    memcpy(wps_ap_creds[i].sta.ssid, evt->ap_cred[i].ssid,
                           sizeof(evt->ap_cred[i].ssid));
                    memcpy(wps_ap_creds[i].sta.password, evt->ap_cred[i].passphrase,
                           sizeof(evt->ap_cred[i].passphrase));
                }
                /* If multiple AP credentials are received from WPS, connect with first one */
                ESP_LOGI(TAG, "Connecting to SSID: %s, Passphrase: %s",
                         wps_ap_creds[0].sta.ssid, wps_ap_creds[0].sta.password);
                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wps_ap_creds[0]) );
            }
            /*
             * If only one AP credential is received from WPS, there will be no event data and
             * esp_wifi_set_config() is already called by WPS modules for backward compatibility
             * with legacy apps. So directly attempt connection here.
             */
            ESP_ERROR_CHECK(esp_wifi_wps_disable());
            esp_wifi_connect();
            
            wifi_config_t conf= { };
            esp_wifi_get_config(ESP_IF_WIFI_STA, &conf);
            ESP_LOGW(TAG, "WPS:: SSID=%s PASSWORD=%s", conf.sta.ssid, conf.sta.password);

            ESP_ERROR_CHECK(nvs_flash_erase());
            esp_err_t err = nvs_flash_init();
            ESP_ERROR_CHECK(err);
            nvs_handle_t my_handle;
            err = nvs_open("storage", NVS_READWRITE, &my_handle);
            if(err != ESP_OK) {
                ESP_LOGI(TAG, "ERROR OPEN NVS!!");
            }
            ESP_LOGI(TAG, "NVS OPENED!!");

            // сохранение имени сети
            err = nvs_set_str(my_handle, "wifi_ssid", (const char *)conf.sta.ssid);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "CANNOT TO WRITE SSID TO NVS!!!");
            }
            ESP_LOGI(TAG, "WRITE SSID TO NVS OK!!");

            // сохранение пароля
            err = nvs_set_str(my_handle, "wifi_password", (const char *)conf.sta.password);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "CANNOT TO WRITE SSID TO NVS!!!");
            }
            ESP_LOGI(TAG, "WRITE PASSWORD TO NVS OK!!");
            ESP_ERROR_CHECK(nvs_commit(my_handle));
            nvs_close(my_handle);
            
            break;
        case WIFI_EVENT_STA_WPS_ER_FAILED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_FAILED");
            ESP_ERROR_CHECK(esp_wifi_wps_disable());
            ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));
            ESP_ERROR_CHECK(esp_wifi_wps_start(0));
            break;
        case WIFI_EVENT_STA_WPS_ER_TIMEOUT:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_TIMEOUT");
            ESP_ERROR_CHECK(esp_wifi_wps_disable());
            ESP_ERROR_CHECK(esp_wifi_wps_enable(&config));
            ESP_ERROR_CHECK(esp_wifi_wps_start(0));
            break;
        case WIFI_EVENT_STA_WPS_ER_PIN:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_WPS_ER_PIN");
            /* display the PIN code */
            // wifi_event_sta_wps_er_pin_t* event = (wifi_event_sta_wps_er_pin_t*) event_data;
            // ESP_LOGI(TAG, "WPS_PIN = " PINSTR, PIN2STR(event->pin_code));
            // break;
        default:
            break;
    }
}

// Обработчик события при получении IP адреса
static void got_ip_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
}

static void start_wps(void)
{

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)) ;

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_event_handler, NULL));
    ESP_LOGI(TAG, "start wps...");


    esp_err_t err = esp_wifi_wps_enable(&config);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "ERROR WIFI WPS NOT ENABLED %s",esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "enabled config...");
    err = esp_wifi_wps_start(0);

    if(err == ESP_OK) {
        ESP_LOGI(TAG, "WPS SUCCESS");
    }
}

// Обработчик событий подключения к WiFi с помощью учетных данных сохраненных в постоянной памяти
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect() ;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP") ;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail") ;
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT) ;
    }
}


static void wifi_init(void)
{
    s_wifi_event_group = xEventGroupCreate() ;

    ESP_ERROR_CHECK(esp_netif_init()) ;

    ESP_ERROR_CHECK(esp_event_loop_create_default()) ;
    esp_netif_create_default_wifi_sta() ;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT() ;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)) ;

    esp_event_handler_instance_t instance_any_id ;
    esp_event_handler_instance_t instance_got_ip ;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id)) ;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip)) ;

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    
    char nvs_ssid[32];
    char nvs_password[64];

    if(err != ESP_OK) {
        ESP_LOGI(TAG, "ERROR OPEN NVS!!");
    }

    size_t ssid_size = sizeof(nvs_ssid);
    size_t password_size = sizeof(nvs_password);
    err = nvs_get_str(my_handle, "wifi_ssid", nvs_ssid, &ssid_size);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "%s",esp_err_to_name(err));
    }
    err = nvs_get_str(my_handle, "wifi_password", nvs_password, &password_size);
    if(err != ESP_OK) {
        ESP_LOGI(TAG, "%s",esp_err_to_name(err));
    }

    switch(err) {
        case ESP_OK:
            ESP_LOGI(TAG, "READING SSID FROM NVS OK!");
            strcpy((const char*)wifi_config.sta.ssid, (const char*)nvs_ssid);
            strcpy((const char*)wifi_config.sta.password, (const char*)nvs_password);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "THE NVS IS NOT INITIALIZED YET!");
            ESP_ERROR_CHECK(nvs_flash_init());
            break;
        default:
            ESP_LOGI(TAG, "ERROR %s READING FROM VNS!", esp_err_to_name(err));
    }
    
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    nvs_close(my_handle);


    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config.sta.ssid, wifi_config.sta.password ) ;
#if CONFIG_EMG8X_BOARD_REV == 4
        gpio_set_level( BOARD_LED1,     1 ) ;
#endif

    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD ) ;
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        start_wps();
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT") ;
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
}


#if CONFIG_EMG8X_BOARD_EMULATION == 0
// send 8bit command
IRAM_ATTR void ad1299_send_cmd8(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset(&t, 0, sizeof(spi_transaction_t)) ;      //Zero out the transaction
    t.flags         = SPI_TRANS_USE_TXDATA ;        //Bitwise OR of SPI_TRANS_* flags
    t.length        = 8 ;                           //Total data length, in bits
    t.user          = (void*)0 ;                    //User-defined variable. Can be used to store eg transaction ID.
    t.tx_data[0]    = cmd ;                         //Pointer to transmit buffer, or NULL for no MOSI phase
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
IRAM_ATTR void ad1299_wreg(spi_device_handle_t spi, const uint8_t addr, const uint8_t value)
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

IRAM_ATTR uint8_t ad1299_rreg(spi_device_handle_t spi, const uint8_t addr)
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
        //ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[0] ) ;
        //ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[1] ) ;
        //ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[2] ) ;
        //ESP_LOGI( TAG, "Read successfuly: 0x%02X", t.rx_data[3] ) ;
    }
    ESP_ERROR_CHECK(ret) ;                          //Should have had no issues.

    return (t.rx_data[0]) ;

}

/*
    ad1299_read_data_block216(spi_device_handle_t spi, uint8_t* data216)
    spi     - spi handle
    data216 - pointer to array of 27 bytes length (216 bits=24bit * 9)
*/
IRAM_ATTR void ad1299_read_data_block216(spi_device_handle_t spi, uint8_t* data216)
{
    esp_err_t ret ;
    spi_transaction_t t ;
    memset( &t, 0, sizeof(spi_transaction_t) ) ;    //Zero out the transaction
    t.flags         = 0 ;                           //Bitwise OR of SPI_TRANS_* flags
    t.length        = 216 ;                         //Total data length, in bits
    t.rxlength      = 216 ;                         //Total data length received, should be not greater than ``length`` in full-duplex mode (0 defaults this to the value of ``length``)
    t.user          = (void*)0 ;                    //User-defined variable. Can be used to store eg transaction ID.
    t.tx_buffer     = NULL ;                        // There are No tx data to send
    t.rx_buffer     = data216 ;                     // Pointer to Rx buffer

    ret = spi_device_polling_transmit(spi, &t) ;
    ESP_ERROR_CHECK(ret) ;                          //Should have had no issues.

}

// Rx data buffer for 1 transaction (9*24 bits)
uint8_t rxDataBuf [27] ;

// DRDY signal ISR
// DRDY becomes low when ADS1299 collect 8 samples (1 sample of 24bit for each channel )
// and ready to transfer these data to ESP32 using 1 SPI transaction with 9*24 bits length
IRAM_ATTR static void drdy_gpio_isr_handler(void* arg)
{
    //type_drdy_isr_context* pContext = (type_drdy_isr_context*) arg ;
    static BaseType_t high_task_wakeup = pdFALSE ;

    //xSemaphoreGiveFromISR( pContext->xSemaphore, &high_task_wakeup ) ;

    vTaskNotifyGiveFromISR( datapumpTaskHandle, &high_task_wakeup ) ;

    /* If high_task_wakeup was set to true you
    should yield.  The actual macro used here is
    port specific. */
    if ( high_task_wakeup!=pdFALSE )
    {
        portYIELD_FROM_ISR( ) ;
    }

}

#endif

// TCP server task
// This routine is waiting for data appears in ADC queue and
// then send to to connected tcp client(s)
char addr_str[128] ;
void tcp_server_task( void* pvParameter )
{

    // This routine is based on https://github.com/sankarcheppali/esp_idf_esp32_posts/blob/master/tcp_server/sta_mode/main/esp_sta_tcp_server.c

    type_tcp_server_thread_context* pTcpServerContext   = (type_tcp_server_thread_context*) pvParameter ;

    struct sockaddr_in tcpServerAddr ;
    tcpServerAddr.sin_addr.s_addr                       = htonl( INADDR_ANY ) ;
    tcpServerAddr.sin_family                            = AF_INET ;
    tcpServerAddr.sin_port                              = htons( CONFIG_EMG8X_TCP_SERVER_PORT ) ;
    int s ;
    static struct sockaddr_in remote_addr ;
    static long unsigned int socklen ;
    socklen = sizeof(remote_addr) ;
    int cs ; //client socket

    //xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY) ;

    while(1)
    {
        s = socket( AF_INET, SOCK_STREAM, 0) ;
        if(s < 0)
        {
            ESP_LOGE(TAG, "TCP_SERVER: Failed to allocate socket.\n") ;
            vTaskDelay(1000 / portTICK_PERIOD_MS) ;
            continue ;
        }

        if(bind(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0)
        {
            ESP_LOGE(TAG, "TCP_SERVER: socket bind failed errno=%d \n", errno) ;
            close(s) ;
            vTaskDelay(4000 / portTICK_PERIOD_MS) ;
            continue ;
        }

        if(listen (s, 1024) != 0)
        {
            ESP_LOGE(TAG, "TCP_SERVER: socket listen failed errno=%d \n", errno) ;
            close(s) ;
            vTaskDelay(4000 / portTICK_PERIOD_MS) ;
            continue ;
        }

        ESP_LOGI(TAG,"TCP server started at %s, listen on the port: %d, at CpuCore%1d\n", pTcpServerContext->serverIpAddr, CONFIG_EMG8X_TCP_SERVER_PORT, xPortGetCoreID() ) ;

        // data transfer cycle
        while(1)
        {
            cs  =   accept(s,(struct sockaddr *)&remote_addr, &socklen) ;

            inet_ntoa_r(remote_addr.sin_addr.s_addr, addr_str, sizeof(addr_str) - 1) ;
            ESP_LOGI(TAG,"New incoming connection from: %s\n", addr_str ) ;

#if CONFIG_EMG8X_BOARD_REV == 4
        gpio_set_level( BOARD_LED2,     1 ) ;
#endif


            while(1)
            {

                if (drdy_thread_context.tail!=drdy_thread_context.head)
                {
#if CONFIG_EMG8X_BOARD_REV == 4
                    gpio_set_level( BOARD_LED3,     1 ) ;
#endif

                    if( write(cs , drdy_thread_context.adcDataQue[drdy_thread_context.tail],
                        ((CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE)/sizeof(int32_t)+(AD1299_NUM_CH+1)*(CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK))*sizeof(int32_t)) < 0)
                    {
                        ESP_LOGE(TAG, "TCP_SERVER: Send failed\n") ;
                        break ;
                    }

#if CONFIG_EMG8X_BOARD_REV == 4
                    gpio_set_level( BOARD_LED3,     0 ) ;
#endif
                    ESP_LOGI(TAG, "TCP_SERVER: Block %d Sent.", (int)drdy_thread_context.adcDataQue[drdy_thread_context.tail][CONFIG_EMG8X_PKT_COUNT_OFFSET] ) ;

                    // move queue tail forward
                    drdy_thread_context.tail        = (drdy_thread_context.tail+1)%(CONFIG_EMG8X_TRANSPORT_QUE_SIZE) ;
                }

                //taskYIELD() ;

                // Notify or/and wait for data appears in the queue
                //if ( xSemaphoreTake( drdy_thread_context.xDataQueueSemaphore, 0xFFFF ) != pdTRUE )
                if (!ulTaskNotifyTake( pdFALSE, 0xffff )) // Wait or/and decrement count
                {
                    // still waiting for data from the ADC
                    ESP_LOGE(TAG, "TCP_SERVER: There are no data from ADC for a long time...\n") ;
                    continue ;
                }

            }

            inet_ntoa_r(remote_addr.sin_addr.s_addr, addr_str, sizeof(addr_str) - 1) ;
            ESP_LOGI(TAG, "Close connection to %s\n", addr_str ) ;

            close(cs) ;

#if CONFIG_EMG8X_BOARD_REV == 4
        gpio_set_level( BOARD_LED2,     0 ) ;
        gpio_set_level( BOARD_LED3,     0 ) ;
#endif

            vTaskDelay( 500 / portTICK_PERIOD_MS) ;
        }
    }

}

static void echo_server_task(void *pvParameters) {
    char rx_buffer[128];
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    struct sockaddr_in6 dest_addr;

    while (1) {

        if (addr_family == AF_INET) {
            struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
            dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
            dest_addr_ip4->sin_family = AF_INET;
            dest_addr_ip4->sin_port = htons(PORT);
            ip_protocol = IPPROTO_IP;
        }

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // Set timeout
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t socklen = sizeof(source_addr);

        while (1) {
            ESP_LOGI(TAG, "Waiting for data");

            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.ss_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
                } 

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
                
                if(rx_buffer[0] == 'E' && rx_buffer[1] == 'M' && rx_buffer[2] == 'G') {
                    char *response = "I am emg shield";
                    int length = strlen(response);
                    int err = sendto(sock, response, length, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));

                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }

            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

static void emg8x_app_start(void)
{
#if CONFIG_EMG8X_BOARD_EMULATION==0
//    int dmaChan             = 0 ;  // disable dma
    esp_err_t ret           = 0 ;

    ESP_LOGI(TAG, "Initialize GPIO lines") ;

    // Initialize GPIO pins



    gpio_reset_pin( AD1299_PWDN_PIN ) ;
    gpio_set_direction( AD1299_PWDN_PIN, GPIO_MODE_OUTPUT ) ;

    gpio_reset_pin( AD1299_RESET_PIN ) ;
    gpio_set_direction( AD1299_RESET_PIN, GPIO_MODE_OUTPUT ) ;

    gpio_reset_pin( AD1299_START_PIN ) ;
    gpio_set_direction( AD1299_START_PIN, GPIO_MODE_OUTPUT ) ;

    gpio_reset_pin( AD1299_DRDY_PIN ) ;
    gpio_set_direction( AD1299_DRDY_PIN, GPIO_MODE_INPUT ) ;
    gpio_set_intr_type( AD1299_DRDY_PIN, GPIO_INTR_NEGEDGE ) ;
    gpio_intr_enable( AD1299_DRDY_PIN ) ;

    // Debug pin
    //gpio_reset_pin( DEBUG_PIN1 ) ;
    //gpio_set_direction( DEBUG_PIN1, GPIO_MODE_OUTPUT ) ;
    //gpio_set_level( DEBUG_PIN1,     0 ) ;

    // See 10.1.2 Setting the Device for Basic Data Capture (ADS1299 Datasheet)
    ESP_LOGI(TAG, "Set PWDN & RESET to 1") ;
    gpio_set_level(AD1299_PWDN_PIN,     1 ) ;
    gpio_set_level(AD1299_RESET_PIN,    1 ) ;
    gpio_set_level(AD1299_START_PIN,    0 ) ;

    ESP_LOGI(TAG, "Wait for 20 tclk") ;

    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;
    //ets_delay_us( 15*10 ) ; //~30 clock periods @2MHz

    // Reset pulse
    ESP_LOGI(TAG, "Reset ad1299") ;
    gpio_set_level(AD1299_RESET_PIN, 0 ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;
    //ets_delay_us( 15*10 ) ;  //~20 clock periods @2MHz - This delay does not works, it is not enough for reset
    gpio_set_level(AD1299_RESET_PIN, 1 ) ;

    vTaskDelay( 500 / portTICK_PERIOD_MS ) ;

    ESP_LOGI(TAG, "Initialize SPI driver...") ;
    // SEE esp-idf/components/driver/include/driver/spi_common.h
    spi_bus_config_t buscfg = {
        .miso_io_num        = GPIO_NUM_19,
        .mosi_io_num        = GPIO_NUM_23,
        .sclk_io_num        = GPIO_NUM_18,
        .quadwp_io_num      = -1,
        .quadhd_io_num      = -1,
        .flags              = 0,                        // Abilities of bus to be checked by the driver. Or-ed value of ``SPICOMMON_BUSFLAG_*`` flags.
        .intr_flags         = ESP_INTR_FLAG_IRAM,
        .max_transfer_sz    = 0                         // maximum data size in bytes, 0 means 4094
    } ;

    // see esp-idf/components/driver/include/driver/spi_master.h
    spi_device_interface_config_t devcfg = {
        .command_bits       = 0,                        // 0-16
        .address_bits       = 0,                        // 0-64
        .dummy_bits         = 0,                        // Amount of dummy bits to insert between address and data phase
        .clock_speed_hz     = 1000000,                  // Clock speed, divisors of 80MHz, in Hz. See ``SPI_MASTER_FREQ_*``.
        .mode               = 1,                        // SPI mode 0
        .flags              = SPI_DEVICE_HALFDUPLEX,    // Bitwise OR of SPI_DEVICE_* flags
        .input_delay_ns     = 0,                        // The time required between SCLK and MISO
        .spics_io_num       = GPIO_NUM_5,               // CS pin
        .queue_size         = 1,                        // No queued transactions
        .cs_ena_pretrans    = 1,                        // 0 not used
        .cs_ena_posttrans   = 0,                        // 0 not used
    } ;

    //Initialize the SPI bus
    ret                     = spi_bus_initialize(VSPI_HOST, &buscfg, 0 ) ;
    ESP_ERROR_CHECK(ret) ;
    //Attach the LCD to the SPI bus
    ret                     = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_dev ) ;
    ESP_ERROR_CHECK(ret) ;
    ESP_LOGI(TAG, "SPI driver initialized, Freq limit is:%dHz", (int)spi_get_freq_limit( false, 0 ) ) ;


    // Send SDATAC / Stop Read Data Continuously Mode
    ESP_LOGI(TAG, "Send SDATAC") ;

    ad1299_send_cmd8( spi_dev, AD1299_CMD_SDATAC ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

    /*

    ESP_LOGI(TAG, "Read chip Id from Reg#0: 0x%02x", ad1299_rreg( spi_dev, AD1299_ADDR_ID ) ) ;

    ad1299_send_cmd8( spi_dev, AD1299_CMD_RDATAC ) ;

    gpio_set_level(AD1299_START_PIN, 1 ) ;

    // Install ISR for all GPIOs
    gpio_install_isr_service(0) ;

    // Configure ISR for DRDY signal
    drdy_isr_context.xSemaphore                     = xSemaphoreCreateBinary() ;
    gpio_isr_handler_add( AD1299_DRDY_PIN, drdy_gpio_isr_handler, &drdy_isr_context ) ;

    while(1)
    {

        // Wait for DRDY
        //while( gpio_get_level(AD1299_DRDY_PIN)==1 ) { vTaskDelay( 1  ) ;}
        if( xSemaphoreTake( drdy_isr_context.xSemaphore, 0xffff ) == pdTRUE )
        {

            // DRDY goes down - data ready to read
            ad1299_read_data_block216( spi_dev, drdy_thread_context.spiReadBuffer ) ;
        }
    }
    */


    // RREG id
    ESP_LOGI(TAG, "Read chip Id from Reg#0:") ;
    uint8_t valueu8             = ad1299_rreg( spi_dev, AD1299_ADDR_ID ) ;

    uint8_t ad1299_rev_id       = valueu8>>5 ;
    uint8_t ad1299_check_bit    = (valueu8>>4) & 0x01 ;
    uint8_t ad1299_dev_id       = (valueu8>>2) & 0x03 ;
    uint8_t ad1299_num_ch       = (valueu8) & 0x03 ;

    if (ad1299_check_bit && ad1299_dev_id==0x03 )
    {
        ESP_LOGI(TAG,"ads1299 found:") ;
        ESP_LOGI(TAG, "!-->ad1299_rev_id:       0x%02X",  ad1299_rev_id ) ;
        ESP_LOGI(TAG, "!-->ad1299_check_bit:    %1d (should be 1)",  ad1299_check_bit ) ;
        ESP_LOGI(TAG, "!-->ad1299_dev_id:       0x%02X",  ad1299_dev_id ) ;
        ESP_LOGI(TAG, "!-->ad1299_num_ch:       0x%02X",  ad1299_num_ch ) ;
    }
    else
    {
        ESP_LOGE(TAG, "error: ads1299 not found!" ) ;
        return ;
    }

    gpio_set_level( ESP_LED1,     1 ) ;


    // Set internal reference
    // WREG CONFIG3 E0h
    ESP_LOGI(TAG, "Set internal reference" ) ;
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG3, 0xEE ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

 /*   while(1)
    {
        uint8_t reg_value = ad1299_rreg( spi_dev, AD1299_ADDR_CONFIG3 ) ;
        ESP_LOGI(TAG, "check CONF3:       0x%02X",  reg_value ) ;
        vTaskDelay( 500 / portTICK_PERIOD_MS ) ;
    }*/

    //Set device for DR=fmod/4096
    // Enable clk output
    ESP_LOGI(TAG, "Set sampling rate" ) ;
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG1, 0xf4 ) ;     // Default 0x96 (see power up sequence)
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

    // Configure test signal parameters
    ad1299_wreg( spi_dev, AD1299_ADDR_CONFIG2, 0xb5 ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

    // Configure test signal parameters
    ad1299_wreg( spi_dev, AD1299_ADDR_LEADOFF, 0x0e ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

    // Set All Channels to Input Short
    for (int i=0;i<8;i++)
    {
        ad1299_wreg( spi_dev, AD1299_ADDR_CH1SET+i, 0x01 ) ;
        //vTaskDelay( 50 / portTICK_PERIOD_MS ) ;
    }
    vTaskDelay( 50 / portTICK_PERIOD_MS ) ;

    // Activate Conversion
    // After This Point DRDY Toggles at
    // fCLK / 8192
    gpio_set_level(AD1299_START_PIN, 1 ) ;
    vTaskDelay( 50 / portTICK_PERIOD_MS ) ;

    // Put the Device Back in RDATAC Mode
    // RDATAC
    ad1299_send_cmd8( spi_dev, AD1299_CMD_RDATAC ) ;
    vTaskDelay( 50 / portTICK_PERIOD_MS ) ;

    // Capture data and check for noise level
    for(int nSample=0;nSample<CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK;nSample++)
    {

        // Wait for DRDY
        while( gpio_get_level(AD1299_DRDY_PIN)==1 ) { vTaskDelay( 1  ) ;}

        // DRDY goes down - data ready to read
        ad1299_read_data_block216( spi_dev, rxDataBuf ) ;

        ESP_LOGI(TAG, "DATA: STAT:0x%02X%02X%02X DATA1:0x%02X%02X%02X",  rxDataBuf[0],rxDataBuf[1],rxDataBuf[2], rxDataBuf[3],rxDataBuf[4],rxDataBuf[5] ) ;

        // Place here code to collect samples and analyse noise level for
        // shorted analog inputs

        vTaskDelay( 1 ) ;

    }

    // Stop all the channels
    ad1299_send_cmd8( spi_dev, AD1299_CMD_SDATAC ) ;
    vTaskDelay( 100 / portTICK_PERIOD_MS ) ;

    // Configure channels
    ad1299_wreg( spi_dev, AD1299_ADDR_CH1SET, 0x40 ) ;      // CH1: Test signal,    PGA_Gain=1
    ad1299_wreg( spi_dev, AD1299_ADDR_CH2SET, 0x30 ) ;      // CH2: Measure VDD,    PGA_Gain=1
    ad1299_wreg( spi_dev, AD1299_ADDR_CH3SET, 0x30 ) ;      // CH3: Normal,         PGA_Gain=1
    ad1299_wreg( spi_dev, AD1299_ADDR_CH4SET, 0x00 ) ;      // CH4: Normal,         PGA_Gain=24
    ad1299_wreg( spi_dev, AD1299_ADDR_CH5SET, 0x30 ) ;      // CH5: Normal,         PGA_Gain=24
    ad1299_wreg( spi_dev, AD1299_ADDR_CH6SET, 0x50 ) ;      // CH6: Normal,         PGA_Gain=24
    ad1299_wreg( spi_dev, AD1299_ADDR_CH7SET, 0x30 ) ;      // CH7: Normal,         PGA_Gain=24
    ad1299_wreg( spi_dev, AD1299_ADDR_CH8SET, 0x30 ) ;      // CH8: Normal,         PGA_Gain=24
    vTaskDelay( 50 / portTICK_PERIOD_MS ) ;

    ESP_LOGI(TAG, "Put device in RDATAC mode" ) ;

    // Put the Device Back in RDATAC Mode
    // RDATAC
    ad1299_send_cmd8( spi_dev, AD1299_CMD_RDATAC ) ;
    vTaskDelay( 50 / portTICK_PERIOD_MS ) ;

#endif // Emulation

    // Initialize drdy_thread_context
    drdy_thread_context.head                        = 0 ;
    drdy_thread_context.tail                        = 0 ;
    drdy_thread_context.sampleCount                 = 0 ;
    drdy_thread_context.blockCounter                = 0 ;

    // Initialize counting semaphore which represents number of queued blocks
    drdy_thread_context.xDataQueueSemaphore         = xSemaphoreCreateCounting( CONFIG_EMG8X_TRANSPORT_QUE_SIZE, 0 ) ;

    for (int iQue=0;iQue<CONFIG_EMG8X_TRANSPORT_QUE_SIZE;iQue++)
    {

        unsigned char* pTransportBlockHeader        = (unsigned char*) drdy_thread_context.adcDataQue[iQue] ;

        // Fill in each header
        memset( pTransportBlockHeader, 0, CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE ) ;

        // Sync 5 bytes
        strncpy( (char*)pTransportBlockHeader, "EMG8x", 6 ) ;

        // Initialize packet counter
        drdy_thread_context.adcDataQue[iQue][CONFIG_EMG8X_PKT_COUNT_OFFSET]     = 0 ;

    }

    // Initialize tcp_server_thread_context
    tcp_server_thread_context.pDrdyThreadContext    = &drdy_thread_context ;

    // Start TCP server task
    //xTaskCreatePinnedToCore( &tcp_server_task, "tcp_server_task", 4096, &tcp_server_thread_context, 5, NULL, 0 ) ;
    /*https://github.com/espressif/esp-idf/blob/0bbc721a633f9afe4f3ebe648b9ca99e2f2f6d5f/components/freertos/include/freertos/task.h*/
    //xTaskCreate( &tcp_server_task, "tcp_server_task", 4096, &tcp_server_thread_context, tskIDLE_PRIORITY+5, &tcpTaskHandle ) ;

    // !!!
    // Data pump task and TCP task should be pinned to different cores to prevent influence of TCP to realtime data pump procedures

    // Start TCP task on Core#0
    xTaskCreatePinnedToCore( &tcp_server_task, "tcp_server_task", 4096, &tcp_server_thread_context, tskIDLE_PRIORITY+5, &tcpTaskHandle, 0 ) ;
    configASSERT( tcpTaskHandle ) ;

    // Start Data pump task on Core#1
    xTaskCreatePinnedToCore( &spi_data_pump_task, "spi_data_pump_task", 4096, &tcp_server_thread_context, tskIDLE_PRIORITY+5, &datapumpTaskHandle, 1 ) ;
    configASSERT( datapumpTaskHandle ) ;

#if CONFIG_EMG8X_BOARD_EMULATION==0
    // ISR uses datapumpTaskHandle to deliver notification

    // Install ISR for all GPIOs
    gpio_install_isr_service(0) ;

    // Configure ISR for DRDY signal
    drdy_isr_context.xSemaphore                     = xSemaphoreCreateBinary() ;
    gpio_isr_handler_add( AD1299_DRDY_PIN, drdy_gpio_isr_handler, &drdy_isr_context ) ;

#endif


}

#if CONFIG_EMG8X_BOARD_EMULATION
int numSamplesToMilliseconds(int numSamples)
{
    int sampleRate  = 16000/(1<<CONFIG_EMG8X_ADC_SAMPLING_FREQUENCY) ;
    return (1000*numSamples/sampleRate) ;
}
#endif

//float my_data_array[4096] ;
//int32_t sampleCounter = 0 ;

IRAM_ATTR void spi_data_pump_task( void* pvParameter )
{
    ESP_LOGI(TAG,"spi_data_pump_task started at CpuCore%1d\n", xPortGetCoreID() ) ;

    // Queue full flag
    int queueFull                                   = 0 ;

    // Continuous capture the data
    while(1)
    {

        // Wait for DRDY
        //while( gpio_get_level(AD1299_DRDY_PIN)==1 ) { vTaskDelay( 1  ) ;}
#if CONFIG_EMG8X_BOARD_EMULATION
        if (drdy_thread_context.sampleCount%60==0)
        {
            vTaskDelay( numSamplesToMilliseconds(60)/ portTICK_PERIOD_MS ) ;
        }
#else

        // Wait for DRDY signal becomes low
        if (gpio_get_level(AD1299_DRDY_PIN)!=0)
        {
            if (!ulTaskNotifyTake( pdTRUE, 0xffff ))
            {
                // Data is not ready
                continue ;
            }
        }

        // Wait using semaphore
        //if( gpio_get_level(AD1299_DRDY_PIN)==0 xSemaphoreTake( drdy_isr_context.xSemaphore, 0xffff )==pdTRUE )

#endif

        //gpio_set_level( DEBUG_PIN1,     1 ) ;
#if CONFIG_EMG8X_BOARD_EMULATION
        drdy_thread_context.spiReadBuffer[0] = 0xc0 ;
#else
        // Read data from device
        ad1299_read_data_block216( spi_dev, drdy_thread_context.spiReadBuffer ) ;
#endif
        //gpio_set_level( DEBUG_PIN1,     0 ) ;

        // Check for stat 0xCx presence
        if ((drdy_thread_context.spiReadBuffer[0]&0xf0)!=0xc0)
        {
            ESP_LOGE(TAG, "raw_REad:[%6d][%6d] 0xc0 not found!", (int)drdy_thread_context.blockCounter, (int)drdy_thread_context.sampleCount ) ;
            continue ;
        }

        //gpio_set_level( DEBUG_PIN1,     1 ) ;
        // get STAT field
        // transform 24 bit field to 32 bit integer
        drdy_thread_context.adcStat32   = (uint32_t) (
                                                        (((uint32_t)drdy_thread_context.spiReadBuffer[0])<<16) |
                                                        (((uint32_t)drdy_thread_context.spiReadBuffer[1])<<8) |
                                                        ((uint32_t) drdy_thread_context.spiReadBuffer[2])
                                                    ) ;

        // save STAT field to transport block
        drdy_thread_context.adcDataQue[drdy_thread_context.head][
            (CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE)/sizeof(int32_t) +
            drdy_thread_context.sampleCount ] = drdy_thread_context.adcStat32 ;

        // transform 24 bit twos-complement samples to 32 bit signed integers
        // save 32 bit samples into thread queue drdy_thread_context
        for(int chNum=0;chNum<AD1299_NUM_CH;chNum++)
        {

            uint32_t signExtBits        = 0x00000000 ;

            int byteOffsetCh            = (chNum+1)*3 ; // offset in raw data to first byte of 24 twos complement field

            if (drdy_thread_context.spiReadBuffer[ byteOffsetCh ] & 0x80)
            {
                // extend sign bit to 31..24
                signExtBits             = 0xff000000 ;
            }

            int32_t value_i32           =
                                        (int32_t) (
                                                        signExtBits |
                                                        (((uint32_t)drdy_thread_context.spiReadBuffer[byteOffsetCh])<<16) |
                                                        (((uint32_t)drdy_thread_context.spiReadBuffer[byteOffsetCh+1])<<8) |
                                                        ((uint32_t) drdy_thread_context.spiReadBuffer[byteOffsetCh+2])
                                                    ) ;
#if CONFIG_EMG8X_BOARD_EMULATION
            value_i32                   = (drdy_thread_context.sampleCount%20)<10 ;
#endif

            drdy_thread_context.adcDataQue[drdy_thread_context.head][
                (CONFIG_EMG8X_TRANSPORT_BLOCK_HEADER_SIZE)/sizeof(int32_t) +
                (CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK)*(chNum+1) +
                drdy_thread_context.sampleCount
                ]    = value_i32 ;
        } //for(int chNum=0
        //gpio_set_level( DEBUG_PIN1,     0 ) ;

        drdy_thread_context.sampleCount++ ;

        if (drdy_thread_context.sampleCount>=CONFIG_EMG8X_SAMPLES_PER_TRANSPORT_BLOCK)
        {
            // Transport block collection done

            // reset sample counter of transport block
            drdy_thread_context.sampleCount     = 0 ;

            // put packet counter to transport header
            drdy_thread_context.adcDataQue[drdy_thread_context.head][CONFIG_EMG8X_PKT_COUNT_OFFSET] = drdy_thread_context.blockCounter ;

            //gpio_set_level( DEBUG_PIN1,     1 ) ;

            if (!queueFull)
            {
                // Send notification to TCP server task and increment taskCounter
                uint32_t taskCounterValueBeforeIncrement  = 0 ;
                xTaskNotifyAndQuery( tcpTaskHandle, 0, eIncrement, &taskCounterValueBeforeIncrement ) ;
                if ((taskCounterValueBeforeIncrement+1)==CONFIG_EMG8X_TRANSPORT_QUE_SIZE)
                {
                    // Queue is over!
                    // Do not move head forward - wait for TCP server sends data to host
                    ESP_LOGE(TAG, "ADC_READ_CYCLE: TCP FIFO Full @ block %d\n", (int)drdy_thread_context.blockCounter ) ;
                    queueFull = 1 ;
                }
                else
                {
                    // There is empty space in the queue
                    // move queue head forward
                    drdy_thread_context.head        = (drdy_thread_context.head+1)%(CONFIG_EMG8X_TRANSPORT_QUE_SIZE) ;
                }

            }
            else
            {
                // check for queue state
                uint32_t taskCounterValue  = 0 ;
                xTaskNotifyAndQuery( tcpTaskHandle, 0, eNoAction, &taskCounterValue ) ;
                if ((taskCounterValue+1)<CONFIG_EMG8X_TRANSPORT_QUE_SIZE)
                {
                    ESP_LOGI(TAG, "ADC_READ_CYCLE: TCP FIFO ok @ block %d\n", (int)drdy_thread_context.blockCounter ) ;
                    queueFull = 0 ;
                }
            }

/*
            //if (xSemaphoreGive( drdy_thread_context.xDataQueueSemaphore )==pdTRUE)
            if ( !queueFull )
            {

                // move queue head forward
                drdy_thread_context.head        = (drdy_thread_context.head+1)%(CONFIG_EMG8X_TRANSPORT_QUE_SIZE) ;

                if (queueFull)
                {
                    ESP_LOGI(TAG, "ADC_READ_CYCLE: TCP FIFO ok @ block %d\n", drdy_thread_context.blockCounter ) ;
                    queueFull = 0 ;
                }
            }
            else
            {
                // There is no space in queue
                // skip data
                if (!queueFull)
                {
                    ESP_LOGE(TAG, "ADC_READ_CYCLE: TCP FIFO Full @ block %d\n", drdy_thread_context.blockCounter ) ;
                    queueFull = 1 ;
                }
            }
            */
            //gpio_set_level( DEBUG_PIN1,     0 ) ;

            // Increment received packet counter
            drdy_thread_context.blockCounter += 1 ;

            //ESP_LOGI(TAG, "head: %d",  drdy_thread_context.head ) ;
        }
    }
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..") ;
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", (int)esp_get_free_heap_size()) ;
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version()) ;

    esp_chip_info_t chip_info ;
    memset(&chip_info,0,sizeof(esp_chip_info_t)) ;
    esp_chip_info(&chip_info) ;
    switch(chip_info.model)
    {
        case CHIP_ESP32:
            ESP_LOGI(TAG, "[APP] Processor model: ESP32") ;
            break ;
        case CHIP_ESP32S2:
            ESP_LOGI(TAG, "[APP] Processor model: ESP32-S2") ;
            break ;
        default:
            ESP_LOGI(TAG, "[APP] Processor model: Unknown(%d)",(int)chip_info.model) ;
            break ;
    }
    ESP_LOGI(TAG, "[APP] Processor num cores: %d",(int)chip_info.cores) ;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init() ;
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase()) ;
      ret = nvs_flash_init() ;
      ESP_LOGI(TAG, "NVS ERASED!");
    }
    ESP_ERROR_CHECK(ret) ;

    gpio_reset_pin( ESP_LED1 ) ;
    gpio_set_direction( ESP_LED1, GPIO_MODE_OUTPUT ) ;
    gpio_set_level( ESP_LED1,     0 ) ;

    wifi_init() ;
    // emg8x_app_start() ;
    // Запуск задачи (сервер поиска платы в сети

    xTaskCreate(echo_server_task, "udp_server", 4096, (void*)AF_INET, 5, NULL);
    emg8x_app_start();
}
