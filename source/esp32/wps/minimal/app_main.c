
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

#include "lwip/sockets.h"
#include "lwip/dns.h"
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
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
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
}
