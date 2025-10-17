
#include <stdlib.h>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "tinyusb.h"

// static const char *TAG = "example";

// --- Pin Mapping ---
// Common
#define GPIO_LAYER_SELECT       GPIO_NUM_38     // GPIO38
#define MAIN_LED                GPIO_NUM_48     // GPIO48

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

static const char* tinyUsbConfig[5] = 
{
  (char[]){0x09, 0x04},         // 0: English (0x0409)
  "Stephen Denis",              // 1: Manufacturer
  "MIDI-MOD Device",            // 2: Product
  "000001",                     // 3: Serial No.
  "A configurable MIDI device", // 4: MIDI
};

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

// LED handle
static led_strip_handle_t led_handle;

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

// emums/structs MOVE
enum DeviceType
{
  KEY =     0,
  ANALOG =  1
};

enum MidiMsgOpcode
{
  NOTE_OFF =        0x80,
  NOTE_ON =         0x90,
  CONTROL_CHANGE =  0xB0
};

enum ControlChangeId
{
  EFFECT_1 =        0x0C,
  EFFECT_2 =         0x0D
};

struct GpioStates
{
  enum DeviceType m_type;
  __uint16_t m_value;
};

// Midi Settings
static uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
static uint8_t const channel = 0;   // 0 for channel 1
static uint8_t const keyVel = 127;  // the key press velocity

// static struct GpioStates prevStates[15];   // The previous gpio states
// static struct GpioStates curStates[15];    // The current gpio states

void updateStates()
{
  // Device 1
  int val_0_0 = 0;
  int val_0_1 = 0;
  int val_0_2 = 0;
  int val_0_3 = 0;
  int val_0_4 = 0;

  // Device 2
  int val_1_0 = 0;
  int val_1_1 = 0;
  int val_1_2 = 0;
  int val_1_3 = 0;
  int val_1_4 = 0;

  // Device 3
  int val_2_0 = 0;
  int val_2_1 = 0;
  int val_2_2 = 0;
  int val_2_3 = 0;
  int val_2_4 = 0;

  // Select
  int device1Select = 0;
  int device2Select = 0;
  int device3Select = 0;
  int layerSelect = 0;

  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_0, &val_0_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_1, &val_0_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_2, &val_0_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_3, &val_0_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_0_4, &val_0_4));

  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_0, &val_1_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_1, &val_1_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_2, &val_1_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_1_handle, ADC_1_3, &val_1_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_1_4, &val_1_4));

  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_0, &val_2_0));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_1, &val_2_1));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_2, &val_2_2));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_3, &val_2_3));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_2_handle, ADC_2_4, &val_2_4));

  device1Select = gpio_get_level(GPIO_DEVICE_1_SELECT);
  device2Select = gpio_get_level(GPIO_DEVICE_2_SELECT);
  device3Select = gpio_get_level(GPIO_DEVICE_3_SELECT);
  layerSelect = gpio_get_level(GPIO_LAYER_SELECT);

  printf("------------------------\n");
  printf("Select:\n");
  printf("    %d\n", device1Select);
  printf("    %d\n", device2Select);
  printf("    %d\n", device3Select);
  printf("    %d\n", layerSelect);
  printf("Device: %d:\n", 1);
  printf("    %d\n", val_0_0);
  printf("    %d\n", val_0_1);
  printf("    %d\n", val_0_2);
  printf("    %d\n", val_0_3);
  printf("    %d\n", val_0_4);
  printf("Device: %d:\n", 2);
  printf("    %d\n", val_1_0);
  printf("    %d\n", val_1_1);
  printf("    %d\n", val_1_2);
  printf("    %d\n", val_1_3);
  printf("    %d\n", val_1_4);
  printf("Device: %d:\n", 3);
  printf("    %d\n", val_2_0);
  printf("    %d\n", val_2_1);
  printf("    %d\n", val_2_2);
  printf("    %d\n", val_2_3);
  printf("    %d\n", val_2_4);
}

void init()
{
  // Setup USB
  tinyusb_config_t const tusb_cfg =
  {
    .device_descriptor = NULL, // If device_descriptor is NULL, tinyusb_driver_install() will use Kconfig
    .string_descriptor = tinyUsbConfig,
    .string_descriptor_count = sizeof(tinyUsbConfig) / sizeof(tinyUsbConfig[0]),
    .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
    .fs_configuration_descriptor = s_midi_cfg_desc,
    .hs_configuration_descriptor = s_midi_hs_cfg_desc,
    .qualifier_descriptor = NULL,
#else
    .configuration_descriptor = s_midi_cfg_desc,
#endif // TUD_OPT_HIGH_SPEED
  };

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  // Setup GPIO. Make sure the channels are correct
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_1_config, &adc_1_handle));
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_2_config, &adc_2_handle));

  adc_oneshot_chan_cfg_t chan_config =
  {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_11  // 3.3V max
  };

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_0_4, &chan_config));

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_1_handle, ADC_1_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_1_4, &chan_config));

  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_0, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_1, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_2, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_2_handle, ADC_2_4, &chan_config));

  gpio_config_t device_1_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_1_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_1_select_conf));

  gpio_config_t device_2_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_2_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_2_select_conf));

  gpio_config_t device_3_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_DEVICE_3_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&device_3_select_conf));

  gpio_config_t layer_select_conf =
  {
    .pin_bit_mask = 1ULL << GPIO_LAYER_SELECT,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_ENABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&layer_select_conf));

  // LED
  led_strip_config_t led_config =
  {
    .strip_gpio_num = MAIN_LED,
    .max_leds = 1,
    .led_pixel_format = LED_PIXEL_FORMAT_GRB,
    .led_model = LED_MODEL_WS2812,
    .flags.invert_out = false,
  };

  led_strip_rmt_config_t rmt_config =
  {
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 10 * 1000 * 1000,  // 10MHz
    .flags.with_dma = false,
  };

  led_strip_new_rmt_device(&led_config, &rmt_config, &led_handle);

  // Init GPIO
  updateStates();
  updateStates();
}

void handleChanges()
{
  if (tud_midi_mounted())
  {
    uint8_t note_on[3] = {NOTE_ON | channel, 1, keyVel};
    tud_midi_stream_write(cable_num, note_on, 3);
    uint8_t note_off[3] = {NOTE_OFF | channel, 1, 0};
    tud_midi_stream_write(cable_num, note_off, 3);
    uint8_t control_1[3] = {CONTROL_CHANGE | channel, EFFECT_1, 20};
    tud_midi_stream_write(cable_num, control_1, 3);
    uint8_t control_2[3] = {CONTROL_CHANGE | channel, EFFECT_2, 40};
    tud_midi_stream_write(cable_num, control_2, 3);
  }
}

static void periodic_midi_write_example_cb(void *arg)
{
  updateStates();
  handleChanges();

  led_strip_set_pixel(led_handle, 0, 0, 0, 255);  // blue
  led_strip_refresh(led_handle);
  // led_strip_clear(strip);  // Turn off
}

void app_main(void)
{
  init();

  // Setup periodic timer
  const esp_timer_create_args_t periodic_midi_args =
  {
    .callback = &periodic_midi_write_example_cb,
    .name = "periodic_midi"
  };

  esp_timer_handle_t periodic_midi_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_midi_args, &periodic_midi_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_midi_timer, 1000000));
}
