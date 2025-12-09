#include "midiManager.hpp"

void MidiManager::updateStates()
{
  // Device 1
  int val_0_0 = 0;
  int val_0_1 = 0;
  int val_0_2 = 0;
  int val_0_3 = 0;
  int val_0_4 = 0;

  // Device 2
  int val_1_0 = 0;
  int val_1_1 = 0;
  int val_1_2 = 0;
  int val_1_3 = 0;
  int val_1_4 = 0;

  // Device 3
  int val_2_0 = 0;
  int val_2_1 = 0;
  int val_2_2 = 0;
  int val_2_3 = 0;
  int val_2_4 = 0;

  // Select
  int device1Select = 0;
  int device2Select = 0;
  int device3Select = 0;
  int layerSelect = 0;

  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_0, &val_0_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_1, &val_0_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_2, &val_0_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_3, &val_0_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_4, &val_0_4));

  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_0, &val_1_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_1, &val_1_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_2, &val_1_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_3, &val_1_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_1_4, &val_1_4));

  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_0, &val_2_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_1, &val_2_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_2, &val_2_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_3, &val_2_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_4, &val_2_4));

  device1Select = gpio_get_level(GPIO_DEVICE_1_SELECT);
  device2Select = gpio_get_level(GPIO_DEVICE_2_SELECT);
  device3Select = gpio_get_level(GPIO_DEVICE_3_SELECT);
  layerSelect = gpio_get_level(GPIO_LAYER_SELECT);

  printf("------------------------\n");
  printf("Select:\n");
  printf("    %d\n", device1Select);
  printf("    %d\n", device2Select);
  printf("    %d\n", device3Select);
  printf("    %d\n", layerSelect);
  printf("Device: %d:\n", 1);
  printf("    %d\n", val_0_0);
  printf("    %d\n", val_0_1);
  printf("    %d\n", val_0_2);
  printf("    %d\n", val_0_3);
  printf("    %d\n", val_0_4);
  printf("Device: %d:\n", 2);
  printf("    %d\n", val_1_0);
  printf("    %d\n", val_1_1);
  printf("    %d\n", val_1_2);
  printf("    %d\n", val_1_3);
  printf("    %d\n", val_1_4);
  printf("Device: %d:\n", 3);
  printf("    %d\n", val_2_0);
  printf("    %d\n", val_2_1);
  printf("    %d\n", val_2_2);
  printf("    %d\n", val_2_3);
  printf("    %d\n", val_2_4);
}

void MidiManager::initMidi()
{
    // Setup USB
  tinyusb_config_t const tusb_cfg =
  {
    .device_descriptor = NULL, // If device_descriptor is NULL, tinyusb_driver_install() will use Kconfig
    .string_descriptor = tinyUsbConfig,
    .string_descriptor_count = sizeof(tinyUsbConfig) / sizeof(tinyUsbConfig[0]),
    .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = s_midi_cfg_desc,
    .hs_configuration_descriptor = s_midi_hs_cfg_desc,
    .qualifier_descriptor = NULL,
#else
    .configuration_descriptor = s_midi_cfg_desc,
#endif // TUD_OPT_HIGH_SPEED
  };

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  // Setup GPIO. Make sure the channels are correct
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_1_config, &adc_1_handle));
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_2_config, &adc_2_handle));

  adc_oneshot_chan_cfg_t chan_config =
  {
    .atten = ADC_ATTEN_DB_11,  // 3.3V max
    .bitwidth = ADC_BITWIDTH_DEFAULT
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_4, &chan_config));

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_1_4, &chan_config));

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_4, &chan_config));

  gpio_config_t device_1_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_1_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_1_select_conf));

  gpio_config_t device_2_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_2_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_2_select_conf));

  gpio_config_t device_3_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_3_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_3_select_conf));

  gpio_config_t layer_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_LAYER_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&layer_select_conf));
}

void MidiManager::handleUpdates()
{
  if (tud_midi_mounted())
  {
    uint8_t note_on[3] = {NOTE_ON | channel, 1, keyVel};
    tud_midi_stream_write(cable_num, note_on, 3);
    uint8_t note_off[3] = {NOTE_OFF | channel, 1, 0};
    tud_midi_stream_write(cable_num, note_off, 3);
    uint8_t control_1[3] = {CONTROL_CHANGE | channel, EFFECT_1, 20};
    tud_midi_stream_write(cable_num, control_1, 3);
    uint8_t control_2[3] = {CONTROL_CHANGE | channel, EFFECT_2, 40};
    tud_midi_stream_write(cable_num, control_2, 3);
  }
}