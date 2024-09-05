#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "driver/gpio.h"

#define AP_SSID "EZGI_ESP_test"
#define AP_PASSWORD "12345678"
#define AP_MAX_CONN 4
#define AP_CHANNEL 1
#define DEFAULT_SCAN_LIST_SIZE 20
#define PORT 23

#define LED_PIN1 GPIO_NUM_38 
#define LED_PIN2 GPIO_NUM_37
#define LED_PIN3 GPIO_NUM_36
#define LED_PIN4 GPIO_NUM_35

#define LED_PIN5 GPIO_NUM_10 
#define LED_PIN6 GPIO_NUM_11
#define LED_PIN7 GPIO_NUM_12
#define LED_PIN8 GPIO_NUM_13

#define LOG_ENABLE 0
static const char *TAG = "wifi_ap";

typedef struct {
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count;
} scan_results_t;

static scan_results_t scan_results;
nvs_handle_t switch_handle;
static void wifi_scan(void)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    memset(&scan_results, 0, sizeof(scan_results));

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&scan_results.ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, scan_results.ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u", scan_results.ap_count);

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < scan_results.ap_count); i++) {
        ESP_LOGI(TAG, "%d - %s (%d) ", (i + 1), scan_results.ap_info[i].ssid, scan_results.ap_info[i].rssi);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi STA started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi STA disconnected");
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        printf("Got IP: %s\n", ip4addr_ntoa(&event->ip_info.ip));
    }
}

static void init_wifi(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .password = AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "init_wifi finished.");
}

static void connect_to_wifi(const char* ssid, const char* password)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}
static void switch_function(const char* state){
// 0. bit control (1. LED)
if (state[0] == '1') {
    gpio_set_level(LED_PIN1, 1);
} else if (state[0] == '0') {
    gpio_set_level(LED_PIN1, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[1] == '1') {
    gpio_set_level(LED_PIN2, 1);
} else if (state[1] == '0') {
    gpio_set_level(LED_PIN2, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[2] == '1') {
    gpio_set_level(LED_PIN3, 1);
} else if (state[2] == '0') {
    gpio_set_level(LED_PIN3, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[3] == '1') {
    gpio_set_level(LED_PIN4, 1);
} else if (state[3] == '0') {
    gpio_set_level(LED_PIN4, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[4] == '1') {
    gpio_set_level(LED_PIN5, 1);
} else if (state[4] == '0') {
    gpio_set_level(LED_PIN5, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[5] == '1') {
    gpio_set_level(LED_PIN6, 1);
} else if (state[5] == '0') {
    gpio_set_level(LED_PIN6, 0);
}else {
    ESP_LOGI(TAG, "Invalid command");
}

if (state[6] == '1') {
    gpio_set_level(LED_PIN7, 1);
} else if (state[6] == '0') {
    gpio_set_level(LED_PIN7, 0);
}
else {                    
    ESP_LOGI(TAG, "Invalid command");
}

if (state[7] == '1') {
    gpio_set_level(LED_PIN8, 1);
} else if (state[7] == '0') {
    gpio_set_level(LED_PIN8, 0);
}
else {
    ESP_LOGI(TAG, "Invalid command");
}   
                    }
static void telnet_task(void *pvParameters)
{
    char rx_buffer[128];
    
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    // LED GPIO pin output mode
    gpio_pad_select_gpio(LED_PIN1);
    gpio_set_direction(LED_PIN1, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN2);
    gpio_set_direction(LED_PIN2, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN3);
    gpio_set_direction(LED_PIN3, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN4);
    gpio_set_direction(LED_PIN4, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_PIN5);
    gpio_set_direction(LED_PIN5, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN6);
    gpio_set_direction(LED_PIN6, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN7);
    gpio_set_direction(LED_PIN7, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_PIN8);
    gpio_set_direction(LED_PIN8, GPIO_MODE_OUTPUT);

    while (1) {
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (listen_sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket bound, port %d", PORT);

        err = listen(listen_sock, 1);
        if (err != 0) {
          #ifdef LOG_ENABLE
            ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
          #endif
            break;
        }
        ESP_LOGI(TAG, "Socket listening");

        while (1) {
            struct sockaddr_in6 sourceAddr;
            uint addrLen = sizeof(sourceAddr);
            int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "Socket accepted");

            // new connected message "Wifi connedted" 
            const char *hello_msg = "Wifi connedted\r\n";
            send(sock, hello_msg, strlen(hello_msg), 0);

        

            while (1) {
                
                int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                if (len < 0) {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                    break;
                } else if (len == 0) {
                    ESP_LOGI(TAG, "Connection closed");
                    break;
                } else {
                    ESP_LOGI(TAG, "Received data:");
                    // for (int i = 0; i < len; i++) {
                    //     ESP_LOGI(TAG, "0x%02X ", rx_buffer[i]);
                    // }
                    err = nvs_open("storage", NVS_READWRITE, &switch_handle);
                    if (err != ESP_OK) {
                        //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));

                        ESP_LOGI(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                    } else {
                        err = nvs_set_str(switch_handle, "switch_state", rx_buffer);
                        const char* Failedmessage = (err != ESP_OK) ? "Failed!" : "Done";
                        ESP_LOGI(TAG, "%s", Failedmessage);

                        //printf((err != ESP_OK) ? "Failed!\n" : "Done\n"); 
                        nvs_close(switch_handle);
                    }
                    switch_function(rx_buffer);
                    
 
                                       
                }
            }

            if (sock != -1) {
                ESP_LOGE(TAG, "Shutting down socket and restarting...");
                shutdown(sock, 0);
                close(sock);
            }
        }

        if (listen_sock != -1) {
            ESP_LOGE(TAG, "Shutting down listen socket and restarting...");
            shutdown(listen_sock, 0);
            close(listen_sock);
        }
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);

    init_wifi();
    int err = nvs_open("storage", NVS_READWRITE, &switch_handle);
    if (err != ESP_OK) {
        //printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        ESP_LOGI(TAG,"Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        char switch_state[128]; // value will default to 0, if not set yet in NVS
        size_t required_size;
        err = nvs_get_str(switch_handle, "switch_state", switch_state, &required_size);
        switch (err) {
            case ESP_OK:
                //printf("Done\n");
                //printf("Switch state: %s\n", switch_state);
                ESP_LOGI(TAG,"Done\n");
                ESP_LOGI(TAG,"Switch state: %s\n", switch_state);
                switch_function(switch_state);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                //printf("The value is not initialized yet!\n");
                ESP_LOGI(TAG,"The value is not initialized yet!\n");
                break;
            default :
                //printf("Error (%s) reading!\n", esp_err_to_name(err));
                ESP_LOGI(TAG,"Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(switch_handle);
    }
    
    xTaskCreate(telnet_task, "telnet_task", 4096, NULL, 5, NULL);
}