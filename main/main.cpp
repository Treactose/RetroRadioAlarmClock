#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Scanner.hpp"
#include "esp_timer.h"

static const Wire WIRES[] = {
    {GPIO_NUM_32, "J13"}, // left side
    {GPIO_NUM_33, "J16"}, // left side
    {GPIO_NUM_25, "J15"}, // left side
    {GPIO_NUM_26, "J14"}, // left side
    {GPIO_NUM_27, "J18"}, // left side
    {GPIO_NUM_13, "J19"}, // left side
};

static const int WIRE_COUNT = sizeof(WIRES) / sizeof(WIRES[0]);

static Scanner scanner(WIRES, WIRE_COUNT);

static void scan_task(void * /*arg*/)
{
    scanner.init();

    // Snapshot every 10 seconds as a reminder of current state
    int64_t last_snapshot = 0;

    while (true)
    {
        scanner.poll();

        int64_t now = esp_timer_get_time();
        if ((now - last_snapshot) >= 10000000LL)
        {
            scanner.snapshot();
            last_snapshot = now;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main()
{
    xTaskCreatePinnedToCore(
        scan_task,
        "scan_task",
        4096,
        nullptr,
        5,
        nullptr,
        0);
}