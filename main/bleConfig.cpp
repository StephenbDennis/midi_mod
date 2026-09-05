#include "bleConfig.hpp"

#include <string.h>
#include <string>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_att.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/util/util.h"

static const char *TAG = "BLE-CFG";
static const char *DEVICE_NAME = "MIDI-Mod";

// 6d696469-2d6d-6f64-000n-636f6e666967 ("midi-mod" / "config").
// BLE_UUID128_INIT takes the bytes little endian, so these read backwards.
#define MIDIMOD_UUID128(nibble) \
  BLE_UUID128_INIT(0x67, 0x69, 0x66, 0x6e, 0x6f, 0x63, (nibble), 0x00, \
                   0x64, 0x6f, 0x6d, 0x2d, 0x69, 0x64, 0x69, 0x6d)

static const ble_uuid128_t s_svcUuid     = MIDIMOD_UUID128(0x01);
static const ble_uuid128_t s_infoUuid    = MIDIMOD_UUID128(0x02);
static const ble_uuid128_t s_controlUuid = MIDIMOD_UUID128(0x03);
static const ble_uuid128_t s_dataUuid    = MIDIMOD_UUID128(0x04);

static uint16_t s_controlHandle = 0;
static uint16_t s_dataHandle = 0;

static uint16_t s_connHandle = BLE_HS_CONN_HANDLE_NONE;
static bool s_controlSubscribed = false;
static bool s_dataSubscribed = false;
static uint8_t s_ownAddrType = 0;

static ConfigManager *s_configManager = nullptr;
static BleConfig::ApplyFn s_onApply = nullptr;

// Bytes the browser has pushed but not yet committed. Sized once at CMD_WRITE
// so a truncated transfer can never be mistaken for a complete one.
static std::string s_staging;
static uint16_t s_expectedLength = 0;
static bool s_writeOpen = false;

// Work that must not run on the NimBLE host task: streaming the config out
// takes many notifications, and committing it touches the filesystem.
enum WorkOp : uint8_t
{
  WORK_READ = 1,
  WORK_COMMIT = 2,
  WORK_REBOOT = 3,
};

struct WorkItem
{
  uint8_t op;
  uint16_t arg;
};

static QueueHandle_t s_workQueue = nullptr;

static void advertise();

/* ------------------------------------------------------------ notifications */

// ble_gatts_notify_custom takes ownership of the mbuf even when it fails, so a
// retry has to allocate a fresh one. Running out of mbufs is expected under a
// burst of chunks and just means "wait a tick".
static bool notifyBytes(uint16_t handle, const uint8_t *data, uint16_t length)
{
  if (s_connHandle == BLE_HS_CONN_HANDLE_NONE)
  {
    return false;
  }

  // A peer that has not enabled notifications on this characteristic will
  // reject every attempt, so there is no point burning the retry budget.
  const bool subscribed = handle == s_controlHandle ? s_controlSubscribed : s_dataSubscribed;
  if (!subscribed)
  {
    ESP_LOGW(TAG, "handle %u is not subscribed", handle);
    return false;
  }

  for (int attempt = 0; attempt < 50; ++attempt)
  {
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, length);
    if (om)
    {
      const int rc = ble_gatts_notify_custom(s_connHandle, handle, om);
      if (rc == 0)
      {
        return true;
      }

      if (rc != BLE_HS_ENOMEM && rc != BLE_HS_EAGAIN)
      {
        ESP_LOGW(TAG, "notify failed on handle %u; rc=%d", handle, rc);
        return false;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  ESP_LOGW(TAG, "notify gave up on handle %u", handle);
  return false;
}

static void sendEvent(uint8_t event, const uint8_t *payload, uint8_t payloadLength)
{
  uint8_t frame[8];
  frame[0] = event;
  if (payloadLength > 0)
  {
    memcpy(&frame[1], payload, payloadLength);
  }

  notifyBytes(s_controlHandle, frame, 1 + payloadLength);
}

static void sendError(uint8_t code)
{
  ESP_LOGW(TAG, "protocol error 0x%02X", code);
  sendEvent(EVT_ERR, &code, 1);
}

static void sendU16Event(uint8_t event, uint16_t value)
{
  const uint8_t payload[2] = { static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>(value >> 8) };
  sendEvent(event, payload, sizeof(payload));
}

/* -------------------------------------------------------------- worker task */

static void streamConfig()
{
  const std::string text = s_configManager->readConfigText();
  const uint16_t length = static_cast<uint16_t>(text.size());
  const uint16_t crc = configCrc16(reinterpret_cast<const uint8_t *>(text.data()), length);

  sendU16Event(EVT_READ_BEGIN, length);

  // Three bytes of every packet go to the ATT notification header.
  uint16_t chunk = ble_att_mtu(s_connHandle);
  chunk = chunk > 3 ? static_cast<uint16_t>(chunk - 3) : 20;

  for (uint16_t offset = 0; offset < length; offset += chunk)
  {
    const uint16_t remaining = static_cast<uint16_t>(length - offset);
    const uint16_t size = remaining < chunk ? remaining : chunk;

    if (!notifyBytes(s_dataHandle, reinterpret_cast<const uint8_t *>(text.data()) + offset, size))
    {
      ESP_LOGW(TAG, "read aborted at offset %u", offset);
      return;
    }
  }

  sendU16Event(EVT_READ_END, crc);
  ESP_LOGI(TAG, "sent %u bytes of config", length);
}

static void commitConfig(uint16_t expectedCrc)
{
  if (!s_writeOpen)
  {
    sendError(ERR_NO_STAGE);
    return;
  }

  if (s_staging.size() != s_expectedLength)
  {
    ESP_LOGW(TAG, "staged %u bytes, expected %u",
             static_cast<unsigned>(s_staging.size()), s_expectedLength);
    sendError(ERR_LEN_MISMATCH);
    return;
  }

  const uint16_t crc = configCrc16(reinterpret_cast<const uint8_t *>(s_staging.data()),
                                   static_cast<uint16_t>(s_staging.size()));
  if (crc != expectedCrc)
  {
    ESP_LOGW(TAG, "crc mismatch: got 0x%04X, expected 0x%04X", crc, expectedCrc);
    sendError(ERR_BAD_CRC);
    return;
  }

  if (!s_configManager->writeConfigText(s_staging))
  {
    sendError(ERR_FS);
    return;
  }

  // Parse from the staged text rather than re-reading, so the config that
  // takes effect is exactly the one whose crc was just checked.
  const Config config = s_configManager->parseText(s_staging);

  s_staging.clear();
  s_staging.shrink_to_fit();
  s_writeOpen = false;
  s_expectedLength = 0;

  if (s_onApply)
  {
    s_onApply(config);
  }

  sendEvent(EVT_WRITE_OK, nullptr, 0);
  ESP_LOGI(TAG, "config committed and applied");
}

static void workTask(void *arg)
{
  WorkItem item{};

  while (true)
  {
    if (xQueueReceive(s_workQueue, &item, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    switch (item.op)
    {
      case WORK_READ:
        streamConfig();
        break;
      case WORK_COMMIT:
        commitConfig(item.arg);
        break;
      case WORK_REBOOT:
        ESP_LOGI(TAG, "rebooting on request");
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        break;
      default:
        break;
    }
  }
}

static void queueWork(uint8_t op, uint16_t arg)
{
  const WorkItem item = { op, arg };
  if (xQueueSend(s_workQueue, &item, 0) != pdTRUE)
  {
    ESP_LOGW(TAG, "work queue full, dropping op %u", op);
  }
}

/* ------------------------------------------------------------- gatt access */

static int handleControlWrite(struct os_mbuf *om)
{
  uint8_t buffer[8];
  uint16_t length = 0;

  const int rc = ble_hs_mbuf_to_flat(om, buffer, sizeof(buffer), &length);
  if (rc != 0 || length < 1)
  {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  // Arguments are little endian uint16s where present.
  const uint16_t arg = length >= 3 ? static_cast<uint16_t>(buffer[1] | (buffer[2] << 8)) : 0;

  switch (buffer[0])
  {
    case CMD_HELLO:
    {
      const uint16_t mtu = ble_att_mtu(s_connHandle);
      const uint8_t payload[5] =
      {
        CONFIG_PROTO_VERSION,
        static_cast<uint8_t>(mtu & 0xFF), static_cast<uint8_t>(mtu >> 8),
        static_cast<uint8_t>(CONFIG_MAX_BYTES & 0xFF), static_cast<uint8_t>(CONFIG_MAX_BYTES >> 8),
      };
      sendEvent(EVT_HELLO, payload, sizeof(payload));
      break;
    }

    case CMD_READ:
      queueWork(WORK_READ, 0);
      break;

    case CMD_WRITE:
      if (length < 3)
      {
        sendError(ERR_BAD_CMD);
        break;
      }

      if (arg > CONFIG_MAX_BYTES)
      {
        sendError(ERR_TOO_LARGE);
        break;
      }

      s_staging.clear();
      s_staging.reserve(arg);
      s_expectedLength = arg;
      s_writeOpen = true;
      sendEvent(EVT_WRITE_READY, nullptr, 0);
      break;

    case CMD_COMMIT:
      if (length < 3)
      {
        sendError(ERR_BAD_CMD);
        break;
      }

      queueWork(WORK_COMMIT, arg);
      break;

    case CMD_ABORT:
      s_staging.clear();
      s_staging.shrink_to_fit();
      s_expectedLength = 0;
      s_writeOpen = false;
      sendEvent(EVT_WRITE_READY, nullptr, 0);
      break;

    case CMD_REBOOT:
      queueWork(WORK_REBOOT, 0);
      break;

    default:
      sendError(ERR_BAD_CMD);
      break;
  }

  return 0;
}

static int handleDataWrite(struct os_mbuf *om)
{
  if (!s_writeOpen)
  {
    sendError(ERR_NO_STAGE);
    return 0;
  }

  const uint16_t available = OS_MBUF_PKTLEN(om);

  // Refuse the chunk that would overrun rather than truncating it, so the
  // length check at commit still means something.
  if (s_staging.size() + available > s_expectedLength)
  {
    ESP_LOGW(TAG, "chunk overruns the announced length");
    s_writeOpen = false;
    sendError(ERR_LEN_MISMATCH);
    return 0;
  }

  uint8_t buffer[512];
  uint16_t copied = 0;

  if (available > sizeof(buffer) ||
      ble_hs_mbuf_to_flat(om, buffer, sizeof(buffer), &copied) != 0)
  {
    s_writeOpen = false;
    sendError(ERR_LEN_MISMATCH);
    return 0;
  }

  s_staging.append(reinterpret_cast<const char *>(buffer), copied);
  return 0;
}

static int accessCb(uint16_t conn_handle, uint16_t attr_handle,
                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
  switch (ctxt->op)
  {
    case BLE_GATT_ACCESS_OP_READ_CHR:
    {
      // Only INFO is readable. Everything else moves over notifications.
      const uint8_t info[5] =
      {
        CONFIG_PROTO_VERSION,
        3, // modules
        5, // devices per module
        static_cast<uint8_t>(CONFIG_MAX_BYTES & 0xFF), static_cast<uint8_t>(CONFIG_MAX_BYTES >> 8),
      };
      return os_mbuf_append(ctxt->om, info, sizeof(info)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
      if (attr_handle == s_controlHandle)
      {
        return handleControlWrite(ctxt->om);
      }

      if (attr_handle == s_dataHandle)
      {
        return handleDataWrite(ctxt->om);
      }

      return BLE_ATT_ERR_UNLIKELY;

    default:
      return BLE_ATT_ERR_UNLIKELY;
  }
}

static const struct ble_gatt_svc_def s_services[] =
{
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &s_svcUuid.u,
    .characteristics = (struct ble_gatt_chr_def[])
    {
      {
        .uuid = &s_infoUuid.u,
        .access_cb = accessCb,
        .flags = BLE_GATT_CHR_F_READ,
      },
      {
        .uuid = &s_controlUuid.u,
        .access_cb = accessCb,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &s_controlHandle,
      },
      {
        .uuid = &s_dataUuid.u,
        .access_cb = accessCb,
        .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &s_dataHandle,
      },
      {},
    },
  },
  {},
};

/* --------------------------------------------------------------- gap / adv */

static void resetTransfer()
{
  s_staging.clear();
  s_staging.shrink_to_fit();
  s_expectedLength = 0;
  s_writeOpen = false;
  s_controlSubscribed = false;
  s_dataSubscribed = false;
}

static int gapEvent(struct ble_gap_event *event, void *arg)
{
  switch (event->type)
  {
    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0)
      {
        s_connHandle = event->connect.conn_handle;
        ESP_LOGI(TAG, "connected; conn_handle=%u", s_connHandle);
      }
      else
      {
        ESP_LOGW(TAG, "connect failed; status=%d", event->connect.status);
        advertise();
      }
      return 0;

    case BLE_GAP_EVENT_DISCONNECT:
      ESP_LOGI(TAG, "disconnected; reason=%d", event->disconnect.reason);
      s_connHandle = BLE_HS_CONN_HANDLE_NONE;
      resetTransfer();
      advertise();
      return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
      if (event->subscribe.attr_handle == s_controlHandle)
      {
        s_controlSubscribed = event->subscribe.cur_notify;
      }
      else if (event->subscribe.attr_handle == s_dataHandle)
      {
        s_dataSubscribed = event->subscribe.cur_notify;
      }
      return 0;

    case BLE_GAP_EVENT_MTU:
      ESP_LOGI(TAG, "mtu now %u", event->mtu.value);
      return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
      advertise();
      return 0;

    default:
      return 0;
  }
}

static void advertise()
{
  // The 128 bit service UUID alone is 18 of the 31 advertising bytes, so the
  // name goes in the scan response. Chrome needs the UUID in the advertisement
  // to match a requestDevice service filter.
  struct ble_hs_adv_fields fields;
  memset(&fields, 0, sizeof(fields));
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.uuids128 = const_cast<ble_uuid128_t *>(&s_svcUuid);
  fields.num_uuids128 = 1;
  fields.uuids128_is_complete = 1;

  int rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0)
  {
    ESP_LOGE(TAG, "adv_set_fields failed; rc=%d", rc);
    return;
  }

  struct ble_hs_adv_fields rsp;
  memset(&rsp, 0, sizeof(rsp));
  rsp.name = reinterpret_cast<uint8_t *>(const_cast<char *>(ble_svc_gap_device_name()));
  rsp.name_len = strlen(ble_svc_gap_device_name());
  rsp.name_is_complete = 1;

  rc = ble_gap_adv_rsp_set_fields(&rsp);
  if (rc != 0)
  {
    ESP_LOGE(TAG, "adv_rsp_set_fields failed; rc=%d", rc);
    return;
  }

  struct ble_gap_adv_params params;
  memset(&params, 0, sizeof(params));
  params.conn_mode = BLE_GAP_CONN_MODE_UND;
  params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  rc = ble_gap_adv_start(s_ownAddrType, NULL, BLE_HS_FOREVER, &params, gapEvent, NULL);
  if (rc != 0)
  {
    ESP_LOGE(TAG, "adv_start failed; rc=%d", rc);
    return;
  }

  ESP_LOGI(TAG, "advertising as %s", DEVICE_NAME);
}

static void onSync()
{
  ESP_ERROR_CHECK(ble_hs_util_ensure_addr(0));

  const int rc = ble_hs_id_infer_auto(0, &s_ownAddrType);
  if (rc != 0)
  {
    ESP_LOGE(TAG, "ble_hs_id_infer_auto failed; rc=%d", rc);
    return;
  }

  advertise();
}

static void onReset(int reason)
{
  ESP_LOGE(TAG, "host reset; reason=%d", reason);
}

static void hostTask(void *param)
{
  nimble_port_run();
  nimble_port_freertos_deinit();
}

/* -------------------------------------------------------------------- start */

void BleConfig::start(ConfigManager *configManager, ApplyFn onApply)
{
  s_configManager = configManager;
  s_onApply = onApply;

  // NimBLE keeps its host state in NVS.
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  s_workQueue = xQueueCreate(4, sizeof(WorkItem));
  ESP_ERROR_CHECK(s_workQueue ? ESP_OK : ESP_ERR_NO_MEM);

  // Streaming the config and writing it to FAT must not run on the host task,
  // so both are handed to this one.
  xTaskCreate(workTask, "ble_cfg_work", 4096, NULL, 5, NULL);

  ESP_ERROR_CHECK(nimble_port_init());

  ble_hs_cfg.sync_cb = onSync;
  ble_hs_cfg.reset_cb = onReset;

  ble_svc_gap_init();
  ble_svc_gatt_init();

  ESP_ERROR_CHECK(ble_gatts_count_cfg(s_services) == 0 ? ESP_OK : ESP_FAIL);
  ESP_ERROR_CHECK(ble_gatts_add_svcs(s_services) == 0 ? ESP_OK : ESP_FAIL);
  ESP_ERROR_CHECK(ble_svc_gap_device_name_set(DEVICE_NAME) == 0 ? ESP_OK : ESP_FAIL);

  nimble_port_freertos_init(hostTask);
}
