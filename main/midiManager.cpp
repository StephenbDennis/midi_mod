#include "midiManager.hpp"

void MidiManager::updateStates(bool initial)
{
  int value = 0;
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_0, &value));
  m_curState.modules[0].values[0] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_1, &value));
  m_curState.modules[0].values[1] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_2, &value));
  m_curState.modules[0].values[2] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_3, &value));
  m_curState.modules[0].values[3] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_4, &value));
  m_curState.modules[0].values[4] = scale(value);
  
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_0, &value));
  m_curState.modules[1].values[0] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_1, &value));
  m_curState.modules[1].values[1] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_2, &value));
  m_curState.modules[1].values[2] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_3, &value));
  m_curState.modules[1].values[3] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_1_4, &value));
  m_curState.modules[1].values[4] = scale(value);
  
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_0, &value));
  m_curState.modules[2].values[0] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_1, &value));
  m_curState.modules[2].values[1] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_2, &value));
  m_curState.modules[2].values[2] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_3, &value));
  m_curState.modules[2].values[3] = scale(value);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_4, &value));
  m_curState.modules[2].values[4] = scale(value);

  m_curState.modules[0].moduleSelect = gpio_get_level(GPIO_DEVICE_1_SELECT);
  m_curState.modules[1].moduleSelect = gpio_get_level(GPIO_DEVICE_2_SELECT);
  m_curState.modules[2].moduleSelect = gpio_get_level(GPIO_DEVICE_3_SELECT);

  if (initial)
  {
    m_lastState = m_curState;
  }

  printf("------------------------\n");
  printf("Select:\n");
  printf("    %d\n", m_curState.modules[0].moduleSelect);
  printf("    %d\n", m_curState.modules[1].moduleSelect);
  printf("    %d\n", m_curState.modules[2].moduleSelect);
  printf("Device: %d:\n", 1);
  printf("    %d\n", m_curState.modules[0].values[0]);
  printf("    %d\n", m_curState.modules[0].values[1]);
  printf("    %d\n", m_curState.modules[0].values[2]);
  printf("    %d\n", m_curState.modules[0].values[3]);
  printf("    %d\n", m_curState.modules[0].values[4]);
  printf("Device: %d:\n", 2);
  printf("    %d\n", m_curState.modules[1].values[0]);
  printf("    %d\n", m_curState.modules[1].values[1]);
  printf("    %d\n", m_curState.modules[1].values[2]);
  printf("    %d\n", m_curState.modules[1].values[3]);
  printf("    %d\n", m_curState.modules[1].values[4]);
  printf("Device: %d:\n", 3);
  printf("    %d\n", m_curState.modules[2].values[0]);
  printf("    %d\n", m_curState.modules[2].values[1]);
  printf("    %d\n", m_curState.modules[2].values[2]);
  printf("    %d\n", m_curState.modules[2].values[3]);
  printf("    %d\n", m_curState.modules[2].values[4]);
}

void MidiManager::init(Config config)
{
  m_config = config;
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
  // uint8_t NOTE_OFF = 0x80;
  // uint8_t NOTE_ON = 0x90;
  // uint8_t CONTROL_CHANGE = 0xB0;
  // uint8_t channel = 0x0;
  // uint8_t keyVel = 0x7F;

  // uint8_t EFFECT_1 = 0x0C;
  // uint8_t EFFECT_2 = 0x0D;

  if (tud_midi_mounted())
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 5; ++j)
      {
        if (m_lastState.modules[i].values[j] != m_curState.modules[i].values[j])
        {
          sendMidiMsg(i, j);
        }
      }
    }

    m_lastState = m_curState;

    uint8_t note_on[3] = {0x90, 1, keyVel};
    tud_midi_stream_write(0, note_on, 3);
    uint8_t note_off[3] = {0x80, 1, 0};
    tud_midi_stream_write(0, note_off, 3);
    uint8_t control_1[3] = {0xB0, 0x0C, 20};
    tud_midi_stream_write(0, control_1, 3);
    uint8_t control_2[3] = {0xB0, 0x0D, 40};
    tud_midi_stream_write(0, control_2, 3);
  }
}

uint8_t MidiManager::scale(int value)
{
  return (value * 127) / 4095;
}

void MidiManager::sendMidiMsg(uint8_t moduleIndex, uint8_t deviceIndex)
{

}