
#include <stdlib.h>
#include <string>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "configManager.hpp"
#include "midiManager.hpp"

#define GPIO_CONFIG       GPIO_NUM_48     // GPIO48

static const char *TAG = "midi-mod";

// Managers
static ConfigManager configManager;
static MidiManager midiManager;

bool init()
{
  // check config gpio
  gpio_config_t config_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_CONFIG,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&config_conf));

  bool configMode = gpio_get_level(GPIO_CONFIG);

  configManager.init();

  Config config;
  if (!configMode)
  {
    printf("Open Device:\n");
    configManager.openDevice();
    return false;
  }
  else
  {
    printf("Parse Config:\n");
    midiManager.init(configManager.getConfig());

    // get initial midi vales
    midiManager.updateStates(true);
    return true;
  }
}

static void periodic_poll_midi_cb(void *arg)
{
  midiManager.updateStates(false);
  midiManager.handleUpdates();
}

extern "C" void app_main(void)
{
  if (init())
  {
    // Setup periodic timer
    const esp_timer_create_args_t periodic_midi_args =
    {
      .callback = &periodic_poll_midi_cb,
      .name = "periodic_midi"
    };

    esp_timer_handle_t periodic_midi_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_midi_args, &periodic_midi_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_midi_timer, 50000));
  }
}
