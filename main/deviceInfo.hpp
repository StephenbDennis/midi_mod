#pragma once
#include <stdlib.h>

// None of these values are greater than 127
// I'm using 255 as unset and 245 and device value
struct Device
{
  uint8_t m_device_type;
  uint8_t m_msg_on_change;
  uint8_t m_msg_on_stop;
  uint8_t m_data;
  uint8_t m_manual_data_change_0;
  uint8_t m_manual_data_change_1;
  uint8_t m_manual_data_stop_0;
  uint8_t m_manual_data_stop_1;
};

struct Module
{
  uint8_t m_channel;
  uint8_t m_press_velocity;
  Device devices[5];
};

struct Config
{
  Module modules[3];
};

struct ModuleState
{
  bool moduleSelect;
  uint8_t values[5];
};

struct State
{
  ModuleState modules[3];
};