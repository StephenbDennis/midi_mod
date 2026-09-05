#include "configManager.hpp"
#include "esp_vfs_fat.h"
#include "esp_log.h"

void ConfigManager::init()
{
  ESP_LOGI("CONFIG", "Mounting storage...");

  // The FAT partition used to be handed to a USB host as a mass storage
  // device. Nothing outside the firmware touches it now, so it is simply
  // mounted for the application and read over BLE instead.
  static wl_handle_t wl_handle = WL_INVALID_HANDLE;

  esp_vfs_fat_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = true;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 4096;

  ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl(BASE_PATH, STORAGE_LABEL, &mount_config, &wl_handle));

  // Both are written only when absent: nothing reads them off the device now
  // that the USB drive is gone, so rewriting them every boot would be wear for
  // nothing.
  writeReference();
  writeDefaultConfig();
}

// Shipped as the starter config.toml and repeated at the bottom of
// reference.txt so the file always shows a working example.
static const char *EXAMPLE_CONFIG =
R"CFG([module1]
channel = 0x0
press_velocity = 0x7F
alpha = 0.5
[module1.device1]
message_on_change = note_on
message_on_stop = note_off
data = C_3
[module1.device2]
message_on_change = note_on
message_on_stop = note_off
data = D_3
[module1.device3]
message_on_change = note_on
message_on_stop = note_off
data = E_3
[module1.device4]
message_on_change = start
message_on_stop = stop
[module1.device5]
message_on_change = pc
data = 0x0

[module2]
channel = 0x0
alpha = 0.3
[module2.device1]
message_on_change = cc
data = 0x1
[module2.device2]
message_on_change = cc
data = 0x7
[module2.device3]
message_on_change = cc
data = 0xA
[module2.device4]
message_on_change = pitch_bend
[module2.device5]
message_on_change = channel_aftertouch

[module3]
channel = 0x1
alpha = 0.3
[module3.device1]
message_on_change = cc
data = 0x0
[module3.device2]
message_on_change = cc
data = 0x2
[module3.device3]
message_on_change = poly_aftertouch
data = C_3
[module3.device4]
message_on_change = cc
data = 0x4
[module3.device5]
message_on_change = cc
data = 0x5
)CFG";

Config ConfigManager::getConfig()
{
  return parseText(readConfigText());
}

std::string ConfigManager::readConfigText()
{
  FILE *f = fopen(CONFIG_PATH, "rb");
  if (!f)
  {
    ESP_LOGW("CONFIG", "Failed to open %s, running on defaults", CONFIG_PATH);
    return std::string();
  }

  std::string text;
  char buffer[256];
  size_t read = 0;
  while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0)
  {
    text.append(buffer, read);
  }

  fclose(f);
  return text;
}

bool ConfigManager::writeConfigText(const std::string& text)
{
  FILE *f = fopen(CONFIG_PATH, "wb");
  if (!f)
  {
    ESP_LOGE("CONFIG", "Failed to open %s for writing", CONFIG_PATH);
    return false;
  }

  const size_t written = text.empty() ? 0 : fwrite(text.data(), 1, text.size(), f);

  // fclose can still fail to flush, so both halves decide the result.
  const bool closed = fclose(f) == 0;
  if (written != text.size() || !closed)
  {
    ESP_LOGE("CONFIG", "Short write to %s (%u of %u bytes)", CONFIG_PATH,
             static_cast<unsigned>(written), static_cast<unsigned>(text.size()));
    return false;
  }

  ESP_LOGI("CONFIG", "Wrote %u bytes to %s", static_cast<unsigned>(text.size()), CONFIG_PATH);
  return true;
}

void ConfigManager::writeReference()
{
  FILE *f = fopen(REFERENCE_PATH, "r");
  if (f)
  {
    fclose(f);
    return;
  }

  f = fopen(REFERENCE_PATH, "w");
  if (!f)
  {
    ESP_LOGE("CONFIG", "Failed to write %s", REFERENCE_PATH);
    return;
  }

  fputs(
R"REF(--- MIDI-Mod config.toml reference ---

Build this file by hand, or point a browser at the configurator:
  https://stephenbdennis.github.io/midi_mod/

Sections
  [module1] .. [module3]                   one per slot, in board order
  [module1.device1] .. [module1.device5]   the five inputs on that module

A line starting with # is a comment. Numbers are hexadecimal, so 0x7F.
Anything unset falls back to the default listed below.

[module(1-3)] options
  channel        : 0x0-0xF  : channel every device in the module sends on           : default 0x0
  press_velocity : 0x0-0x7F : velocity byte for note_on / note_off                  : default 0x7F
  alpha          : 0.01-0.5 : input responsivity, higher is faster, lower smoother  : default 0.3

[module(1-3).device(1-5)] options
  device_type          : analog, digital     : how the input is read                : default module type
  channel              : 0x0-0xF, inherit    : per device channel override          : default inherit
  message_on_change    : see the table       : sent on change (analog) or rising edge (digital)
  message_on_stop      : see the table       : sent when the value settles (analog) or falling edge (digital)
  data                 : note, 0x0-0x7F      : note, controller or program number   : default 0x0
  manual_data_change_0 : 0x0-0x7F, dev       : override the 1st data byte on change : unset
  manual_data_change_1 : 0x0-0x7F, dev       : override the 2nd data byte on change : unset
  manual_data_stop_0   : 0x0-0x7F, dev       : override the 1st data byte on stop   : unset
  manual_data_stop_1   : 0x0-0x7F, dev       : override the 2nd data byte on stop   : unset

  "dev" substitutes the live 0-127 reading of the input.
  Digital inputs read 127 at rest and 0 when pressed.

Messages
  name                status  bytes  data 1             data 2
  ------------------  ------  -----  -----------------  ----------------
  noop                  -       0    nothing is sent
  note_off             0x80     3    note (data)        velocity
  note_on              0x90     3    note (data)        velocity
  poly_aftertouch      0xA0     3    note (data)        pressure (input)
  cc                   0xB0     3    controller (data)  value (input)
  pc                   0xC0     2    program (data)     -
  channel_aftertouch   0xD0     2    pressure (input)   -
  pitch_bend           0xE0     3    bend LSB (input)   bend MSB (input)
  mtc_quarter_frame    0xF1     2    time code (data)   -
  song_position        0xF2     3    position LSB       position MSB
  song_select          0xF3     2    song (data)        -
  tune_request         0xF6     1    -                  -
  clock                0xF8     1    -                  -
  start                0xFA     1    -                  -
  continue             0xFB     1    -                  -
  stop                 0xFC     1    -                  -
  active_sensing       0xFE     1    -                  -
  system_reset         0xFF     1    -                  -

  Aliases: control_change = cc, program_change = pc,
           channel_pressure = channel_aftertouch, timing_clock = clock.
  A raw status byte (0x80-0xFF) works in place of a name.
  Messages from 0xF1 up are system messages and ignore the channel.
  pitch_bend and song_position spread the 0-127 reading across their
  full 14 bit range.

Notes
  C_(-1..9)  C#_(-1..9)  D_(-1..9)  D#_(-1..9)  E_(-1..9)  F_(-1..9)
  F#_(-1..9)  G_(-1..9)  G#_(-1..8)  A_(-1..8)  A#_(-1..8)  B_(-1..8)

--- Example ---

)REF", f);

  fputs(EXAMPLE_CONFIG, f);
  fclose(f);
}

void ConfigManager::writeDefaultConfig()
{
  FILE *f = fopen(CONFIG_PATH, "r");
  if (f)
  {
    // Never clobber a config the user has already written.
    fclose(f);
    return;
  }

  f = fopen(CONFIG_PATH, "w");
  if (!f)
  {
    ESP_LOGE("CONFIG", "Failed to write %s", CONFIG_PATH);
    return;
  }

  ESP_LOGI("CONFIG", "No config found, writing the example to %s", CONFIG_PATH);
  fputs(EXAMPLE_CONFIG, f);
  fclose(f);
}

Config ConfigManager::parseText(const std::string& text)
{
  Config config{};
  uint8_t moduleIndex = 0;
  uint8_t deviceIndex = 0;
  bool inDevice = false;

  std::istringstream stream(text);
  std::string raw;

  while (std::getline(stream, raw))
  {
    std::string line(raw);

    // Values are unquoted, so whitespace can go. '#' only opens a comment at
    // the start of a line, which keeps note names such as C#_4 intact.
    line.erase(std::remove_if(line.begin(), line.end(),
                              [](unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }),
               line.end());

    if (line.empty() || line[0] == '#')
    {
      continue;
    }

    if (line[0] == '[')
    {
      const size_t close = line.find(']');
      const std::string section = line.substr(1, close == std::string::npos ? std::string::npos : close - 1);
      const std::vector<std::string> parts = split(section, '.');

      const int module = sectionIndex(parts[0], "module", 3);
      if (module >= 0)
      {
        moduleIndex = static_cast<uint8_t>(module);
        deviceIndex = 0;
        inDevice = false;
      }

      if (parts.size() > 1)
      {
        const int device = sectionIndex(parts[1], "device", 5);
        if (device >= 0)
        {
          deviceIndex = static_cast<uint8_t>(device);
          inDevice = true;
        }
      }

      continue;
    }

    // key = value. Split on the first '=' only; alpha values carry a '.' and
    // note names carry a '#', so neither can be used as a separator.
    const size_t equals = line.find('=');
    if (equals == std::string::npos)
    {
      continue;
    }

    applySetting(config, moduleIndex, deviceIndex, inDevice, line.substr(0, equals), line.substr(equals + 1));
  }

  return config;
}

void ConfigManager::applySetting(Config& config, uint8_t moduleIndex, uint8_t deviceIndex, bool inDevice, const std::string& key, const std::string& value)
{
  if (value.empty())
  {
    return;
  }

  Module &module = config.modules[moduleIndex];
  Device &device = module.devices[deviceIndex];
  // channel exists at both levels, so it follows the section we are in.
  if (compareStrings(key, "channel"))
  {
    if (inDevice)
    {
      device.m_channel = parseChannel(value);
    }
    else
    {
      module.m_channel = parseChannel(value) & 0x0F;
    }
  }
  else if (compareStrings(key, "press_velocity"))
  {
    module.m_press_velocity = parseHex(value) & 0x7F;
  }
  else if (compareStrings(key, "alpha"))
  {
    module.m_alpha = parseFloat(value, 0.3);
  }
  else if (compareStrings(key, "device_type"))
  {
    device.m_device_type = parseType(value);
  }
  else if (compareStrings(key, "message_on_change"))
  {
    device.m_msg_on_change = parseMsg(value);
  }
  else if (compareStrings(key, "message_on_stop"))
  {
    device.m_msg_on_stop = parseMsg(value);
  }
  else if (compareStrings(key, "data"))
  {
    device.m_data = parseNote(value);
  }
  else if (compareStrings(key, "manual_data_change_0"))
  {
    device.m_manual_data_change_0 = parseHex(value);
  }
  else if (compareStrings(key, "manual_data_change_1"))
  {
    device.m_manual_data_change_1 = parseHex(value);
  }
  else if (compareStrings(key, "manual_data_stop_0"))
  {
    device.m_manual_data_stop_0 = parseHex(value);
  }
  else if (compareStrings(key, "manual_data_stop_1"))
  {
    device.m_manual_data_stop_1 = parseHex(value);
  }
  else
  {
    ESP_LOGW("CONFIG", "Ignoring unknown option '%s'", key.c_str());
  }
}

// "module2" -> 1, for prefixes numbered 1..count. -1 when it does not match.
int ConfigManager::sectionIndex(const std::string& name, const std::string& prefix, int count)
{
  for (int i = 1; i <= count; ++i)
  {
    if (compareStrings(name, prefix + std::to_string(i)))
    {
      return i - 1;
    }
  }

  return -1;
}

DeviceType ConfigManager::parseType(std::string str)
{
  DeviceType ret;
  if (compareStrings(str, "analog"))
  {
    ret = FORCE_ANALOG;
  }
  else if(compareStrings(str, "digital"))
  {
    ret = FORCE_DIGITAL;
  }
  else
  {
    ret = HARDWARE_DEFAULT;
  }

  return ret;
}

uint8_t ConfigManager::parseNote(std::string str)
{
  uint8_t ret = 0;
  std::vector<std::string> parts = split(str, '_');

  if (parts.size() > 1)
  {
    uint8_t note = 0;
    if (compareStrings(parts[0], "C"))
    {
      note = static_cast<uint8_t>(C);
    }
    if (compareStrings(parts[0], "C#"))
    {
      note = static_cast<uint8_t>(C_SHARP);
    }
    if (compareStrings(parts[0], "D"))
    {
      note = static_cast<uint8_t>(D);
    }
    if (compareStrings(parts[0], "D#"))
    {
      note = static_cast<uint8_t>(D_SHARP);
    }
    if (compareStrings(parts[0], "E"))
    {
      note = static_cast<uint8_t>(E);
    }
    if (compareStrings(parts[0], "F"))
    {
      note = static_cast<uint8_t>(F);
    }
    if (compareStrings(parts[0], "F#"))
    {
      note = static_cast<uint8_t>(F_SHARP);
    }
    if (compareStrings(parts[0], "G"))
    {
      note = static_cast<uint8_t>(G);
    }
    if (compareStrings(parts[0], "G#"))
    {
      note = static_cast<uint8_t>(G_SHARP);
    }
    if (compareStrings(parts[0], "A"))
    {
      note = static_cast<uint8_t>(A);
    }
    if (compareStrings(parts[0], "A#"))
    {
      note = static_cast<uint8_t>(A_SHARP);
    }
    if (compareStrings(parts[0], "B"))
    {
      note = static_cast<uint8_t>(B);
    }

    // Octaves run -1..9, so C_-1 is note 0 and G_9 is note 127.
    const long octave = parseNumber(parts[1], 10, 0);
    const long value = (12 * (octave + 1)) + note;
    ret = value < 0 ? 0 : (value > 127 ? 127 : static_cast<uint8_t>(value));
  }
  else
  {
    ret = parseHex(str);
  }

  return ret;
}

uint8_t ConfigManager::parseHex(std::string str)
{
  if (compareStrings(str, "dev"))
  {
    return DATA_FROM_DEVICE;
  }

  const long value = parseNumber(str, 16, -1);
  if (value < 0 || value > 0xFF)
  {
    ESP_LOGW("CONFIG", "Ignoring out of range value '%s'", str.c_str());
    return DATA_UNSET;
  }

  return static_cast<uint8_t>(value);
}

uint8_t ConfigManager::parseChannel(std::string str)
{
  if (compareStrings(str, "inherit") || compareStrings(str, "module"))
  {
    return CHANNEL_INHERIT;
  }

  const long value = parseNumber(str, 16, -1);
  if (value < 0 || value > 0xF)
  {
    ESP_LOGW("CONFIG", "Ignoring out of range channel '%s'", str.c_str());
    return CHANNEL_INHERIT;
  }

  return static_cast<uint8_t>(value);
}

// strtol rather than stoi: exceptions are off in the default IDF build, so a
// typo in the config would otherwise abort the boot.
long ConfigManager::parseNumber(const std::string& str, int base, long fallback)
{
  if (str.empty())
  {
    return fallback;
  }

  char *end = nullptr;
  errno = 0;
  const long value = strtol(str.c_str(), &end, base);

  // Anything left over means the value was not a clean number.
  if (end == str.c_str() || *end != '\0' || errno == ERANGE)
  {
    return fallback;
  }

  return value;
}

float ConfigManager::parseFloat(std::string str, float defaultValue)
{
  char *end = nullptr;
  const float ret = strtof(str.c_str(), &end);

  if (end == str.c_str() || *end != '\0')
  {
    return defaultValue;
  }

  return ret;
}

uint16_t ConfigManager::parseMsg(std::string str)
{
  static const struct
  {
    const char *name;
    uint16_t status;
  } messages[] =
  {
    {"noop",               MSG_UNSET},
    {"note_off",           MSG_NOTE_OFF},
    {"note_on",            MSG_NOTE_ON},
    {"poly_aftertouch",    MSG_POLY_AFTERTOUCH},
    {"cc",                 MSG_CONTROL_CHANGE},
    {"control_change",     MSG_CONTROL_CHANGE},
    {"pc",                 MSG_PROGRAM_CHANGE},
    {"program_change",     MSG_PROGRAM_CHANGE},
    {"channel_aftertouch", MSG_CHANNEL_AFTERTOUCH},
    {"channel_pressure",   MSG_CHANNEL_AFTERTOUCH},
    {"pitch_bend",         MSG_PITCH_BEND},
    {"mtc_quarter_frame",  MSG_MTC_QUARTER_FRAME},
    {"song_position",      MSG_SONG_POSITION},
    {"song_select",        MSG_SONG_SELECT},
    {"tune_request",       MSG_TUNE_REQUEST},
    {"clock",              MSG_TIMING_CLOCK},
    {"timing_clock",       MSG_TIMING_CLOCK},
    {"start",              MSG_START},
    {"continue",           MSG_CONTINUE},
    {"stop",               MSG_STOP},
    {"active_sensing",     MSG_ACTIVE_SENSING},
    {"system_reset",       MSG_SYSTEM_RESET},
  };

  for (const auto &message : messages)
  {
    if (compareStrings(str, message.name))
    {
      return message.status;
    }
  }

  // Fall back to a raw status byte, e.g. 0x90.
  const long raw = parseNumber(str, 16, -1);
  if (raw < 0 || !midiIsSupportedStatus(static_cast<uint16_t>(raw)))
  {
    ESP_LOGW("CONFIG", "Unknown message '%s', nothing will be sent", str.c_str());
    return MSG_UNSET;
  }

  return static_cast<uint16_t>(raw);
}

bool ConfigManager::compareStrings(std::string str1, std::string str2)
{
  if (str1.length() != str2.length())
  {
    return false;
  }

  for (int i = 0; i < str1.length(); ++i)
  {
    if (tolower(str1[i]) != tolower(str2[i]))
    {
      return false;
    }
  }
  return true;
}

std::vector<std::string> ConfigManager::split(const std::string& s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}
