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

// Substitute the configured override for a data byte, if there is one.
static uint8_t resolveDataByte(uint8_t manual, uint8_t fallback, uint8_t value)
{
  if (manual == DATA_UNSET)
  {
    return fallback;
  }

  return (manual == DATA_FROM_DEVICE ? value : manual) & 0x7F;
}

// What a data byte holds when the config does not override it. Index 0 is the
// first byte after the status, index 1 the second.
static uint8_t defaultDataByte(uint16_t status, uint8_t index, const Module &module, const Device &device, uint8_t value)
{
  const uint8_t data = device.m_data == DATA_UNSET ? 0 : device.m_data;
  const uint16_t wide = midiExpand14(value);
  const uint8_t lsb = wide & 0x7F;
  const uint8_t msb = (wide >> 7) & 0x7F;

  if (midiIsSystemMessage(status))
  {
    // Song position is a 14 bit counter; quarter frame and song select take a
    // single configured byte.
    return status == MSG_SONG_POSITION ? (index == 0 ? lsb : msb) : data;
  }

  switch (status & 0xF0)
  {
    case MSG_NOTE_OFF:
    case MSG_NOTE_ON:
      // Note number, then velocity.
      return index == 0 ? data : module.m_press_velocity;
    case MSG_POLY_AFTERTOUCH:
    case MSG_CONTROL_CHANGE:
      // Note / controller number, then the live reading.
      return index == 0 ? data : value;
    case MSG_PROGRAM_CHANGE:
      return data;
    case MSG_CHANNEL_AFTERTOUCH:
      return value;
    case MSG_PITCH_BEND:
      return index == 0 ? lsb : msb;
    default:
      return data;
  }
}

void MidiManager::sendMidiMsg(uint8_t moduleIndex, uint8_t deviceIndex, bool changing)
{
  const Module &module = m_config.modules[moduleIndex];
  const Device &device = module.devices[deviceIndex];

  const uint16_t status = changing ? device.m_msg_on_change : device.m_msg_on_stop;
  if (!midiIsSupportedStatus(status))
  {
    // Nothing configured for this action (MSG_UNSET), or a status this device
    // cannot express.
    return;
  }

  const uint8_t manual_0 = changing ? device.m_manual_data_change_0 : device.m_manual_data_stop_0;
  const uint8_t manual_1 = changing ? device.m_manual_data_change_1 : device.m_manual_data_stop_1;
  const uint8_t value = m_curState.modules[moduleIndex].values[deviceIndex];

  uint8_t msg[3] = {0, 0, 0};
  if (midiIsSystemMessage(status))
  {
    // System messages have no channel nibble.
    msg[0] = static_cast<uint8_t>(status);
  }
  else
  {
    const uint8_t channel = device.m_channel == CHANNEL_INHERIT ? module.m_channel : device.m_channel;
    msg[0] = (static_cast<uint8_t>(status) & 0xF0) | (channel & 0x0F);
  }

  // Length is fixed by the status byte. Sending three bytes for a two byte
  // message leaves a stray data byte on the wire.
  const uint8_t len = 1 + midiDataByteCount(status);
  if (len > 1)
  {
    msg[1] = resolveDataByte(manual_0, defaultDataByte(status, 0, module, device, value), value);
  }
  if (len > 2)
  {
    msg[2] = resolveDataByte(manual_1, defaultDataByte(status, 1, module, device, value), value);
  }

  // printf("Sending: %X %X %X (%d bytes)\n", msg[0], msg[1], msg[2], len);
  tud_midi_stream_write(0, msg, len);
}
