
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "midiManager.hpp"

static const char *TAG = "midi-mod";

// Midi Manager
static MidiManager midiManager;

void init()
{
  midiManager.initMidi();

  // Init GPIO
  midiManager.updateStates();
  midiManager.updateStates();
}

static void periodic_poll_midi_cb(void *arg)
{
  midiManager.updateStates();
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
