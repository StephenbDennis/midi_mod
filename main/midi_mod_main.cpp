
#include <stdlib.h>
#include <string>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "configManager.hpp"
#include "midiManager.hpp"

#define GPIO_CONFIG       GPIO_NUM_38     // GPIO38

static const char *TAG = "midi-mod";

// Managers
static ConfigManager configManager;
static MidiManager midiManager;

void init()
{
    // check config gpio
  gpio_config_t config_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_CONFIG,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&config_conf));

  bool configMode = gpio_get_level(GPIO_CONFIG);

  configManager.init();

  Config config;
  if (false || configMode)
  {
    configManager.openDevice();
  }
  else
  {
    config = configManager.getConfig();
  }

  midiManager.init(config);

  // get initial midi vales
  midiManager.updateStates(true);
}

static void periodic_poll_midi_cb(void *arg)
{
  midiManager.updateStates(false);
  midiManager.handleUpdates();
}

extern "C" void app_main(void)
{
  init();

  // Setup periodic timer
  const esp_timer_create_args_t periodic_midi_args =
  {
    .callback = &periodic_poll_midi_cb,
    .name = "periodic_midi"
  };

  esp_timer_handle_t periodic_midi_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_midi_args, &periodic_midi_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_midi_timer, 1000000));
}
