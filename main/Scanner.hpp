#pragma once

#include <cstdint>
#include "driver/gpio.h"

// One entry per wire — label matches your physical connector
struct Wire
{
    gpio_num_t gpio;
    const char *label;
};

class Scanner
{
public:
    // Pass in your wire map and count
    Scanner(const Wire *wires, int count);

    // Call once in app_main
    void init();

    // Call in a loop — prints to serial on any state change
    void poll();

    // Print all pin states right now regardless of change
    void snapshot() const;

private:
    const Wire *m_wires;
    int m_count;
    uint32_t m_last_mask{0};

    uint32_t read_mask() const;
    void report_change(uint32_t mask) const;
    void identify(uint32_t mask) const;
};