#pragma once
#include <stdlib.h>

// None of these values are greater than 127
// I'm using 255 as unset and 254 as device value

static const char* tinyUsbConfig[5] = 
{
  (char[]){0x09, 0x04},         // 0: English (0x0409)
  "Stephen Dennis",             // 1: Manufacturer
  "MIDI-Mod",                   // 2: Product
  "000001",                     // 3: Serial No.
  "MIDI-Mod Device",            // 4: MIDI/MSC
};

enum DeviceType
{
  HARDWARE_DEFAULT = 0,
  FORCE_ANALOG = 1,
  FORCE_DIGITAL = 2,
};

enum Notes
{
  C = 0,
  C_SHARP = 1,
  D = 2,
  D_SHARP = 3,
  E = 4,
  F = 5,
  F_SHARP = 6,
  G = 7,
  G_SHARP = 8,
  A = 9,
  A_SHARP = 10,
  B = 11,
};

struct Device
{
  DeviceType m_device_type;
  uint8_t m_msg_on_change;
  uint8_t m_msg_on_stop;
  uint8_t m_data;
  uint8_t m_manual_data_change_0;
  uint8_t m_manual_data_change_1;
  uint8_t m_manual_data_stop_0;
  uint8_t m_manual_data_stop_1;

  Device()
  {
    m_device_type = HARDWARE_DEFAULT;
    m_msg_on_change = 0xFF;
    m_msg_on_stop = 0xFF;
    m_data = 0xFF;
    m_manual_data_change_0 = 0xFF;
    m_manual_data_change_1 = 0xFF;
    m_manual_data_stop_0 = 0xFF;
    m_manual_data_stop_1 = 0xFF;
  }
};

struct Module
{
  uint8_t m_channel;
  uint8_t m_press_velocity;
  float m_alpha;
  Device devices[5];

  Module()
  {
    m_channel = 0;
    m_press_velocity = 0x7F;
    m_alpha = 0.3;
  }
};

struct Config
{
  Module modules[3];
};

struct ModuleState
{
  bool moduleSelect;
  uint8_t values[5];
  bool stale[5];
};

struct State
{
  ModuleState modules[3];
};