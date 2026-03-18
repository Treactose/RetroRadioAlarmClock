#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

enum class ButtonEvent : uint8_t
{
    FAST = 0,   // SW101 → GPIO32
    SLOW = 1,   // SW102 → GPIO34
    ALARM = 2,  // SW103 → GPIO35
    SLEEP = 3,  // SW104 → GPIO33
    SNOOZE = 4, // SW105 → GPIO27
};

class Buttons
{
public:
    explicit Buttons(QueueHandle_t event_queue);
    void init();

private:
    QueueHandle_t m_queue;

    static void configure_standard(gpio_num_t pin, ButtonEvent event);
    static void configure_input_only(gpio_num_t pin, ButtonEvent event);
    static void IRAM_ATTR isr_handler(void *arg);
};