#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "app/app_controller.h"
#include "service/event_bus.h"

extern "C" void app_main(void) {
    static const char* kTag = "main";

    hb::EventBus bus;
    hb::AppController app(bus);

    if (!app.begin()) {
        ESP_LOGE(kTag, "init failed");
    }

    while (true) {
        app.tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
