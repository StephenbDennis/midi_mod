# Modular USB MIDI Controller (Midi-Mod)

A **USB MIDI device** that supports up to **3 hot-swappable input modules**, configurable as **buttons**, **sliders**, or **potentiometers**.  
I was going for a compact design that allowed the user to configure their Midi-Mod device without needing firmware updates.
* Note: This is a work in progress... It's working, but there are many things that could be improved.

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
- message_on_change    : (Optional) note_off,note_on,cc,pc,0x0-0xF : Defaults to noop
  - The message to send when the value changes (analog) or rising edge (digital). This can be a string or a hex value
- message_on_stop      : (Optional) note_off,note_on,cc,pc,0x0-0xF : Defaults to noop
  - The message to send when the value stops changing (analog) or falling edge (digital). This can be a string or a hex value
- data 	               : (Optional) notes,0x0-0x7F                 : Defaults to 0x0. 
  - For messages note_off,note_on,cc,pc this will be the value in the message. This can be a note string or a hex value. Overwritten by Manual Data
- manual_data_change_0 : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the first data byte on a change action to this value. If dev, the value of the device is used 
- manual_data_change_1 : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the second data byte on a change action to this value. If dev, the value of the device is used
- manual_data_stop_0   : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the first data byte on a stop action to this value. If dev, the value of the device is used
- manual_data_stop_1   : (Optional) 0x0-0x7F,dev                   : Not used if not set
  - If set this will set the second data byte on a stop action to this value. If dev, the value of the device is used

### Example
```
[module1]
press_velocity = 0x7F
channel = 0x0
alpha = 0.5
[module1.device1]
message_on_change = note_on
message_on_stop = note_off
data = C_-1
[module1.device2]
message_on_change = note_on
message_on_stop = note_off
data = C_0
[module1.device3]
message_on_change = note_on
message_on_stop = note_off
data = C_1
[module1.device4]
message_on_change = note_on
message_on_stop = note_off
data = C_2
[module1.device5]
message_on_change = note_on
message_on_stop = note_off
data = C_3

[module2]
press_velocity = 0x7F
channel = 0x0
alpha = 0.5
[module2.device1]
message_on_change = cc
data = 0x0
[module2.device2]
message_on_change = cc
data = 0x1
[module2.device3]
message_on_change = cc
data = 0x2
[module2.device4]
message_on_change = cc
data = 0x3
[module2.device5]
message_on_change = cc
data = 0x4

[module3]
press_velocity = 0x7F
channel = 0x0
alpha = 0.5
[module3.device1]
message_on_change = pc
data = 0x0
[module3.device2]
message_on_change = pc
data = 0x1
[module3.device3]
message_on_change = pc
data = 0x2
[module3.device4]
message_on_change = pc
data = 0x3
[module3.device5]
message_on_change = pc
data = 0x4
```