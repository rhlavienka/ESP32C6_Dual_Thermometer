# Zigbee2MQTT Configuration for ESP32-C6 Dual Thermometer

## Automatic Detection

The ESP32-C6 with implemented Zigbee stack should be automatically recognized by Zigbee2MQTT as a temperature sensor with two endpoints.

After connecting the device to the Zigbee network, you should see in Home Assistant:
- **Sensor 1**: Temperature sensor (endpoint 11)
- **Sensor 2**: Temperature sensor (endpoint 12)

## Pairing Procedure

1. **Povoľte párovanie v Zigbee2MQTT:**
   - V Home Assistant prejdite na: Configuration → Integrations → Zigbee2MQTT
   - Alebo otvorte Zigbee2MQTT web rozhranie
   - Kliknite na "Permit Join" (povoliť pripojenie)

2. **Resetujte ESP32-C6:**
   - Reštartujte zariadenie
   - Zariadenie sa automaticky pokúsi pripojiť do Zigbee siete

3. **Čakajte na párovanie:**
   - V logoch by ste mali vidieť: "Joined network successfully"
   - V Z2M sa zobrazí nové zariadenie

## Manual Configuration (if needed)

If automatic detection does not work properly, you can create a custom converter.

### Variant 1: External Converter (JavaScript)

Create file: `esp32c6_thermometer.js` in the Zigbee2MQTT directory:

```javascript
const fz = require('zigbee-herdsman-converters/converters/fromZigbee');
const tz = require('zigbee-herdsman-converters/converters/toZigbee');
const exposes = require('zigbee-herdsman-converters/lib/exposes');
const reporting = require('zigbee-herdsman-converters/lib/reporting');
const e = exposes.presets;

const definition = {
    zigbeeModel: ['ESP32C6.TH'],
    model: 'ESP32C6-DUAL-TEMP',
    vendor: 'Espressif',
    description: 'ESP32-C6 Dual DS18B20 Temperature Sensor',
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
        
        await reporting.bind(endpoint1, coordinatorEndpoint, ['msTemperatureMeasurement']);
        await reporting.bind(endpoint2, coordinatorEndpoint, ['msTemperatureMeasurement']);
        
        await reporting.temperature(endpoint1);
        await reporting.temperature(endpoint2);
    },
};

module.exports = definition;
```

### Activating the external converter:

1. Save the file `esp32c6_thermometer.js` to the directory with Zigbee2MQTT configuration
2. Edit `configuration.yaml` in Zigbee2MQTT:

```yaml
# Zigbee2MQTT configuration.yaml
external_converters:
  - esp32c6_thermometer.js

# Other settings...
advanced:
  log_level: info
  
mqtt:
  base_topic: zigbee2mqtt
  server: mqtt://localhost:1883
```

3. Restart Zigbee2MQTT

### Variant 2: Simpler Configuration (YAML)

If you are using a newer version of Z2M, you can add the device directly in `devices.yaml`:

```yaml
'0x00124b001234abcd':  # Replace with your device's IEEE address
  friendly_name: 'esp32c6_thermometer'
  description: 'ESP32-C6 Dual Temperature Sensor'
  endpoints:
    11:
      bindings:
        - cluster: 'msTemperatureMeasurement'
          target:
            type: 'endpoint'
            endpoint: 1
      reporting:
        temperature:
          min: 10
          max: 300
          change: 100  # 1°C = 100 (value * 100)
    12:
      bindings:
        - cluster: 'msTemperatureMeasurement'
          target:
            type: 'endpoint'
            endpoint: 1
      reporting:
        temperature:
          min: 10
          max: 300
          change: 100
```

## Home Assistant Integration

After successful pairing, entities will be automatically created in Home Assistant:

```yaml
# Example entities
sensor.esp32c6_thermometer_sensor1_temperature
sensor.esp32c6_thermometer_sensor2_temperature
```

### Example automation in Home Assistant:

```yaml
# configuration.yaml or automations.yaml
automation:
  - alias: "Temperature change notification"
    trigger:
      - platform: state
        entity_id: 
          - sensor.esp32c6_thermometer_sensor1_temperature
          - sensor.esp32c6_thermometer_sensor2_temperature
    condition:
      - condition: template
        value_template: >
          {{ (trigger.to_state.state | float - trigger.from_state.state | float) | abs >= 1.0 }}
    action:
      - service: notify.mobile_app
        data:
          title: "Temperature Change"
          message: >
            Temperature on {{ trigger.to_state.name }} changed from 
            {{ trigger.from_state.state }}°C to {{ trigger.to_state.state }}°C
```

### Lovelace card:

```yaml
type: entities
title: ESP32-C6 Temperatures
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Sensor 1 (DS18B20)
    icon: mdi:thermometer
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Sensor 2 (DS18B20)
    icon: mdi:thermometer
```

## Functionality Verification

### In Zigbee2MQTT web interface:

1. Go to the "Devices" tab
2. Find the ESP32C6 device
3. You should see two endpoints with temperature sensors
4. Click on "Exposes" and verify that temperatures are displayed

### In logs:

```
Zigbee2MQTT:info  Device 'esp32c6_thermometer' joined
Zigbee2MQTT:info  Starting interview of 'esp32c6_thermometer'
Zigbee2MQTT:info  Successfully interviewed 'esp32c6_thermometer'
Zigbee2MQTT:info  Device 'esp32c6_thermometer' announced itself
```

### Testing temperature change:

1. Heat one of the sensors (e.g. with fingers)
2. Wait until the temperature changes by more than 1°C
3. In Z2M you should see the value update
4. In Home Assistant the value updates automatically

## Troubleshooting

### Device won't connect:
1. Verify that Z2M is in "Permit Join" mode
2. Check the Zigbee channel (set in ESP32 code PRIMARY_CHANNEL_MASK)
3. Restart ESP32-C6
4. Check logs in serial monitor

### Temperatures not updating:
1. Verify that binding and reporting are correctly configured
2. Check threshold (1°C) in code
3. Verify sensor functionality
4. Check MQTT connection

### Only one sensor is displayed:
1. Verify multi-endpoint configuration
2. Check if both DS18B20 sensors are found in logs
3. Verify endpoint configuration (11 and 12)

## Useful Commands

### Zigbee2MQTT log monitoring:
```bash
# Linux/Docker
docker logs -f zigbee2mqtt

# Home Assistant addon
ha addons logs zigbee2mqtt -f
```

### MQTT monitoring:
```bash
# Subscribe to temperature topics
mosquitto_sub -h localhost -t 'zigbee2mqtt/esp32c6_thermometer/#' -v
```

## Notes

- ESP32-C6 funguje ako Zigbee router (nie end device), takže posilňuje sieť
- Zmeny teploty sa odosielajú iba ak je rozdiel ≥ 1°C (nastaviteľné v kóde)
- Meranie prebieha každých 5 sekúnd
- IEEE adresa zariadenia je generovaná z ESP32-C6 MAC adresy
