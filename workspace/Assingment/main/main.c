#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_partition.h"
#include "nvs_flash.h"

#define PARTITION_NAME "new_partition"
#define FILE_URL "https://www.befunky.com/images/prismic/e8c80c0a-bc59-4df2-a86e-cc4eabd44285_hero-blur-image-1.jpg?auto=avif,webp&format=jpg&width=1000"
#define DOWNLOAD_BUFFER_SIZE 1024
#define PARTITION_SIZE (1 * 1024 * 1024) // 1MB partition size

static const char *TAG = "FILE_DOWNLOAD";

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGD(TAG, "Unhandled HTTP event (%d)", evt->event_id);
            break;
    }
    return ESP_OK;
}

void download_and_store_file() {
    ESP_LOGI(TAG, "Finding or creating partition...");

    // Try to find the existing partition
    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);

    // If the partition is not found, create a new one
    if (!partition) {
        ESP_LOGW(TAG, "Partition not found. Creating a new one...");

        // Create a new partition
        esp_partition_erase_range(esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL), 0, PARTITION_SIZE);

        // Try to find the newly created partition
        partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);

        if (!partition) {
            ESP_LOGE(TAG, "Failed to create a new partition. Exiting...");
            return;
        }
    }

    ESP_LOGI(TAG, "Partition found or created.");

    esp_http_client_config_t config = {
        .url = FILE_URL,
        .event_handler = _http_event_handler,
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP request completed successfully");
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    int read_len;
    int offset = 0;
    uint8_t buffer[DOWNLOAD_BUFFER_SIZE];

    while ((read_len = esp_http_client_read(client, (char *)buffer, DOWNLOAD_BUFFER_SIZE)) > 0) {
        esp_partition_write(partition, offset, buffer, read_len);
        offset += read_len;
    }

    ESP_LOGI(TAG, "Download and storage completed.");

    esp_http_client_cleanup(client);
}

void app_main() {
    ESP_LOGI(TAG, "Initializing NVS flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    download_and_store_file();
}
