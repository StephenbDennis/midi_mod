#include "configManager.hpp"

static void _mount(void)
{
  ESP_LOGI("CONFIG", "Mount storage...");
  ESP_ERROR_CHECK(tinyusb_msc_storage_mount(BASE_PATH));

  return;
}

static esp_err_t storage_init_spiflash(wl_handle_t *wl_handle)
{
  const esp_partition_t *data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
  if (data_partition == NULL) {
      ESP_LOGE("CONFIG", "Failed to find FATFS partition. Check the partition table.");
      return ESP_ERR_NOT_FOUND;
  }

  return wl_mount(data_partition, wl_handle);
}

static void storage_mount_changed_cb(tinyusb_msc_event_t *event)
{
  ESP_LOGI("CONFIG", "Storage mounted to application: %s", event->mount_changed_data.is_mounted ? "Yes" : "No");
}

void ConfigManager::init()
{
  ESP_LOGI("CONFIG", "Initializing storage...");

  static wl_handle_t wl_handle = WL_INVALID_HANDLE;
  ESP_ERROR_CHECK(storage_init_spiflash(&wl_handle));

  const esp_vfs_fat_mount_config_t mount_config = {
    .format_if_mount_failed = true,
    .max_files = 5,
    .allocation_unit_size = 4096,
  };

  const tinyusb_msc_spiflash_config_t config_spi = {
    .wl_handle = wl_handle,
    .callback_mount_changed = storage_mount_changed_cb,
    .callback_premount_changed = NULL,
    .mount_config = mount_config,
  };
  ESP_ERROR_CHECK(tinyusb_msc_storage_init_spiflash(&config_spi));
  ESP_ERROR_CHECK(tinyusb_msc_register_callback(TINYUSB_MSC_EVENT_MOUNT_CHANGED, storage_mount_changed_cb)); /* Other way to register the callback i.e. registering using separate API. If the callback had been already registered, it will be overwritten. */

  _mount();
}

Config ConfigManager::getConfig()
{
  parseConfig();
  return m_config;
}

void ConfigManager::openDevice()
{
  // Generate Reference file
  FILE *f = fopen("/data/reference.txt", "w");
  fprintf(f, "--- config.toml Reference Guide ---\n");
  fprintf(f, "\n");
  fprintf(f, "\n");

  fprintf(f, "[module(1-3)] options:\n");
  fprintf(f, "press_velocity : 0x0-0x7F : This is the note_on message press velocity. 			                  : Defaults to 0x7F\n");
  fprintf(f, "alpha	    	   : 0.01-0.5 : This value changes the responsivity of the input. Higher is faster. : Defaults to 0.3\n");
  fprintf(f, "channel 	     : 0x0-0xF  : The channel messages are sent on. (Part of the MIDI messages)       : Defaults to 0x0\n");
  fprintf(f, "[module*.device(1-5)]\n");
  fprintf(f, "  device_type       : analog,digital		       : If set this will read each device as 0,1 for digital or 1-127 for analog         : Defaults to module type\n");
  fprintf(f, "  message_on_change : note_off,note_on,cc,pc,0x0-0xF : The message to send when the value changes (analog) or rising (digital)    : Defaults to noop\n");
  fprintf(f, "  message_on_stop   : note_off,note_on,cc,pc,0x0-0xF : The message to send when the value is stale (analog) or falling (digital)  : Defaults to noop\n");
  fprintf(f, "  data 	      : notes,0x0-0x7F                 : For messages note_off,note_on,cc,pc this will be the value in the message.       : Defaults to 0x0. Overwritten by Manual Options\n");
  fprintf(f, "  --- Manual Options ---\n");
  fprintf(f, "  manual_data_change_0 : 0x0-0x7F,dev  : If set this will set the first data byte on a change action to this value. If dev, the value of the device is used  : Not used if not set \n");
  fprintf(f, "  manual_data_change_1 : 0x0-0x7F,dev  : If set this will set the second data byte on a change action to this value. If dev, the value of the device is used : Not used if not set\n");
  fprintf(f, "  manual_data_stop_0   : 0x0-0x7F,dev  : If set this will set the first data byte on a stop action to this value. If dev, the value of the device is used    : Not used if not set\n");
  fprintf(f, "  manual_data_stop_1   : 0x0-0x7F,dev  : If set this will set the second data byte on a stop action to this value. If dev, the value of the device is used   : Not used if not set\n");
  fprintf(f, "\n");
  fprintf(f, "notes:\n");
  fprintf(f, "  C_(-1-9)\n");
  fprintf(f, "  C#_(-1-9)\n");
  fprintf(f, "  D_(-1-9)\n");
  fprintf(f, "  D#_(-1-9)\n");
  fprintf(f, "  E_(-1-9)\n");
  fprintf(f, "  F_(-1-9)\n");
  fprintf(f, "  F#_(-1-9)\n");
  fprintf(f, "  G_(-1-9)\n");
  fprintf(f, "  G#_(-1-8)\n");
  fprintf(f, "  A_(-1-8)\n");
  fprintf(f, "  A#_(-1-8)\n");
  fprintf(f, "  B_(-1-8)\n");
  fprintf(f, "\n");
  fprintf(f, "\n");
  fprintf(f, "--- Example ---\n");
  fprintf(f, "\n");
  fprintf(f, "[module1]\n");
  fprintf(f, "press_velocity = 0x7F\n");
  fprintf(f, "channel = 0x0\n");
  fprintf(f, "[module1.device1]\n");
  fprintf(f, "message_on_change = note_on\n");
  fprintf(f, "message_on_stop = note_off\n");
  fprintf(f, "data = C_-1\n");
  fprintf(f, "[module1.device2]\n");
  fprintf(f, "message_on_change = note_on\n");
  fprintf(f, "message_on_stop = note_off\n");
  fprintf(f, "data = C_0\n");
  fprintf(f, "[module1.device3]\n");
  fprintf(f, "message_on_change = note_on\n");
  fprintf(f, "message_on_stop = note_off\n");
  fprintf(f, "data = C_1\n");
  fprintf(f, "[module1.device4]\n");
  fprintf(f, "message_on_change = note_on\n");
  fprintf(f, "message_on_stop = note_off\n");
  fprintf(f, "data = C_2\n");
  fprintf(f, "[module1.device5]\n");
  fprintf(f, "message_on_change = note_on\n");
  fprintf(f, "message_on_stop = note_off\n");
  fprintf(f, "data = C_3\n");
  fprintf(f, "\n");
  fprintf(f, "[module2]\n");
  fprintf(f, "press_velocity = 0x7F\n");
  fprintf(f, "channel = 0x0\n");
  fprintf(f, "[module2.device1]\n");
  fprintf(f, "message_on_change = cc\n");
  fprintf(f, "data = 0x0\n");
  fprintf(f, "[module2.device2]\n");
  fprintf(f, "message_on_change = cc\n");
  fprintf(f, "data = 0x1\n");
  fprintf(f, "[module2.device3]\n");
  fprintf(f, "message_on_change = cc\n");
  fprintf(f, "data = 0x2\n");
  fprintf(f, "[module2.device4]\n");
  fprintf(f, "message_on_change = cc\n");
  fprintf(f, "data = 0x3\n");
  fprintf(f, "[module2.device5]\n");
  fprintf(f, "message_on_change = cc\n");
  fprintf(f, "data = 0x4\n");
  fprintf(f, "\n");
  fprintf(f, "[module3]\n");
  fprintf(f, "press_velocity = 0x7F\n");
  fprintf(f, "channel = 0x0\n");
  fprintf(f, "[module3.device1]\n");
  fprintf(f, "message_on_change = pc\n");
  fprintf(f, "data = 0x0\n");
  fprintf(f, "[module3.device2]\n");
  fprintf(f, "message_on_change = pc\n");
  fprintf(f, "data = 0x1\n");
  fprintf(f, "[module3.device3]\n");
  fprintf(f, "message_on_change = pc\n");
  fprintf(f, "data = 0x2\n");
  fprintf(f, "[module3.device4]\n");
  fprintf(f, "message_on_change = pc\n");
  fprintf(f, "data = 0x3\n");
  fprintf(f, "[module3.device5]\n");
  fprintf(f, "message_on_change = pc\n");
  fprintf(f, "data = 0x4\n");

  fclose(f);

  ESP_LOGI("CONFIG", "USB MSC initialization");
  const tinyusb_config_t tusb_cfg = {
      .device_descriptor = &descriptor_config,
      .string_descriptor = tinyUsbConfig,
      .string_descriptor_count = sizeof(tinyUsbConfig) / sizeof(tinyUsbConfig[0]),
      .external_phy = false,
      .configuration_descriptor = msc_fs_configuration_desc,
  };
  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
  ESP_LOGI("CONFIG", "USB MSC initialization DONE");
}

void ConfigManager::parseConfig()
{
  std::vector<std::string> lines;

  FILE* f = fopen("/data/config.toml", "r");
  if (!f)
  {
    printf("Failed to open file: %s\n", "/data/config.toml");
  }

  char buffer[64];
  while (fgets(buffer, sizeof(buffer), f))
  {
    size_t len = strlen(buffer);
    if (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r'))
    {
      buffer[len-1] = '\0';
      if (len > 1 && buffer[len-2] == '\r') buffer[len-2] = '\0';
    }

    lines.push_back(std::string(buffer));
  }

  fclose(f);

  uint8_t moduleIndex = 0;
  uint8_t deviceIndex = 0;
  for (uint16_t i = 0; i < lines.size(); ++i)
  {
    // cleanup string
    lines[i].erase(std::remove(lines[i].begin(), lines[i].end(), ' '), lines[i].end());
    lines[i].erase(std::remove(lines[i].begin(), lines[i].end(), '\t'), lines[i].end());
    std::vector<std::string> strs = split(lines[i], '.');
    
    if (compareStrings(lines[i], "[module1]"))
    {
      moduleIndex = 0;
    }
    else if (compareStrings(lines[i], "[module2]"))
    {
      moduleIndex = 1;
    }
    else if (compareStrings(lines[i], "[module3]"))
    {
      moduleIndex = 2;
    }
    else if (strs.size() > 1)
    {
      if (compareStrings(strs[1], "device1]"))
      {
        deviceIndex = 0;
      }
      if (compareStrings(strs[1], "device2]"))
      {
        deviceIndex = 1;
      }
      if (compareStrings(strs[1], "device3]"))
      {
        deviceIndex = 2;
      }
      if (compareStrings(strs[1], "device4]"))
      {
        deviceIndex = 3;
      }
      if (compareStrings(strs[1], "device5]"))
      {
        deviceIndex = 4;
      }
    }
    else
    {
      std::vector<std::string> pairs = split(lines[i], '=');
      if (pairs.size() > 1)
      {
        if (compareStrings(pairs[0], "press_velocity"))
        {
          m_config.modules[moduleIndex].m_press_velocity = parseHex(pairs[1]);
        }
        if (compareStrings(pairs[0], "channel"))
        {
          m_config.modules[moduleIndex].m_channel = parseHex(pairs[1]);
        }
        if (compareStrings(pairs[0], "alpha"))
        {
          m_config.modules[moduleIndex].m_alpha = parseFloat(pairs[1], 0.3);
        }
        if (compareStrings(pairs[0], "device_type"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_device_type = parseType(pairs[1]);
        }
        if (compareStrings(pairs[0], "message_on_change"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_change = parseMsg(pairs[1]);
        }
        if (compareStrings(pairs[0], "message_on_stop"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_msg_on_stop = parseMsg(pairs[1]);
        }
        if (compareStrings(pairs[0], "data"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_data = parseNote(pairs[1]);
        }
        if (compareStrings(pairs[0], "manual_data_change_0"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_0 = parseHex(pairs[1]);
        }
        if (compareStrings(pairs[0], "manual_data_change_1"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_change_1 = parseHex(pairs[1]);
        }
        if (compareStrings(pairs[0], "manual_data_stop_0"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_0 = parseHex(pairs[1]);
        }
        if (compareStrings(pairs[0], "manual_data_stop_1"))
        {
          m_config.modules[moduleIndex].devices[deviceIndex].m_manual_data_stop_1 = parseHex(pairs[1]);
        }
      }
    }
  }
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

    ret = (12 * (std::stoi(parts[1]) + 1)) + note;
  }
  else
  {
    ret = parseHex(str);
  }

  return ret;
}

uint8_t ConfigManager::parseHex(std::string str)
{
  return compareStrings(str, "dev") ? 254 : static_cast<uint8_t>(std::stoi(str, nullptr, 16));
}

float ConfigManager::parseFloat(std::string str, float defaultValue)
{
  float ret = std::stof(str);
  
  return ret;
}

uint8_t ConfigManager::parseMsg(std::string str)
{
  uint8_t ret = 0;
  if (compareStrings(str, "note_off"))
  {
    ret = 0x80;
  }
  else if(compareStrings(str, "note_on"))
  {
    ret = 0x90;
  }
  else if (compareStrings(str, "cc"))
  {
    ret = 0xB0;
  }
  else if(compareStrings(str, "pc"))
  {
    ret = 0xC0;
  }
  else
  {
    ret = parseHex(str);
  }

  return ret;
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
