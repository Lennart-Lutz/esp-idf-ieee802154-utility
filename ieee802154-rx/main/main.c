#include <string.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_ieee802154.h>
#include <esp_log.h>
#include <esp_phy_init.h>
#include <esp_mac.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/message_buffer.h>

#include "ieee802154_util.h"

#define TAG "main"
#define RADIO_TAG "ieee802154"

#define IEEE802154_PAN_ID 0x0001
#define IEEE802154_SHORT_ADDR_RECEIVER 0x0002

StreamBufferHandle_t xMessageBuffer = NULL;

/* --- IEEE802154 Functions --- */

void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

// ISR Context (Radio callbacks)

void esp_ieee802154_receive_sfd_done(void)
{
    ESP_EARLY_LOGI(RADIO_TAG, "RX sfd done, Radio state: %d", esp_ieee802154_get_state());
}

void esp_ieee802154_receive_done(uint8_t* frame, esp_ieee802154_frame_info_t* frame_info)
{
    ESP_EARLY_LOGI(RADIO_TAG, "RX OK, received %d bytes with rssi: %d and lqi: %d", frame[0], frame_info->rssi, frame_info->lqi);
    xMessageBufferSendFromISR(xMessageBuffer, frame, frame[0] + 1, NULL); // Add one to the frame length to get the lqi (the hardware inserts a 0 between data and rssi/lqi)
    esp_ieee802154_receive_handle_done(frame);
}

esp_err_t esp_ieee802154_enh_ack_generator(uint8_t *frame, esp_ieee802154_frame_info_t *frame_info, uint8_t *enhack_frame)
{
    esp_ieee802154_create_2015_ack_frame(frame, enhack_frame);
    return ESP_OK;
}

/* FreeRTOS Tasks */

static void receiver_task(void *pvParameters)
{
    uint8_t frame[127];

    while (1)
    {
        size_t readBytes = xMessageBufferReceive(xMessageBuffer, frame, sizeof(frame), portMAX_DELAY);
		if (readBytes == 0) break;

        //ESP_LOG_BUFFER_HEXDUMP(RADIO_TAG, frame, frame[0], ESP_LOG_INFO);
        esp_ieee802154_print_packet(frame);
    }

    ESP_LOGE("receiver_task", "Terminated");
    vTaskDelete(NULL);
}

void app_main()
{
    
    initialize_nvs();

    xMessageBuffer = xMessageBufferCreate(4 * 128);
    xTaskCreate(receiver_task, "receiver_task", 8192, NULL, 20, NULL);

    esp_err_t ret = esp_ieee802154_enable();
    if (ret == ESP_OK)
    {
        esp_ieee802154_set_coordinator(false);
        esp_ieee802154_set_promiscuous(false);

        esp_ieee802154_set_panid(IEEE802154_PAN_ID);
        esp_ieee802154_set_short_address(IEEE802154_SHORT_ADDR_RECEIVER);

        // For the promiscous mode to work, the hardware needs the address in reversed byte order
        uint8_t mac_addr[8] = {0};
        esp_read_mac(mac_addr, ESP_MAC_IEEE802154);
        uint8_t eui64_rev[8] = {0};
        for (int i=0; i<8; i++) {
            eui64_rev[7-i] = mac_addr[i];
        }
        esp_ieee802154_set_extended_address(eui64_rev);

        esp_ieee802154_set_channel(26);
        esp_ieee802154_set_txpower(0);

        esp_ieee802154_set_rx_when_idle(true);
        esp_ieee802154_receive();
    }

    while (1)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
