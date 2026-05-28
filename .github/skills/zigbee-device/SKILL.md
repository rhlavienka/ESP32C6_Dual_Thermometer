---

---
name: zigbee-device
description: Scaffold & maintain ESP-IDF Zigbee end-device (dual DS18B20 -> ZCL 0x0402 on EP1/EP2)
triggers:
  - "init zigbee end device"
  - "add temperature clusters"
  - "enable zcl reporting"
  - "add ds18b20 bus"
steps:
  - Edit: "idf_component.yml"          # ensure esp-zigbee-sdk dependency
  - Edit: "sdkconfig.defaults"         # enable 802.15.4 + Zigbee for ESP32-H2
  - Edit: "main/zb_app.c"              # endpoints, clusters, attributes, callbacks
  - Edit: "main/ds18b20.c"             # 1-Wire readout (skeleton), Celsius->ZCL units
  - Run:  "code --diff before after"   # present changes
notes:
  - Keep IDF pinned to 5.5.1; no coordinator role.

