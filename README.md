# Modular USB MIDI Controller (Midi-Mod)

A **USB MIDI device** that supports up to **3 hot-swappable input modules**, configurable as **buttons**, **sliders**, or **potentiometers**.  
I was going for a compact design that allowed the user to configure their Midi-Mod device without needing firmware updates.
* Note: This is a work in progress... It's working, but there are many things that could be improved.

---

## Configure it in a browser

**[stephenbdennis.github.io/midi_mod](https://stephenbdennis.github.io/midi_mod/)**

Pick a message for every input, watch the MIDI bytes it will put on the wire, and download the
`config.toml`. It also imports a config you already have, and can listen to the device over Web MIDI
so you can check what it is actually sending. Everything runs in the browser; nothing is uploaded.

---

### Project Build and Flash
idf.py set-target esp32s3
idf.py build
idf.py -p COM<xxx> flash
idf.py -p COM<xxx> monitor

---

## Features

- **Up to 3 modules active at once**
- Modules can be:
  - Button Module (5 Cherry Mx buttons)
  - Slider Module (2 Linear Potentiometers)
  - Pot Module (5 Knob Potentiometers)
- Modules can be **mixed** (e.g. 3 Button Modules or 1 Button + 1 Slider + 1 Pot)
- **Every MIDI message**: notes, aftertouch, control change, program change, pitch bend, and the
  system common and real time messages
- Configurable with onboard .toml(ish) file
- MSC for configuration changes

---

## Module Types

Each module occupies **one slot**.  
A maximum of **3 slots** are available.

### 1. Button Module
- Defaults to a digital device when connected

### 2. Slider Module
- Defaults to an analog device when connected
- Uses dual sliders, so device 1 and 2 should have the roughly the same value. and well as 3 and 4. Device 5 is tied to ground.

### 3. Potentiometer Module
- Defaults to an analog device when connected

---

## Configuration
- Press and Hold the config button on the bottom of the main board when conecting to power and the device will come up as a MSC device. You can save multiple files, but only config.toml will be used to configure the device when it comes up in the MIDI mode (Default). On first boot you may need to format the device.

- reference.txt Will be generated every time the device comes up in config mode. It contains information about what is expected in the config files.

- If no config.toml is present the device writes the example below, so there is always something to edit.

- A line starting with `#` is a comment. Numbers are hexadecimal.

### Module Options
- press_velocity : (Optional) 0x0-0x7F : Defaults to 0x7F
  - This is the note_on/note_off message velocity. Only used with buttons
- channel 	     : (Optional) 0x0-0xF  : Defaults to 0x0
  - The channel messages are sent on. (Part of the MIDI message)
- alpha	    	 : (Optional) 0.01-0.5 : Defaults to 0.3
  - This value changes the responsivity of the input. Higher is faster, Lower is smoother but slower. Currently buttons are analog reads, so this should be pretty high when using that module
### Device Options
- device_type          : (Optional) analog,digital                 : Defaults to module type
  - Digital: triggers message_on_change on the rising edge and message_on_stop on the falling edge. 
  - Analog:  triggers message_on_change when value is changing and message_on_stop when the value is no longer changing. 
- channel              : (Optional) 0x0-0xF,inherit                : Defaults to inherit
  - Overrides the module channel for this one input. System messages ignore it.
- message_on_change    : (Optional) see Messages below             : Defaults to noop
  - The message to send when the value changes (analog) or rising edge (digital). This can be a name or a status byte
- message_on_stop      : (Optional) see Messages below             : Defaults to noop
  - The message to send when the value stops changing (analog) or falling edge (digital). This can be a name or a status byte
- data 	               : (Optional) notes,0x0-0x7F                 : Defaults to 0x0. 
  - For messages that carry a note, controller or program number this will be the value in the message. This can be a note string or a hex value. Overwritten by Manual Data
- manual_data_change_0 : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the first data byte on a change action to this value. If dev, the value of the device is used 
- manual_data_change_1 : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the second data byte on a change action to this value. If dev, the value of the device is used
- manual_data_stop_0   : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the first data byte on a stop action to this value. If dev, the value of the device is used
- manual_data_stop_1   : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the second data byte on a stop action to this value. If dev, the value of the device is used

### Messages

`input` is the live 0-127 reading of the button, slider or knob. `data` is the device's `data` option.

| Name | Status | Bytes | Data 1 | Data 2 |
| --- | --- | --- | --- | --- |
| `noop` | - | 0 | nothing is sent | |
| `note_off` | 0x80 | 3 | note (data) | velocity (press_velocity) |
| `note_on` | 0x90 | 3 | note (data) | velocity (press_velocity) |
| `poly_aftertouch` | 0xA0 | 3 | note (data) | pressure (input) |
| `cc` | 0xB0 | 3 | controller (data) | value (input) |
| `pc` | 0xC0 | 2 | program (data) | |
| `channel_aftertouch` | 0xD0 | 2 | pressure (input) | |
| `pitch_bend` | 0xE0 | 3 | bend LSB (input) | bend MSB (input) |
| `mtc_quarter_frame` | 0xF1 | 2 | time code (data) | |
| `song_position` | 0xF2 | 3 | position LSB (input) | position MSB (input) |
| `song_select` | 0xF3 | 2 | song (data) | |
| `tune_request` | 0xF6 | 1 | | |
| `clock` | 0xF8 | 1 | | |
| `start` | 0xFA | 1 | | |
| `continue` | 0xFB | 1 | | |
| `stop` | 0xFC | 1 | | |
| `active_sensing` | 0xFE | 1 | | |
| `system_reset` | 0xFF | 1 | | |

- Aliases: `control_change` = `cc`, `program_change` = `pc`, `channel_pressure` = `channel_aftertouch`, `timing_clock` = `clock`
- A raw status byte such as `0x90` works in place of a name
- Messages from 0xF1 up are system messages and ignore the channel
- `pitch_bend` and `song_position` spread the 0-127 reading across their full 14 bit range

### Notes
`C_(-1..9)` `C#_(-1..9)` `D_(-1..9)` `D#_(-1..9)` `E_(-1..9)` `F_(-1..9)` `F#_(-1..9)` `G_(-1..9)` `G#_(-1..8)` `A_(-1..8)` `A#_(-1..8)` `B_(-1..8)`

### Example
```
[module1]
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
```

---

## Repo layout

| Path | What it is |
| --- | --- |
| `main/` | ESP32-S3 firmware |
| `boards/` | KiCad projects for the main board and the three module types |
| `docs/` | The web configurator, published to GitHub Pages |
| `.github/workflows/pages.yml` | Deploys `docs/` on every push to `main` |

To turn the page on, set **Settings → Pages → Source** to **GitHub Actions**. (Deploying from the
`main` branch `/docs` folder works too, and needs no workflow.)
