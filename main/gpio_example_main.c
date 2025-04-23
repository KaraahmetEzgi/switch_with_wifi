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

#include "esp_system.h"
#include "esp_mac.h"



// AP_SSID oluşturma
char ap_ssid[32];  // SSID buffer
char device_mac_str[18]; // MAC buffer

void get_mac_address_str() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(device_mac_str, sizeof(device_mac_str), "ESP32S3_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

void create_unique_ap_ssid(void) {
    get_mac_address_str();
    snprintf(ap_ssid, sizeof(ap_ssid), "%s", device_mac_str);
}

#define AP_PASSWORD "12345678"
#define AP_MAX_CONN 4
#define AP_CHANNEL 1
#define DEFAULT_SCAN_LIST_SIZE 20
#define PORT 23

#define LOG_ENABLE 0
static const char *TAG = "wifi_ap";
static int led_pins[8] = {GPIO_NUM_38, GPIO_NUM_37, GPIO_NUM_36, GPIO_NUM_35, GPIO_NUM_10, GPIO_NUM_11, GPIO_NUM_12, GPIO_NUM_13};

typedef struct {
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count;
} scan_results_t;

static scan_results_t scan_results;
nvs_handle_t switch_handle;

static void wifi_scan(void) {
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

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi STA started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi STA disconnected");
        esp_wifi_connect();
        ESP_LOGI(TAG, "Retrying connection to AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void init_wifi(void) {
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

    wifi_config_t ap_config = {0};
    create_unique_ap_ssid();
    strncpy((char*)ap_config.ap.ssid, ap_ssid, sizeof(ap_config.ap.ssid));
    ap_config.ap.ssid_len = strlen(ap_ssid);
    strncpy((char*)ap_config.ap.password, AP_PASSWORD, sizeof(ap_config.ap.password));
    ap_config.ap.max_connection = AP_MAX_CONN;
    ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

    if (strlen(AP_PASSWORD) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "init_wifi finished.");
}

static void switch_function(const char* state) {
    for (size_t i = 0; i < 8; i++) {
        if (state[i] == '1') {
            gpio_set_level(led_pins[i], 1);
            ESP_LOGI("DEBUG", "Set HIGH for switch %d", i);
        } else if (state[i] == '0') {
            gpio_set_level(led_pins[i], 0);
            ESP_LOGI("DEBUG", "Set LOW for switch %d", i);
        } else {
            ESP_LOGI("ERROR", "Invalid command for switch %d", i);
            return;
        }
    }

    int err = nvs_open("storage", NVS_READWRITE, &switch_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    } else {
        ESP_LOGI("DEBUG", "switch_state: %s", state);
        err = nvs_set_str(switch_handle, "switch_state", state);
        ESP_LOGI(TAG, "%s", (err != ESP_OK) ? "Switch states setting failed! Try Again!" : "Switch states setting successful.");
        nvs_close(switch_handle);
    }
}

static void telnet_task(void *pvParameters)
{
    char rx_buffer[1024];
    
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    // LED GPIO pin output mode
    for (size_t i = 0; i < 8; i++)
    {
        gpio_set_direction(led_pins[i], GPIO_MODE_OUTPUT);
    }
    

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
            const char *hello_msg = "Wifi connected \r\n Please enter switch configuration";
            send(sock, hello_msg, strlen(hello_msg), 0);

        

            while (1) {
                bool success_flag = false;
                int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
                if (len < 0) {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                    break;
                } else if (len == 0) {
                    ESP_LOGI(TAG, "Connection closed");
                    break;
                } else {
                    // if (strncmp(rx_buffer, "\n", 2) != 0)
                    if (rx_buffer[len -1] == '\n' || rx_buffer[len-1] == '\r')
                    {
                        rx_buffer[len] = '\0';
                        ESP_LOGI(TAG, "Received data:%s", rx_buffer);
                        char tx_buffer[len+13];
                        snprintf(tx_buffer,sizeof(tx_buffer),"Switch setted %s \r\n",rx_buffer);
                        int send_res = send(sock,tx_buffer, strlen(tx_buffer), 0);
                        // if (send_res < 0) {
                        //     ESP_LOGE(TAG, "Error sending data back: errno %d", errno);
                        // } else {
                        //     ESP_LOGI(TAG, "Data sent back to client: %s", rx_buffer);
                        // }
                        switch_function(rx_buffer); 
                    }
                    
                            
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