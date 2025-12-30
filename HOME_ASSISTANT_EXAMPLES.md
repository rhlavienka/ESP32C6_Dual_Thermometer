# Home Assistant - Automation and Configuration Examples

## Automatic Entities After Connection

After successfully connecting ESP32-C6 to Zigbee2MQTT, the following will be automatically created in Home Assistant:

```yaml
sensor.esp32c6_thermometer_sensor1_temperature
sensor.esp32c6_thermometer_sensor2_temperature
```

## Lovelace Cards

### 1. Basic Card with Temperatures

```yaml
type: entities
title: ESP32-C6 Temperature Sensors
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Sensor 1 (Room)
    icon: mdi:thermometer
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Sensor 2 (Outside)
    icon: mdi:thermometer
show_header_toggle: false
```

### 2. Gauge Card (Thermometer)

```yaml
type: gauge
entity: sensor.esp32c6_thermometer_sensor1_temperature
name: Room Temperature
unit: °C
min: 0
max: 40
severity:
  green: 18
  yellow: 25
  red: 30
needle: true
```

### 3. History Graph

```yaml
type: history-graph
title: Temperature History (24h)
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Room
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Outside
hours_to_show: 24
refresh_interval: 60
```

### 4. Mini Graph Card (requires HACS plugin)

```yaml
type: custom:mini-graph-card
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Room
    color: '#e74c3c'
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Outside
    color: '#3498db'
name: Temperatures
hours_to_show: 12
points_per_hour: 4
line_width: 2
show:
  labels: true
  points: false
```

### 5. Temperature Difference (Template sensor)

Create in `configuration.yaml`:

```yaml
template:
  - sensor:
      - name: "Temperature Difference"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ (sensor1 - sensor2) | round(1) }}
        icon: mdi:thermometer-lines
```

Potom v Lovelace:

```yaml
type: entity
entity: sensor.teplotny_rozdiel
name: Temperature Difference (Inside - Outside)
icon: mdi:delta
```

## Automations

### 1. High Temperature Notification

```yaml
automation:
  - alias: "High Temperature Warning"
    description: "Sends notification when temperature exceeds 30°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor1_temperature
        above: 30
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "🔥 High Temperature!"
          message: >
            Room temperature reached {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C
          data:
            priority: high
            ttl: 0
```

### 2. Low Temperature Notification

```yaml
automation:
  - alias: "Low Temperature Warning"
    description: "Sends notification when temperature drops below 15°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor2_temperature
        below: 15
        for:
          minutes: 5
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "❄️ Low Temperature!"
          message: >
            Outside temperature dropped to {{ states('sensor.esp32c6_thermometer_sensor2_temperature') }}°C
```

### 3. Auto Heating When Low Temperature

```yaml
automation:
  - alias: "Automatic Heating"
    description: "Turns on heating if temperature drops below 19°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor1_temperature
        below: 19
        for:
          minutes: 10
    condition:
      - condition: time
        after: "06:00:00"
        before: "22:00:00"
    action:
      - service: climate.set_temperature
        target:
          entity_id: climate.thermostat
        data:
          temperature: 21
      - service: notify.mobile_app_your_phone
        data:
          message: "Heating turned on (temperature: {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C)"
```

### 4. Ventilation with High Temperature Difference

```yaml
automation:
  - alias: "Ventilation Recommendation"
    description: "Recommends ventilation if outside is cooler by more than 5°C"
    trigger:
      - platform: template
        value_template: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ (sensor1 - sensor2) > 5 }}
    condition:
      - condition: time
        after: "08:00:00"
        before: "20:00:00"
      - condition: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor1_temperature
        above: 24
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "💨 Ventilation Recommended"
          message: >
            Outside is {{ (states('sensor.esp32c6_thermometer_sensor1_temperature') | float - 
            states('sensor.esp32c6_thermometer_sensor2_temperature') | float) | round(1) }}°C cooler. 
            Open windows!
```

### 5. Daily Temperature Report

```yaml
automation:
  - alias: "Daily Temperature Report"
    description: "Sends daily temperature summary"
    trigger:
      - platform: time
        at: "20:00:00"
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "📊 Daily Temperature Report"
          message: >
            Today's temperatures:
            
            Room:
            - Current: {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C
            - Min: {{ state_attr('sensor.esp32c6_thermometer_sensor1_temperature', 'min_value') }}°C
            - Max: {{ state_attr('sensor.esp32c6_thermometer_sensor1_temperature', 'max_value') }}°C
            
            Outside:
            - Current: {{ states('sensor.esp32c6_thermometer_sensor2_temperature') }}°C
            - Min: {{ state_attr('sensor.esp32c6_thermometer_sensor2_temperature', 'min_value') }}°C
            - Max: {{ state_attr('sensor.esp32c6_thermometer_sensor2_temperature', 'max_value') }}°C
```

### 6. Log to Google Sheets (requires Google Sheets integration)

```yaml
automation:
  - alias: "Log Temperatures to Google Sheets"
    description: "Logs temperatures to Google Sheets every hour"
    trigger:
      - platform: time_pattern
        minutes: 0
    action:
      - service: google_sheets.append_sheet
        data:
          worksheet_id: "your_worksheet_id"
          data:
            timestamp: "{{ now().strftime('%Y-%m-%d %H:%M:%S') }}"
            sensor1_temp: "{{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}"
            sensor2_temp: "{{ states('sensor.esp32c6_thermometer_sensor2_temperature') }}"
            difference: "{{ (states('sensor.esp32c6_thermometer_sensor1_temperature') | float - states('sensor.esp32c6_thermometer_sensor2_temperature') | float) | round(1) }}"
```

## Template Sensors (utility)

In `configuration.yaml`:

```yaml
template:
  - sensor:
      # Average temperature from both sensors
      - name: "Average Temperature"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ ((sensor1 + sensor2) / 2) | round(1) }}
        icon: mdi:thermometer
        
      # Minimum temperature
      - name: "Minimum Temperature"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ [sensor1, sensor2] | min | round(1) }}
        icon: mdi:thermometer-chevron-down
        
      # Maximum temperature
      - name: "Maximum Temperature"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ [sensor1, sensor2] | max | round(1) }}
        icon: mdi:thermometer-chevron-up

  - binary_sensor:
      # Frost detection
      - name: "Frost Risk"
        state: >
          {{ states('sensor.esp32c6_thermometer_sensor2_temperature') | float(100) < 3 }}
        icon: mdi:snowflake-alert
        device_class: cold
        
      # High temperature warning
      - name: "High Temperature Warning"
        state: >
          {{ states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) > 28 }}
        icon: mdi:fire-alert
        device_class: heat
```

## Graphs and Statistics

### Statistics Card

```yaml
type: statistics-graph
entities:
  - sensor.esp32c6_thermometer_sensor1_temperature
  - sensor.esp32c6_thermometer_sensor2_temperature
stat_types:
  - mean
  - min
  - max
period:
  calendar:
    period: day
days_to_show: 7
```

### ApexCharts Card (requires HACS)

```yaml
type: custom:apexcharts-card
graph_span: 24h
header:
  show: true
  title: Temperatures for Last 24 Hours
series:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Room
    stroke_width: 2
    curve: smooth
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Outside
    stroke_width: 2
    curve: smooth
apex_config:
  chart:
    height: 300px
  yaxis:
    - title:
        text: "Temperature (°C)"
```

## Node-RED Integration

If you use Node-RED, you can create a flow to process data:

### Example flow:

1. **MQTT In** node
   - Server: mqtt://your_mqtt_server:1883
   - Topic: `zigbee2mqtt/esp32c6_thermometer`

2. **Function** node (processing):
   ```javascript
   const temp1 = msg.payload.temperature_sensor1;
   const temp2 = msg.payload.temperature_sensor2;
   
   msg.payload = {
       sensor1: temp1,
       sensor2: temp2,
       average: (temp1 + temp2) / 2,
       difference: Math.abs(temp1 - temp2)
   };
   
   return msg;
   ```

3. **Debug/Output** nodes as needed

## InfluxDB and Grafana

For advanced visualizations:

### InfluxDB configuration in HA:

```yaml
influxdb:
  host: localhost
  port: 8086
  database: homeassistant
  username: !secret influxdb_username
  password: !secret influxdb_password
  max_retries: 3
  default_measurement: state
  include:
    entities:
      - sensor.esp32c6_thermometer_sensor1_temperature
      - sensor.esp32c6_thermometer_sensor2_temperature
```

### Grafana Dashboard query:

```sql
SELECT mean("value") 
FROM "°C" 
WHERE ("entity_id" = 'esp32c6_thermometer_sensor1_temperature') 
AND $timeFilter 
GROUP BY time(5m) fill(linear)
```

## Useful Scripts

### Script to reset min/max values (if using Statistics)

```yaml
script:
  reset_temperature_stats:
    alias: "Reset Temperature Statistics"
    sequence:
      - service: recorder.purge_entities
        data:
          entity_id:
            - sensor.esp32c6_thermometer_sensor1_temperature
            - sensor.esp32c6_thermometer_sensor2_temperature
          keep_days: 0
      - service: notify.mobile_app_your_phone
        data:
          message: "Temperature statistics have been reset"
```

## Diagnostics and Monitoring

### Monitoring device availability

```yaml
automation:
  - alias: "ESP32-C6 Offline Notification"
    description: "Notifies if ESP32-C6 stops communicating"
    trigger:
      - platform: state
        entity_id: sensor.esp32c6_thermometer_sensor1_temperature
        to: 'unavailable'
        for:
          minutes: 5
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "⚠️ ESP32-C6 Offline"
          message: "ESP32-C6 temperature sensor hasn't responded for 5 minutes!"
          data:
            priority: high
```

---

**Tip:** You can customize all these examples to your needs. Don't forget to replace `your_phone`, entity IDs, and other specific values according to your setup!
