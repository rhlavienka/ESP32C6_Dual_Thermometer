# Home Assistant Integration Examples

Simple examples for using the ESP32-C6 Dual Thermometer in Home Assistant.

## Available Entities

After pairing with Zigbee2MQTT, these entities will be automatically created:

- `sensor.esp32c6_thermometer_temperature_sensor1`
- `sensor.esp32c6_thermometer_temperature_sensor2`

---

## Example 1: Basic Lovelace Card

Display both temperature sensors in a simple card:

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

**Or use gauge cards for visual display:**

```yaml
type: horizontal-stack
cards:
  - type: gauge
    entity: sensor.esp32c6_thermometer_temperature_sensor1
    name: Living Room
    min: 0
    max: 40
    severity:
      green: 18
      yellow: 25
      red: 30
  - type: gauge
    entity: sensor.esp32c6_thermometer_temperature_sensor2
    name: Kitchen
    min: 0
    max: 40
    severity:
      green: 18
      yellow: 25
      red: 30
```

---

## Example 2: Temperature Alert Automation

Send notification when temperature exceeds a threshold:

```yaml
automation:
  - alias: "High Temperature Alert"
    description: "Notify when temperature is too high"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_temperature_sensor1
        above: 28
        for:
          minutes: 5
    action:
      - service: notify.mobile_app
        data:
          title: "🔥 High Temperature Alert"
          message: >
            Living room temperature is {{ states('sensor.esp32c6_thermometer_temperature_sensor1') }}°C
```

---

## Example 3: Temperature Difference Sensor

Create a template sensor to calculate temperature difference between two sensors.

**Add to `configuration.yaml`:**

```yaml
template:
  - sensor:
      - name: "Temperature Difference"
        unique_id: temp_diff_sensor1_sensor2
        unit_of_measurement: "°C"
        device_class: temperature
        state: >
          {% set t1 = states('sensor.esp32c6_thermometer_temperature_sensor1') | float(0) %}
          {% set t2 = states('sensor.esp32c6_thermometer_temperature_sensor2') | float(0) %}
          {{ (t1 - t2) | round(1) }}
        icon: mdi:thermometer-lines
```

**Display in Lovelace:**

```yaml
type: entity
entity: sensor.temperature_difference
name: Temperature Difference
icon: mdi:delta
```

**Use in automation:**

```yaml
automation:
  - alias: "Large Temperature Difference Alert"
    description: "Notify when temperature difference is significant"
    trigger:
      - platform: numeric_state
        entity_id: sensor.temperature_difference
        above: 5
    action:
      - service: notify.mobile_app
        data:
          title: "Temperature Difference Alert"
          message: >
            Temperature difference is {{ states('sensor.temperature_difference') }}°C
```

---

## Additional Resources

- **Full Configuration Guide**: [ZIGBEE2MQTT_CONFIG.md](ZIGBEE2MQTT_CONFIG.md)
- **Hardware Wiring**: [WIRING.md](WIRING.md)
- **Project README**: [README.md](README.md)
- **FAQ**: [FAQ.md](FAQ.md)

---

**Note:** Replace entity IDs with your actual entity names from Home Assistant. Entity naming may vary based on your Zigbee2MQTT configuration.
