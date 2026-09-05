
#include <stdlib.h>
#include <string>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bleConfig.hpp"
#include "configManager.hpp"
#include "midiManager.hpp"

static const char *TAG = "midi-mod";

// Managers
static ConfigManager configManager;
static MidiManager midiManager;
static BleConfig bleConfig;

// Called on the BLE worker task once a synced config has been written and
// parsed. Staging it lets the change take effect on the next poll tick rather
// than needing a replug.
static void onConfigApplied(const Config &config)
{
  midiManager.stageConfig(config);
}

static void periodic_poll_midi_cb(void *arg)
{
  midiManager.applyPendingConfig();
  midiManager.updateStates(false);
  midiManager.handleUpdates();
}

extern "C" void app_main(void)
{
  // There is no config boot mode any more: the device is always a USB MIDI
  // interface, and the configurator reaches it over BLE while it plays.
  configManager.init();

  midiManager.init(configManager.getConfig());
  midiManager.updateStates(true);

  bleConfig.start(&configManager, onConfigApplied);

  const esp_timer_create_args_t periodic_midi_args =
  {
    .callback = &periodic_poll_midi_cb,
    .name = "periodic_midi"
  };

  esp_timer_handle_t periodic_midi_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_midi_args, &periodic_midi_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_midi_timer, 50000));

  ESP_LOGI(TAG, "Running: USB MIDI up, BLE config service advertising");
}
