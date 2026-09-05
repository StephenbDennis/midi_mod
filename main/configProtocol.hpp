#pragma once
#include <stdint.h>
#include <stddef.h>

// Wire protocol for the BLE config service. docs/app.js mirrors every constant
// in this file; change one side and you must change the other.
//
// Service   6d696469-2d6d-6f64-0001-636f6e666967
//   INFO    ...-0002-...  read          device/protocol facts
//   CONTROL ...-0003-...  write, notify commands in, events out
//   DATA    ...-0004-...  write, notify the config.toml bytes, chunked
//
// Read (browser pulls config.toml):
//   -> CMD_READ
//   <- EVT_READ_BEGIN [len16]      then N notifies on DATA      then EVT_READ_END [crc16]
//
// Write (browser pushes config.toml):
//   -> CMD_WRITE [len16]
//   <- EVT_WRITE_READY             then browser writes N chunks to DATA
//   -> CMD_COMMIT [crc16]
//   <- EVT_WRITE_OK  or  EVT_ERR [code]
//
// All multi-byte fields are little endian.

static const uint8_t  CONFIG_PROTO_VERSION = 1;

// The staging buffer the device is willing to hold. The example config is
// around 1 kB, so this leaves generous room without risking the heap.
static const uint16_t CONFIG_MAX_BYTES = 4096;

enum ConfigCommand : uint8_t
{
  CMD_HELLO  = 0x00, // -> EVT_HELLO, carries the negotiated MTU
  CMD_READ   = 0x01,
  CMD_WRITE  = 0x02, // + uint16 length
  CMD_COMMIT = 0x03, // + uint16 crc
  CMD_ABORT  = 0x04,
  CMD_REBOOT = 0x05,
};

enum ConfigEvent : uint8_t
{
  EVT_READ_BEGIN  = 0x81, // + uint16 length
  EVT_READ_END    = 0x82, // + uint16 crc
  EVT_WRITE_READY = 0x83,
  EVT_WRITE_OK    = 0x84,
  EVT_ERR         = 0x85, // + uint8 code
  EVT_HELLO       = 0x86, // + uint8 proto + uint16 mtu + uint16 max bytes
};

enum ConfigError : uint8_t
{
  ERR_TOO_LARGE    = 0x01, // length exceeds CONFIG_MAX_BYTES
  ERR_BAD_CRC      = 0x02, // staged bytes do not match the committed crc
  ERR_NO_STAGE     = 0x03, // commit without a preceding write
  ERR_FS           = 0x04, // the file could not be written
  ERR_LEN_MISMATCH = 0x05, // fewer or more chunk bytes than announced
  ERR_BAD_CMD      = 0x06,
};

// CRC-16/CCITT-FALSE: init 0xFFFF, poly 0x1021, no reflection, no final xor.
// Chosen because it is four lines in both C++ and JavaScript.
static inline uint16_t configCrc16(const uint8_t *data, size_t length)
{
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i)
  {
    crc ^= static_cast<uint16_t>(data[i]) << 8;

    for (int bit = 0; bit < 8; ++bit)
    {
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                           : static_cast<uint16_t>(crc << 1);
    }
  }

  return crc;
}
