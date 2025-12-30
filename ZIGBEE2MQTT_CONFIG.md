# Zigbee2MQTT Configuration Guide for ESP32-C6 Dual Thermometer

Complete guide for adding support for the ESP32-C6 dual temperature sensor device to Zigbee2MQTT.

## Overview

This ESP32-C6 device implements a Zigbee Router with two DS18B20 temperature sensors on separate endpoints:
- **Endpoint 11**: Temperature sensor 1 (DS18B20 #1)
- **Endpoint 12**: Temperature sensor 2 (DS18B20 #2)
- **Zigbee Model**: `ESP32C6.TH`
- **Manufacturer**: Espressif

---

## Step 1: Pairing the Device with Zigbee2MQTT

### 1.1 Enable Pairing Mode

**Via Home Assistant:**
1. Navigate to **Settings** → **Devices & Services** → **Zigbee2MQTT**
2. Click **Configure**
3. Enable **Permit Join** (allow new devices)

**Via Zigbee2MQTT Web Interface:**
1. Open Zigbee2MQTT frontend (usually `http://homeassistant.local:8099`)
2. Click **Permit Join** button at the top

**Via MQTT:**
```bash
mosquitto_pub -h localhost -t 'zigbee2mqtt/bridge/request/permit_join' -m '{"value": true}'
```

### 1.2 Reset and Pair ESP32-C6

1. Connect ESP32-C6 to power via USB-C
2. Open serial monitor (115200 baud) to observe pairing process
3. Press **RESET button** on ESP32-C6 (or power cycle)
4. Device will automatically attempt to join the Zigbee network

**Expected serial output:**
```
[Zigbee] Starting network steering
[Zigbee] Network steering started
[Zigbee] Joined network successfully
[Zigbee] Device announced
```

### 1.3 Verify Device in Zigbee2MQTT

In Zigbee2MQTT logs you should see:
```
Zigbee2MQTT:info  Device '0x00124b001234abcd' joined
Zigbee2MQTT:info  Starting interview of '0x00124b001234abcd'
Zigbee2MQTT:warn  Device '0x00124b001234abcd' with Zigbee model 'ESP32C6.TH' and manufacturer name 'Espressif' is NOT supported
Zigbee2MQTT:info  Please follow https://www.zigbee2mqtt.io/advanced/support-new-devices/01_support_new_devices.html
```

The warning message is expected - the device paired successfully but needs an external converter for full support.

---

## Step 2: Adding External Converter Support

Zigbee2MQTT uses external converters to support custom devices. There are **two methods** to add support.

### Method A: Using Frontend (Recommended for Testing)

**Via Zigbee2MQTT Frontend:**

1. Open Zigbee2MQTT web interface
2. Navigate to device → **Settings** → **Dev console** tab
3. You should see exposed clusters and endpoints
4. Click **"Generate external definition"** button
5. Copy the generated code
6. Navigate to **Settings** → **External converters**
7. Click **Add converter**
8. Paste the code (or use the code below)
9. Modify as needed and save

### Method B: Using Configuration File (Recommended for Production)

**For Traditional Zigbee2MQTT Installation:**

1. Locate your Zigbee2MQTT `data` directory (same level as `configuration.yaml`)
2. Create `external_converters` folder if it doesn't exist:
   ```bash
   mkdir -p /opt/zigbee2mqtt/data/external_converters
   ```

3. Create file `esp32c6_thermometer.js` with this content:

```javascript
/**
 * Zigbee2MQTT External Converter for ESP32-C6 Dual Thermometer
 * 
 * Device: ESP32-C6 with dual DS18B20 temperature sensors
 * Endpoints: 11 (sensor1), 12 (sensor2)
 * Manufacturer: Espressif
 * Model: ESP32C6.TH
 */

const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;

const definition = {
    zigbeeModel: ['ESP32C6.TH'],
    model: 'ESP32C6-DUAL-TEMP',
    vendor: 'Espressif',
    description: 'ESP32-C6 Dual DS18B20 Temperature Sensor with Zigbee Router',
    fromZigbee: [fz.temperature],
    toZigbee: [],
    exposes: [
        e.temperature().withEndpoint('sensor1'),
        e.temperature().withEndpoint('sensor2'),
    ],
    endpoint: (device) => {
        return {
            'sensor1': 11,
            'sensor2': 12,
        };
    },
    meta: {
        multiEndpoint: true,
    },
    configure: async (device, coordinatorEndpoint, logger) => {
        const endpoint1 = device.getEndpoint(11);
        const endpoint2 = device.getEndpoint(12);
        
        // Bind temperature measurement cluster for both endpoints
        await reporting.bind(endpoint1, coordinatorEndpoint, ['msTemperatureMeasurement']);
        await reporting.bind(endpoint2, coordinatorEndpoint, ['msTemperatureMeasurement']);
        
        // Configure temperature reporting
        // Min: 10 seconds, Max: 300 seconds (5 min), Change: 100 (1°C)
        await reporting.temperature(endpoint1, {min: 10, max: 300, change: 100});
        await reporting.temperature(endpoint2, {min: 10, max: 300, change: 100});
        
        logger.info('ESP32C6 Dual Thermometer configured successfully');
    },
};

module.exports = definition;
```

4. Update `configuration.yaml`:

```yaml
# Add to your configuration.yaml
external_converters:
  - esp32c6_thermometer.js
```

5. Restart Zigbee2MQTT:
```bash
# Docker
docker restart zigbee2mqtt

# Home Assistant Addon
# Settings → Add-ons → Zigbee2MQTT → Restart

# Systemd
sudo systemctl restart zigbee2mqtt
```

**For Home Assistant OS (Addon):**

1. Navigate to **Settings** → **Add-ons** → **Zigbee2MQTT**
2. Click on **Configuration** tab
3. Enable **Show in sidebar** if not already enabled
4. Go to **Zigbee2MQTT** in sidebar
5. Click hamburger menu (three dots) → **File editor**
6. Create `external_converters` folder
7. Create `esp32c6_thermometer.js` with the code above
8. Edit `/config/zigbee2mqtt/configuration.yaml` and add:
   ```yaml
   external_converters:
     - esp32c6_thermometer.js
   ```
9. Restart the addon

---

## Step 3: Device Configuration and Testing

### 3.1 Verify Device Recognition

After restarting Zigbee2MQTT with the external converter:

1. Navigate to Zigbee2MQTT web interface
2. Go to **Devices** tab
3. Find your ESP32-C6 device (should now show as `ESP32C6-DUAL-TEMP`)
4. Click on the device
5. Check **Exposes** tab - you should see:
   - Temperature (sensor1) - endpoint 11
   - Temperature (sensor2) - endpoint 12

### 3.2 Rename Device (Optional but Recommended)

1. In device details, click **Settings** tab
2. Set **Friendly name**: `esp32c6_thermometer` (or your preferred name)
3. Save changes

### 3.3 Test Temperature Reporting

**Check current temperatures:**
- In Zigbee2MQTT: Device → **Exposes** tab shows current values
- Via MQTT:
  ```bash
  mosquitto_sub -h localhost -t 'zigbee2mqtt/esp32c6_thermometer' -v
  ```

**Test temperature change detection:**
1. Gently heat one sensor (e.g., hold between fingers)
2. Wait 10-60 seconds
3. Temperature should update when change ≥ 1°C
4. Check in Home Assistant or MQTT logs

**Expected MQTT payload:**
```json
{
  "temperature_sensor1": 21.5,
  "temperature_sensor2": 22.3,
  "linkquality": 120
}
```

---

## Step 4: Home Assistant Integration

After successful configuration, temperature sensors will automatically appear in Home Assistant.

### 4.1 Entity Names

Entities will be created as:
- `sensor.esp32c6_thermometer_temperature_sensor1`
- `sensor.esp32c6_thermometer_temperature_sensor2`

### 4.2 Customize Entity Names

**Via Home Assistant UI:**
1. Go to **Settings** → **Devices & Services** → **Entities**
2. Search for `esp32c6_thermometer`
3. Click on entity → Settings icon
4. Change **Name** and **Entity ID** as desired

**Example customization:**
- Sensor 1: `Living Room Temperature`
- Sensor 2: `Kitchen Temperature`

### 4.3 Lovelace Dashboard Card

```yaml
type: entities
title: Temperature Sensors
entities:
  - entity: sensor.esp32c6_thermometer_temperature_sensor1
    name: Living Room
    icon: mdi:thermometer
  - entity: sensor.esp32c6_thermometer_temperature_sensor2
    name: Kitchen
    icon: mdi:thermometer
```

**Or use gauge cards:**
```yaml
type: horizontal-stack
cards:
  - type: gauge
    entity: sensor.esp32c6_thermometer_temperature_sensor1
    name: Sensor 1
    min: 0
    max: 40
    severity:
      green: 18
      yellow: 24
      red: 28
  - type: gauge
    entity: sensor.esp32c6_thermometer_temperature_sensor2
    name: Sensor 2
    min: 0
    max: 40
    severity:
      green: 18
      yellow: 24
      red: 28
```

### 4.4 Example Automations

**Alert on temperature change:**
```yaml
automation:
  - alias: "Temperature Alert"
    trigger:
      - platform: state
        entity_id: 
          - sensor.esp32c6_thermometer_temperature_sensor1
          - sensor.esp32c6_thermometer_temperature_sensor2
    condition:
      - condition: template
        value_template: >
          {{ (trigger.to_state.state | float - trigger.from_state.state | float) | abs >= 2.0 }}
    action:
      - service: notify.mobile_app
        data:
          title: "Temperature Change"
          message: >
            {{ trigger.to_state.attributes.friendly_name }} changed from 
            {{ trigger.from_state.state }}°C to {{ trigger.to_state.state }}°C
```

---

## Troubleshooting

### Device Won't Pair

**Problem:** Device doesn't appear in Zigbee2MQTT after reset

**Solutions:**
1. Verify **Permit Join** is enabled in Zigbee2MQTT
2. Check Zigbee channel compatibility:
   - Default in code: Channel 11
   - Check Z2M channel: `configuration.yaml` → `advanced.channel`
3. Ensure coordinator is in range (< 10m for initial pairing)
4. Check ESP32 serial output for error messages
5. Try factory reset: Hold BOOT button while pressing RESET

### Device Paired but Not Recognized

**Problem:** Device shows as unsupported in Z2M logs

**Solutions:**
1. Verify external converter file exists in correct location
2. Check `configuration.yaml` has correct path to converter
3. Restart Zigbee2MQTT completely
4. Check Z2M logs for JavaScript errors in converter
5. Verify converter syntax (no typos in `zigbeeModel`)

### Temperatures Not Updating

**Problem:** Temperature values are stale or don't change

**Solutions:**
1. Check DS18B20 sensor wiring (see [WIRING.md](WIRING.md))
2. Verify sensors are detected in ESP32 serial output
3. Check reporting configuration in converter (min/max/change values)
4. Monitor MQTT topic directly to see if device is publishing
5. Check if threshold is too high (default 1°C = 100)

### Only One Sensor Visible

**Problem:** Only one temperature sensor appears in Home Assistant

**Solutions:**
1. Verify both DS18B20 sensors are connected and detected (check serial output)
2. Confirm multi-endpoint configuration in converter
3. Check endpoint configuration (11 and 12)
4. Re-interview device: Z2M → Device → **Reconfigure**
5. Remove and re-pair device if issue persists

### MQTT Messages Not Received

**Problem:** No data appears in MQTT or Home Assistant

**Solutions:**
1. Verify MQTT broker is running: `mosquitto_sub -h localhost -t '#' -v`
2. Check Z2M connection to MQTT in logs
3. Verify Home Assistant MQTT integration is configured
4. Check device is still connected: Z2M → Devices → check last seen
5. Restart Home Assistant Core

---

## Advanced Configuration

### Adjusting Report Intervals

Modify the `configure` function in `esp32c6_thermometer.js`:

```javascript
// Current: Min 10s, Max 300s, Change 1°C
await reporting.temperature(endpoint1, {min: 10, max: 300, change: 100});

// More frequent: Min 5s, Max 60s, Change 0.5°C
await reporting.temperature(endpoint1, {min: 5, max: 60, change: 50});

// Less frequent: Min 60s, Max 600s, Change 2°C
await reporting.temperature(endpoint1, {min: 60, max: 600, change: 200});
```

**Note:** Changes require device reconfiguration or re-pairing.

### Monitoring Device Health

```yaml
# Track link quality
sensor:
  - platform: mqtt
    name: "ESP32C6 Link Quality"
    state_topic: "zigbee2mqtt/esp32c6_thermometer"
    value_template: "{{ value_json.linkquality }}"
    unit_of_measurement: "lqi"
```

### Using with Node-RED

Example flow to log temperature changes:

```json
[
    {
        "id": "mqtt-in",
        "type": "mqtt in",
        "topic": "zigbee2mqtt/esp32c6_thermometer",
        "broker": "mqtt-broker",
        "outputs": 1
    },
    {
        "id": "json-parse",
        "type": "json"
    },
    {
        "id": "function-process",
        "type": "function",
        "func": "msg.payload = {\n    sensor1: msg.payload.temperature_sensor1,\n    sensor2: msg.payload.temperature_sensor2,\n    timestamp: new Date()\n};\nreturn msg;"
    }
]
```

---

## Important Notes

1. **Device Type**: ESP32-C6 functions as a **Zigbee Router**, not an end device
   - Always powered (not battery operated)
   - Strengthens Zigbee mesh network
   - Can route messages for other devices

2. **Temperature Reporting**: 
   - Threshold set to 1°C by default (configurable in firmware)
   - Measurement every 5 seconds
   - Report only on significant change to reduce network traffic

3. **Precision**: DS18B20 sensors provide 12-bit resolution (0.0625°C steps)

4. **IEEE Address**: Auto-generated from ESP32-C6 MAC address (unique per device)

5. **Network Security**: Device supports Zigbee 3.0 security (install code not required)

---

## Reference Links

- **Zigbee2MQTT External Converters**: https://www.zigbee2mqtt.io/advanced/more/external_converters.html
- **Support New Devices**: https://www.zigbee2mqtt.io/advanced/support-new-devices/01_support_new_devices.html
- **ESP32-C6 Documentation**: https://www.espressif.com/en/products/socs/esp32-c6
- **Project README**: [README.md](README.md)
- **Hardware Wiring**: [WIRING.md](WIRING.md)

---

**Last Updated**: December 2025  
**Zigbee2MQTT Version**: 1.35.0+  
**Tested with**: Home Assistant 2024.12
