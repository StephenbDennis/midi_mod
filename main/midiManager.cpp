#include "midiManager.hpp"

void MidiManager::updateStates(bool initial)
{
  int value = 0;
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_0, &value));
  m_curState.modules[0].values[0] += m_config.modules[0].m_alpha * (scale(value) - m_curState.modules[0].values[0]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_1, &value));
  m_curState.modules[0].values[1] += m_config.modules[0].m_alpha * (scale(value) - m_curState.modules[0].values[1]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_2, &value));
  m_curState.modules[0].values[2] += m_config.modules[0].m_alpha * (scale(value) - m_curState.modules[0].values[2]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_3, &value));
  m_curState.modules[0].values[3] += m_config.modules[0].m_alpha * (scale(value) - m_curState.modules[0].values[3]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_4, &value));
  m_curState.modules[0].values[4] += m_config.modules[0].m_alpha * (scale(value) - m_curState.modules[0].values[4]);
  
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_0, &value));
  m_curState.modules[1].values[0] += m_config.modules[1].m_alpha * (scale(value) - m_curState.modules[1].values[0]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_1, &value));
  m_curState.modules[1].values[1] += m_config.modules[1].m_alpha * (scale(value) - m_curState.modules[1].values[1]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_2, &value));
  m_curState.modules[1].values[2] += m_config.modules[1].m_alpha * (scale(value) - m_curState.modules[1].values[2]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_3, &value));
  m_curState.modules[1].values[3] += m_config.modules[1].m_alpha * (scale(value) - m_curState.modules[1].values[3]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_1_4, &value));
  m_curState.modules[1].values[4] += m_config.modules[1].m_alpha * (scale(value) - m_curState.modules[1].values[4]);
  
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_0, &value));
  m_curState.modules[2].values[0] += m_config.modules[2].m_alpha * (scale(value) - m_curState.modules[2].values[0]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_1, &value));
  m_curState.modules[2].values[1] += m_config.modules[2].m_alpha * (scale(value) - m_curState.modules[2].values[1]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_2, &value));
  m_curState.modules[2].values[2] += m_config.modules[2].m_alpha * (scale(value) - m_curState.modules[2].values[2]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_3, &value));
  m_curState.modules[2].values[3] += m_config.modules[2].m_alpha * (scale(value) - m_curState.modules[2].values[3]);
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_4, &value));
  m_curState.modules[2].values[4] += m_config.modules[2].m_alpha * (scale(value) - m_curState.modules[2].values[4]);

  m_curState.modules[0].moduleSelect = gpio_get_level(GPIO_DEVICE_1_SELECT);
  m_curState.modules[1].moduleSelect = gpio_get_level(GPIO_DEVICE_2_SELECT);
  m_curState.modules[2].moduleSelect = gpio_get_level(GPIO_DEVICE_3_SELECT);

  if (initial)
  {
    m_lastState = m_curState;
  }
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
}

void MidiManager::handleUpdates()
{
  if (tud_midi_mounted())
  {
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 5; ++j)
      {
        m_curState.modules[i].stale[j] = m_curState.modules[i].values[j] == m_lastState.modules[i].values[j];
        
        // Analog - Action on a value delta, and when change stops
        // Digital - Action of rising and falling edges 
        if ((m_config.modules[i].devices[j].m_device_type == FORCE_ANALOG) || (m_curState.modules[i].moduleSelect && m_config.modules[i].devices[j].m_device_type == HARDWARE_DEFAULT))
        {
          // Analog
          m_curState.modules[i].stale[j] = m_curState.modules[i].values[j] == m_lastState.modules[i].values[j];

          if (!m_curState.modules[i].stale[j])
          {
            // changing value
            // printf("Analog Changing\n");
            sendMidiMsg(i, j, true);
          }

          if (!m_lastState.modules[i].stale[j] && m_curState.modules[i].stale[j])
          {
            // stopped value
            // printf("Analog Stopped\n");
            sendMidiMsg(i, j, false);
          }
        }
        else
        {
          // Digital
          bool curState = m_curState.modules[i].values[j] > 63;
          bool lastState = m_lastState.modules[i].values[j] > 63;
          m_curState.modules[i].stale[j] = curState == lastState;

          if (!m_curState.modules[i].stale[j])
          {
            // Detected an edge
            // printf("Digital %s\n", m_curState.modules[i].values[j] < 64 ? "Rising" : "Falling");
            sendMidiMsg(i, j, m_curState.modules[i].values[j] < 64);
          }
        }
      }
    }

    m_lastState = m_curState;
  }
}

uint8_t MidiManager::scale(int value)
{
  return (value * 127) / 4095;
}

void MidiManager::sendMidiMsg(uint8_t moduleIndex, uint8_t deviceIndex, bool changing)
{
  bool success = true;
  uint8_t msg[3] = {0, 0, 0};
  // Build message based on config
  if (changing && m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_change != 0xFF)
  {
    uint8_t msgId = m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_change;
    msg[0] = msgId | m_config.modules[moduleIndex].m_channel;

    if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_0 != 0xFF)
    {
      if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_0 == 0xFE)
      {
        msg[1] = m_curState.modules[moduleIndex].values[deviceIndex];
      }
      else
      {
        msg[1] = m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_0;
      }
    }
    else
    {
      msg[1] = m_config.modules[moduleIndex].devices[deviceIndex].m_data;
    }

    if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_1 != 0xFF)
    {
      if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_1 == 0xFE)
      {
        msg[2] = m_curState.modules[moduleIndex].values[deviceIndex];
      }
      else
      {
        msg[2] = m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_1;
      }
    }
    else
    {
      if (msgId == 0x80 || msgId == 0x90)
      {
        msg[2] = m_config.modules[moduleIndex].m_press_velocity;
      }
      else if (msgId == 0xB0 || msgId == 0xC0)
      {
        msg[2] = m_curState.modules[moduleIndex].values[deviceIndex];
      }
    }
  }
  else if (!changing && m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_stop != 0xFF)
  {
    uint8_t msgId = m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_stop;
    msg[0] = msgId | m_config.modules[moduleIndex].m_channel;

    if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_0 != 0xFF)
    {
      if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_0 == 0xFE)
      {
        msg[1] = m_curState.modules[moduleIndex].values[moduleIndex];
      }
      else
      {
        msg[1] = m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_0;
      }
    }
    else
    {
      msg[1] = m_config.modules[moduleIndex].devices[deviceIndex].m_data;
    }

    if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_1 != 0xFF)
    {
      if (m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_1 == 0xFE)
      {
        msg[2] = m_curState.modules[moduleIndex].values[deviceIndex];
      }
      else
      {
        msg[2] = m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_1;
      }
    }
    else
    {
      if (msgId == 0x80 || msgId == 0x90)
      {
        msg[2] = m_config.modules[moduleIndex].m_press_velocity;
      }
      else if (msgId == 0xB0 || msgId == 0xC0)
      {
        msg[2] = m_curState.modules[moduleIndex].values[deviceIndex];
      }
    }
  }
  else
  {
    success = false;
  }

  if (success)
  {
    // printf("Sending: %X %X %X\n", msg[0], msg[1], msg[2]);
    tud_midi_stream_write(0, msg, 3);
  }
}