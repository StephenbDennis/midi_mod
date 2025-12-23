#include <stdlib.h>
#include <vector>
#include <unordered_map>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "tinyusb.h"
#include "deviceInfo.hpp"

// --- Pin Mapping ---
// Common
#define MAIN_LED                GPIO_NUM_38     // GPIO38

// Device 1
#define ADC_0_0                 ADC_CHANNEL_0   // GPIO1
#define ADC_0_1                 ADC_CHANNEL_1   // GPIO2
#define ADC_0_2                 ADC_CHANNEL_3   // GPIO4
#define ADC_0_3                 ADC_CHANNEL_4   // GPIO5
#define ADC_0_4                 ADC_CHANNEL_5   // GPIO6
#define GPIO_DEVICE_1_SELECT    GPIO_NUM_40     // GPIO40

// Device 2
#define ADC_1_0                 ADC_CHANNEL_6   // GPIO7
#define ADC_1_1                 ADC_CHANNEL_7   // GPIO8
#define ADC_1_2                 ADC_CHANNEL_8   // GPIO9
#define ADC_1_3                 ADC_CHANNEL_9   // GPIO10
#define ADC_1_4                 ADC_CHANNEL_0   // GPIO11
#define GPIO_DEVICE_2_SELECT    GPIO_NUM_41     // GPIO41

// Device 3
#define ADC_2_0                 ADC_CHANNEL_1   // GPIO12
#define ADC_2_1                 ADC_CHANNEL_2   // GPIO13
#define ADC_2_2                 ADC_CHANNEL_3   // GPIO14
#define ADC_2_3                 ADC_CHANNEL_6   // GPIO17
#define ADC_2_4                 ADC_CHANNEL_7   // GPIO18
#define GPIO_DEVICE_3_SELECT    GPIO_NUM_42     // GPIO42

// TinyUSB Settings
#define TUSB_DESCRIPTOR_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_MIDI * TUD_MIDI_DESC_LEN)

// ADC handles
static adc_oneshot_unit_handle_t adc_1_handle;
static adc_oneshot_unit_handle_t adc_2_handle;

// ADC config
static adc_oneshot_unit_init_cfg_t init_1_config = 
{
  .unit_id = ADC_UNIT_1,
  .ulp_mode = ADC_ULP_MODE_DISABLE
};
static adc_oneshot_unit_init_cfg_t init_2_config = 
{
  .unit_id = ADC_UNIT_2,
  .ulp_mode = ADC_ULP_MODE_DISABLE
};

enum interface_count
{
#if CFG_TUD_MIDI
  ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING,
#endif
  ITF_COUNT
};

enum usb_endpoints
{
  EP_EMPTY = 0,
#if CFG_TUD_MIDI
  EPNUM_MIDI,
#endif
};

static const uint8_t s_midi_cfg_desc[] =
{
  // Configuration number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 100),

  // Interface number, string index, EP Out & EP In address, EP size
  TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 64),
};

#if (TUD_OPT_HIGH_SPEED)
static const uint8_t s_midi_hs_cfg_desc[] =
{
  // Configuration number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, TUSB_DESCRIPTOR_TOTAL_LEN, 0, 100),

  // Interface number, string index, EP Out & EP In address, EP size
  TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 4, EPNUM_MIDI, (0x80 | EPNUM_MIDI), 512),
};
#endif // TUD_OPT_HIGH_SPEED

class MidiManager
{
  public:
    void updateStates(bool initial);
    void init(Config config);
    void handleUpdates();
    uint8_t scale(int value);
    void sendMidiMsg(uint8_t moduleIndex, uint8_t deviceIndex, bool changing);

    Config m_config{};
    State m_lastState{};
    State m_curState{};
};