#pragma once
#include "configManager.hpp"
#include "configProtocol.hpp"

// GATT service that hands config.toml to and from the browser configurator.
//
// This replaces the USB mass storage drive the device used to expose in config
// mode: same file, same TOML text, delivered over the air instead. USB is left
// entirely to the MIDI interface, so the device stays plugged into a DAW while
// it is being reconfigured.
class BleConfig
{
  public:
    // Called on a worker task once a new config has been written and parsed.
    // MidiManager uses it to stage the config for the next poll tick, so a
    // sync takes effect without a reboot.
    using ApplyFn = void (*)(const Config&);

    void start(ConfigManager *configManager, ApplyFn onApply);
};
