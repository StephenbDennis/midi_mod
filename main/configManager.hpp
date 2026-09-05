#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <errno.h>
#include <dirent.h>
#include <vector>
#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_partition.h"
#include "driver/gpio.h"
#include "deviceInfo.hpp"

#define BASE_PATH "/data" // base path to mount the partition
#define CONFIG_PATH    BASE_PATH "/config.toml"
#define REFERENCE_PATH BASE_PATH "/reference.txt"

// Label of the FAT partition in partitions.csv.
#define STORAGE_LABEL "storage"

class ConfigManager
{
  public:
    void init();
    Config getConfig();

    // Raw file access, for the BLE config service. The device stores the
    // config as the same config.toml text the configurator downloads, so what
    // goes over the air is exactly what a user would save to disk.
    std::string readConfigText();
    bool writeConfigText(const std::string& text);

    // Parse without touching the filesystem, so a config can be validated
    // before it is committed.
    Config parseText(const std::string& text);

  private:
    void writeReference();
    void writeDefaultConfig();
    void applySetting(Config& config, uint8_t moduleIndex, uint8_t deviceIndex, bool inDevice, const std::string& key, const std::string& value);
    int sectionIndex(const std::string& name, const std::string& prefix, int count);
    DeviceType parseType(std::string str);
    uint8_t parseNote(std::string str);
    uint8_t parseHex(std::string str);
    uint8_t parseChannel(std::string str);
    long parseNumber(const std::string& str, int base, long fallback);
    float parseFloat(std::string str, float defaultValue);
    uint16_t parseMsg(std::string str);
    bool compareStrings(std::string str1, std::string str2);
    std::vector<std::string> split(const std::string& s, char delimiter);
};
