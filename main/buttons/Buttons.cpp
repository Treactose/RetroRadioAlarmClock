#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "buttons/Buttons.hpp"

static const char *TAG = "buttons";

constexpr gpio_num_t PIN_FAST = GPIO_NUM_32;   // J13
constexpr gpio_num_t PIN_SLOW = GPIO_NUM_14;   // J14
constexpr gpio_num_t PIN_ALARM = GPIO_NUM_13;  // J15
constexpr gpio_num_t PIN_SLEEP = GPIO_NUM_33;  // J16
constexpr gpio_num_t PIN_SNOOZE = GPIO_NUM_27; // J19

static QueueHandle_t s_queue = nullptr;

// ─── ISR ──────────────────────────────────────────────────────────────────────
static int64_t s_last_isr_us[5] = {};
constexpr int64_t DEBOUNCE_US = 200000LL;

void IRAM_ATTR Buttons::isr_handler(void *arg)
{
    auto event = static_cast<ButtonEvent>(
        reinterpret_cast<uintptr_t>(arg));

    // Debounce inside ISR — reject if too soon after last press
    int idx = static_cast<int>(event);
    int64_t now = esp_timer_get_time();
    if ((now - s_last_isr_us[idx]) < DEBOUNCE_US)
    {
        return; // bounced — discard completely, don't queue
    }
    s_last_isr_us[idx] = now;

    BaseType_t higher_prio_woken = pdFALSE;
    xQueueSendFromISR(s_queue, &event, &higher_prio_woken);
    if (higher_prio_woken)
        portYIELD_FROM_ISR();
}

// ─── Constructor ──────────────────────────────────────────────────────────────

Buttons::Buttons(QueueHandle_t event_queue)
    : m_queue(event_queue) {}

// ─── Init ─────────────────────────────────────────────────────────────────────

void Buttons::init()
{
    s_queue = m_queue;

    gpio_install_isr_service(0);

    configure_standard(PIN_FAST, ButtonEvent::FAST);
    configure_standard(PIN_SLOW, ButtonEvent::SLOW);
    configure_standard(PIN_ALARM, ButtonEvent::ALARM);
    configure_standard(PIN_SLEEP, ButtonEvent::SLEEP);
    configure_standard(PIN_SNOOZE, ButtonEvent::SNOOZE);

    ESP_LOGI(TAG, "Buttons initialised");
    ESP_LOGI(TAG, "  FAST   -> GPIO%d", PIN_FAST);
    ESP_LOGI(TAG, "  SLOW   -> GPIO%d", PIN_SLOW);
    ESP_LOGI(TAG, "  ALARM  -> GPIO%d", PIN_ALARM);
    ESP_LOGI(TAG, "  SLEEP  -> GPIO%d", PIN_SLEEP);
    ESP_LOGI(TAG, "  SNOOZE -> GPIO%d", PIN_SNOOZE);
}

// ─── Private ──────────────────────────────────────────────────────────────────

void Buttons::configure_standard(gpio_num_t pin, ButtonEvent event)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_isr_handler_add(
        pin,
        isr_handler,
        reinterpret_cast<void *>(static_cast<uintptr_t>(event)));
}

void Buttons::configure_input_only(gpio_num_t pin, ButtonEvent event)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_isr_handler_add(
        pin,
        isr_handler,
        reinterpret_cast<void *>(static_cast<uintptr_t>(event)));
}