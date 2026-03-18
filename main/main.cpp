#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "buttons/Buttons.hpp"

static const char *TAG = "main";

static QueueHandle_t s_button_queue = nullptr;

static void handle_button(ButtonEvent event)
{
    switch (event)
    {
    case ButtonEvent::FAST:
        ESP_LOGI(TAG, "FAST pressed");
        break;
    case ButtonEvent::SLOW:
        ESP_LOGI(TAG, "SLOW pressed");
        break;
    case ButtonEvent::ALARM:
        ESP_LOGI(TAG, "ALARM pressed");
        break;
    case ButtonEvent::SLEEP:
        ESP_LOGI(TAG, "SLEEP pressed");
        break;
    case ButtonEvent::SNOOZE:
        ESP_LOGI(TAG, "SNOOZE pressed");
        break;
    }
}

static void button_task(void * /*arg*/)
{
    ButtonEvent event;
    while (true)
    {
        if (xQueueReceive(s_button_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            handle_button(event);
        }
    }
}

extern "C" void app_main()
{
    s_button_queue = xQueueCreate(10, sizeof(ButtonEvent));
    configASSERT(s_button_queue);

    static Buttons buttons(s_button_queue);
    buttons.init();

    xTaskCreatePinnedToCore(
        button_task,
        "button_task",
        2048,
        nullptr,
        5,
        nullptr,
        0);

    ESP_LOGI(TAG, "GE clock mod ready");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}