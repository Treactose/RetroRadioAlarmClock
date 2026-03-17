#include "Scanner.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "scanner";

Scanner::Scanner(const Wire *wires, int count)
    : m_wires(wires), m_count(count) {}

void Scanner::init()
{
    for (int i = 0; i < m_count; i++)
    {
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << m_wires[i].gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
    }

    // Capture initial state so we don't false-fire on boot
    m_last_mask = read_mask();

    ESP_LOGI(TAG, "Scanner ready — watching %d wires", m_count);
    ESP_LOGI(TAG, "Find GND first: the wire that shows LOW on every button press");
    ESP_LOGI(TAG, "Once found, move it to ESP32 GND pin\n");

    snapshot();
}

void Scanner::poll()
{
    uint32_t mask = read_mask();

    if (mask != m_last_mask)
    {
        report_change(mask);
        m_last_mask = mask;
    }
}

void Scanner::snapshot() const
{
    int64_t now_ms = esp_timer_get_time() / 1000;
    printf("\n=== SNAPSHOT @ %lldms ===\n", now_ms);

    for (int i = 0; i < m_count; i++)
    {
        int level = gpio_get_level(m_wires[i].gpio);
        printf("  %-10s GPIO%-2d  %s\n",
               m_wires[i].label,
               m_wires[i].gpio,
               level ? "HIGH" : "LOW *");
    }
    printf("  (* = LOW means shorted to GND)\n\n");
}

// Private ─────────────────────────────────────────────

uint32_t Scanner::read_mask() const
{
    uint32_t mask = 0;
    for (int i = 0; i < m_count; i++)
    {
        if (gpio_get_level(m_wires[i].gpio) == 0)
        {
            mask |= (1u << i);
        }
    }
    return mask;
}

void Scanner::report_change(uint32_t mask) const
{
    int64_t now_ms = esp_timer_get_time() / 1000;

    if (mask == 0)
    {
        printf("[%6lldms] Released — all pins back to HIGH\n", now_ms);
        return;
    }

    printf("[%6lldms] LOW detected: ", now_ms);
    for (int i = 0; i < m_count; i++)
    {
        if (mask & (1u << i))
        {
            printf("%s(GPIO%d) ", m_wires[i].label, m_wires[i].gpio);
        }
    }
    printf("\n");

    identify(mask);
}

void Scanner::identify(uint32_t mask) const
{
    int count = __builtin_popcount(mask);

    if (count == 1)
    {
        // Only one wire went LOW — it is a direct GND rail
        for (int i = 0; i < m_count; i++)
        {
            if (mask & (1u << i))
            {
                printf("  → %s is likely the COMMON GND rail\n",
                       m_wires[i].label);
                printf("     Move this wire to ESP32 GND pin!\n");
            }
        }
    }
    else if (count == 2)
    {
        // Two wires shorted — these are the two legs of one button
        int first = -1, second = -1;
        for (int i = 0; i < m_count; i++)
        {
            if (mask & (1u << i))
            {
                if (first < 0)
                    first = i;
                else
                    second = i;
            }
        }
        printf("  → BUTTON: %s <-> %s\n",
               m_wires[first].label,
               m_wires[second].label);
    }
    else if (count >= 3)
    {
        // Multiple wires low — one of them is GND, others are button legs
        printf("  → Multiple pins low. One is GND, others are button legs.\n");
        printf("     Press a different button to narrow it down.\n");
    }
}