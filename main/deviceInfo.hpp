#pragma once
#include <stdlib.h>
#include <stdint.h>

static const char* tinyUsbConfig[5] = 
{
  (char[]){0x09, 0x04},         // 0: English (0x0409)
  "Stephen Dennis",             // 1: Manufacturer
  "MIDI-Mod",                   // 2: Product
  "000001",                     // 3: Serial No.
  "MIDI-Mod Device",            // 4: MIDI/MSC
};

// Sentinels. Real MIDI data bytes never exceed 0x7F, so the top of the range is
// free to mean "not configured". Status bytes go up to 0xFF (system reset), so
// the message fields are widened to 16 bits to keep a sentinel of their own.
static const uint16_t MSG_UNSET        = 0xFFFF; // no message configured -> send nothing
static const uint8_t  DATA_UNSET       = 0xFF;   // option absent from config.toml
static const uint8_t  DATA_FROM_DEVICE = 0xFE;   // substitute the live device value
static const uint8_t  CHANNEL_INHERIT  = 0xFF;   // device follows its module channel

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

// Every MIDI status byte the device can emit. Channel voice messages carry the
// channel in the low nibble; system messages are channel-less and are sent
// exactly as written.
enum MidiMessage : uint16_t
{
  // Channel voice
  MSG_NOTE_OFF            = 0x80,
  MSG_NOTE_ON             = 0x90,
  MSG_POLY_AFTERTOUCH     = 0xA0,
  MSG_CONTROL_CHANGE      = 0xB0,
  MSG_PROGRAM_CHANGE      = 0xC0,
  MSG_CHANNEL_AFTERTOUCH  = 0xD0,
  MSG_PITCH_BEND          = 0xE0,
  // System common
  MSG_MTC_QUARTER_FRAME   = 0xF1,
  MSG_SONG_POSITION       = 0xF2,
  MSG_SONG_SELECT         = 0xF3,
  MSG_TUNE_REQUEST        = 0xF6,
  // System real time
  MSG_TIMING_CLOCK        = 0xF8,
  MSG_START               = 0xFA,
  MSG_CONTINUE            = 0xFB,
  MSG_STOP                = 0xFC,
  MSG_ACTIVE_SENSING      = 0xFE,
  MSG_SYSTEM_RESET        = 0xFF,
};

// System messages (0xF0-0xFF) have no channel nibble.
static inline bool midiIsSystemMessage(uint16_t status)
{
  return status >= 0xF0 && status <= 0xFF;
}

// Sysex (0xF0/0xF7) needs a payload this device has no way to describe, and
// 0xF4/0xF5 are undefined, so they are rejected rather than sent half-formed.
static inline bool midiIsSupportedStatus(uint16_t status)
{
  if (status < 0x80 || status > 0xFF)
  {
    return false;
  }

  switch (status)
  {
    case 0xF0:
    case 0xF4:
    case 0xF5:
    case 0xF7:
      return false;
    default:
      return true;
  }
}

// How many data bytes follow the status byte. Getting this right matters:
// program change and channel aftertouch are two byte messages, and the real
// time messages are a single byte.
static inline uint8_t midiDataByteCount(uint16_t status)
{
  if (midiIsSystemMessage(status))
  {
    switch (status)
    {
      case MSG_MTC_QUARTER_FRAME:
      case MSG_SONG_SELECT:
        return 1;
      case MSG_SONG_POSITION:
        return 2;
      default:
        return 0;
    }
  }

  switch (status & 0xF0)
  {
    case MSG_PROGRAM_CHANGE:
    case MSG_CHANNEL_AFTERTOUCH:
      return 1;
    default:
      return 2;
  }
}

// Stretch a 7 bit device reading over the 14 bit range used by pitch bend and
// song position. 127 * 129 == 16383, so the full range is reachable.
static inline uint16_t midiExpand14(uint8_t value)
{
  return static_cast<uint16_t>(value) * 129u;
}

struct Device
{
  DeviceType m_device_type;
  uint16_t m_msg_on_change;
  uint16_t m_msg_on_stop;
  uint8_t m_channel;
  uint8_t m_data;
  uint8_t m_manual_data_change_0;
  uint8_t m_manual_data_change_1;
  uint8_t m_manual_data_stop_0;
  uint8_t m_manual_data_stop_1;

  Device()
  {
    m_device_type = HARDWARE_DEFAULT;
    m_msg_on_change = MSG_UNSET;
    m_msg_on_stop = MSG_UNSET;
    m_channel = CHANNEL_INHERIT;
    m_data = DATA_UNSET;
    m_manual_data_change_0 = DATA_UNSET;
    m_manual_data_change_1 = DATA_UNSET;
    m_manual_data_stop_0 = DATA_UNSET;
    m_manual_data_stop_1 = DATA_UNSET;
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
