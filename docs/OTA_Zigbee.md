# OTA over Zigbee (Zigbee2MQTT local OTA) for esp32h2_thermometer

This document describes how to prepare and deliver OTA firmware updates to this ESP321C Zigbee thermometer using Zigbee2MQTT's local OTA feature (local index and firmware files). It covers both the coordinator-side steps (Zigbee2MQTT) and the device-side requirements (partition layout and OTA client behavior). It also lists a simpler alternative (HTTPS OTA) if you prefer to avoid full ZCL OTA implementation initially.

## Summary
- Zigbee2MQTT can act as an Image Provider by hosting OTA binaries and an `index.json` file. Coordinators then deliver the image to devices over the ZCL OTA cluster (Image Block transfers).
- Your device must implement a ZCL OTA client (Query Next Image, Image Block handling, Upgrade End) and persist received bytes into an alternate `ota` partition using the ESP32 `esp_ota_*` APIs.
- The firmware file placed in Zigbee2MQTT must be a proper OTA image that the device expects (metadata: manufacturer ID, image type, file version, file size, and the raw payload). The exact header format depends on your OTA client implementation.

## Coordinator (Zigbee2MQTT) — Local OTA index

Zigbee2MQTT local OTA expects firmware files and an `index.json` describing them. See:
- https://www.zigbee2mqtt.io/guide/usage/ota_updates.html#local-ota-index-and-firmware-files

Basic steps for Zigbee2MQTT:

1. Place firmware files into the Zigbee2MQTT `data/firmware` folder. Organize by vendor and model if you want.
   - Example path: `data/firmware/my_vendor/esp32h2_thermo_1.2.0.bin`

2. Edit `data/firmware/index.json` and add an entry for the new firmware. A minimal entry looks like:

```
{
  "manufacturer": "MyVendor",
  "models": ["esp32h2_thermometer"],
  "file": "my_vendor/esp32h2_thermo_1.2.0.bin",
  "meta": {
    "version": 120,
    "size": 1536000,
    "imageType": 2,
    "manufacturerId": 0x1234
  }
}
```

Notes:
- `meta.version` is an integer file version — choose a scheme (e.g., `1.2.0 -> 10200` or `120` as above). Keep consistent with device expectations.
- `size` must be the image size in bytes.
- `imageType` and `manufacturerId` are used by ZCL OTA to match devices and must match values used by the device's OTA client.

3. Restart Zigbee2MQTT (or reload firmware list) and use Zigbee2MQTT UI or MQTT commands to trigger OTA for the device.

Important: Zigbee2MQTT's local OTA functionality only *hosts* the file and exposes it through the coordinator's Image Provider logic. The device must request and accept blocks using the ZCL OTA protocol.

## Device-side requirements (this project)

To accept Zigbee OTA images you must implement the following on the ESP321C firmware:

1. Partition table with two OTA slots

For this project (4 MB flash, current single `factory` app at `0x10000` of size `0x1C0000`) a realistic OTA-ready layout reuses the same overall app region and Zigbee partitions, but splits the app into two OTA slots and adds `ota_data`.

Proposed layout for a future OTA-ready release (for example 1.3.0):

```
# name,      type, subType, offset,   size,    flags
nvs,         data, nvs,     0x09000,  0x06000,
phy_init,    data, phy,     0x0F000,  0x01000,
ota_0,       app,  ota_0,   0x10000,  0xE0000,
ota_1,       app,  ota_1,   0xF0000,  0xE0000,
zb_storage,  data, fat,     0x1D0000, 0x20000,
zb_fct,      data, fat,     0x1F0000, 0x01000,
ota_data,    data, ota,     0x1F1000, 0x02000,
```

Notes:
- `nvs`, `phy_init`, `zb_storage` and `zb_fct` keep their current offsets/sizes so Zigbee NVS behaviour stays the same after a clean reflash.
- The previous single `factory` region `[0x10000 .. 0x1D0000)` is now split into two equal OTA app slots (`ota_0` and `ota_1`, each `0xE0000` bytes).
- `ota_data` is placed in the remaining free space below `0x200000` and is used by the ESP-IDF OTA subsystem to track the current active slot.
- After switching to this layout, devices must be reflashed once via serial with an OTA-ready image. Subsequent updates can then be delivered via OTA into the inactive `ota_X` slot.

2. Implement a ZCL OTA client module

Create a `main/zigbee_ota.c` that implements the ZCL OTA client behavior:
- Handle `Query Next Image` responses and start a download if the server reports an available image that matches `manufacturerId`/`imageType`/version.
- Process `Image Block Response` frames, buffer blocks, and write incoming bytes using `esp_ota_begin()`, `esp_ota_write()` and `esp_ota_end()` into the selected `ota_X` partition.
- On successful image completion, perform validation (CRC/signature) and call `esp_ota_set_boot_partition()` to mark the new partition as bootable, then restart.
- Support resume/retries: store current progress to `ota_data` so interrupted downloads can resume from the last accepted block.

Implementation notes:
- Keep Zigbee stack lock time minimal: parse ZCL frames and queue payloads for processing on a worker task that performs `esp_ota_write()`.
- Decide on header format: the Zigbee OTA image must contain any metadata your client requires (e.g., esp image header). You may choose to embed the raw `esp` OTA binary as the payload; the client must be able to detect the firmware image size and layout.
- If you don't implement signature verification on device, at least check CRC and image size before `esp_ota_set_boot_partition()`.

3. Match metadata between coordinator and device

Make sure the `manufacturerId`, `imageType` and `version` you put into `index.json` match what the device's OTA client expects when it issues `Query Next Image`.

4. Resource constraints

- Zigbee OTA sends the image in small blocks — download time can be long. Ensure your device stays powered and remains joined during OTA. Router devices (mains powered) are a good fit.
- Implement progress logging so you can debug failed updates.

## Alternative: Zigbee-triggered HTTPS OTA (simpler to implement)

If implementing the full ZCL OTA client is too heavy, consider a hybrid approach:

- Use a Zigbee command (custom cluster command or specific attribute write) to tell the device to fetch an HTTPS URL.
- Host signed OTA images on an HTTPS server (S3, local web server) and implement an `esp_https_ota()` client on the device to download and apply the update.

Advantages:
- You avoid the complexity of implementing ZCL Image Block protocol and the need to create ZCL OTA image headers.
- Device logic becomes: receive Zigbee command with URL/version → enable Wi‑Fi (if RF switch or antenna control needed) → run `esp_https_ota()` → set boot partition and restart.

Notes for this project:
- Because this board also uses the RF switch pins to enable Zigbee, you must correctly toggle Wi‑Fi RF pins before enabling Wi‑Fi.
- This approach requires the device to be able to temporarily activate Wi‑Fi and reach your OTA server.

## CI / Packaging

1. Build the firmware using `idf.py build`.
2. Produce the OTA payload. For Zigbee OTA this usually means packaging the raw firmware into the format expected by your Zigbee OTA client (may include header with manufacturerId/imageType/version). For HTTPS OTA this is just the signed `.bin`.
3. Optionally sign the image with `espsecure.py` and include signature verification in the device OTA client.
4. Upload the file to your Zigbee2MQTT `data/firmware` folder (or to your HTTPS server) and update `index.json`.
5. Trigger OTA via Zigbee2MQTT UI or MQTT command.

## Testing checklist

- Test on a single test device first.
- Test successful update end‑to‑end.
- Test interruption (power loss) and verify resume/rollback behavior.
- Verify logs and increase verbosity during development.

## Example index.json entry for Zigbee2MQTT

Add to `data/firmware/index.json` (list element):

```
{
  "manufacturer": "MyVendor",
  "models": ["esp32h2_thermometer"],
  "file": "my_vendor/esp32h2_thermo_1.2.0.zigbee.bin",
  "meta": {
    "version": 10200,
    "size": 1536000,
    "imageType": 2,
    "manufacturerId": 4660
+  }
+}
+```
+
+`manufacturerId` can be a private value you choose if you control both sides — use the same value in the device OTA client.
+
+## Next steps I can do for you
+
+- A: Add a recommended `partitions.csv` change and commit it to the repo (so builds create two OTA slots).
+- B: Add a starter `main/zigbee_ota.c` with a stubbed ZCL OTA client state machine and `esp_ota_*` calls (skeleton to iterate from).
+- C: Add `docs/OTA_Zigbee.md` to the repo (this file) and expand it with example code snippets tailored to this project's endpoints & IDs.
+
+Tell me which of A/B/C you want next and I will implement it.
