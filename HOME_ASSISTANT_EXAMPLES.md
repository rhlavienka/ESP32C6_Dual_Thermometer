# Home Assistant - Príklady automatizácií a konfigurácií

## Automatické entity po pripojení

Po úspešnom pripojení ESP32-C6 do Zigbee2MQTT sa v Home Assistant automaticky vytvoria:

```yaml
sensor.esp32c6_thermometer_sensor1_temperature
sensor.esp32c6_thermometer_sensor2_temperature
```

## Lovelace karty

### 1. Základná karta s teplotami

```yaml
type: entities
title: ESP32-C6 Teplotné senzory
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Senzor 1 (Miestnosť)
    icon: mdi:thermometer
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Senzor 2 (Vonku)
    icon: mdi:thermometer
show_header_toggle: false
```

### 2. Gauge karta (teplomer)

```yaml
type: gauge
entity: sensor.esp32c6_thermometer_sensor1_temperature
name: Teplota v miestnosti
unit: °C
min: 0
max: 40
severity:
  green: 18
  yellow: 25
  red: 30
needle: true
```

### 3. Grafická história

```yaml
type: history-graph
title: Priebeh teplôt (24h)
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Miestnosť
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Vonku
hours_to_show: 24
refresh_interval: 60
```

### 4. Mini Graph Card (vyžaduje HACS plugin)

```yaml
type: custom:mini-graph-card
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Miestnosť
    color: '#e74c3c'
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Vonku
    color: '#3498db'
name: Teploty
hours_to_show: 12
points_per_hour: 4
line_width: 2
show:
  labels: true
  points: false
```

### 5. Rozdiel teplôt (Template sensor)

Vytvorte v `configuration.yaml`:

```yaml
template:
  - sensor:
      - name: "Teplotný rozdiel"
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
name: Rozdiel teplôt (Vnútri - Vonku)
icon: mdi:delta
```

## Automatizácie

### 1. Upozornenie pri vysokej teplote

```yaml
automation:
  - alias: "Upozornenie - Vysoká teplota"
    description: "Odošle notifikáciu pri teplote nad 30°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor1_temperature
        above: 30
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "🔥 Vysoká teplota!"
          message: >
            Teplota v miestnosti dosiahla {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C
          data:
            priority: high
            ttl: 0
```

### 2. Upozornenie pri nízkej teplote

```yaml
automation:
  - alias: "Upozornenie - Nízka teplota"
    description: "Odošle notifikáciu pri teplote pod 15°C"
    trigger:
      - platform: numeric_state
        entity_id: sensor.esp32c6_thermometer_sensor2_temperature
        below: 15
        for:
          minutes: 5
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "❄️ Nízka teplota!"
          message: >
            Vonkajšia teplota klesla na {{ states('sensor.esp32c6_thermometer_sensor2_temperature') }}°C
```

### 3. Zapnutie vykurovania pri nízkej teplote

```yaml
automation:
  - alias: "Automatické vykurovanie"
    description: "Zapne vykurovanie ak teplota klesne pod 19°C"
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
          message: "Vykurovanie zapnuté (teplota: {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C)"
```

### 4. Vetranie pri vysokom teplotnom rozdiele

```yaml
automation:
  - alias: "Odporúčanie vetrania"
    description: "Odporúča vetranie ak je vonku chladnejšie o viac ako 5°C"
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
          title: "💨 Vetranie odporúčané"
          message: >
            Vonku je o {{ (states('sensor.esp32c6_thermometer_sensor1_temperature') | float - 
            states('sensor.esp32c6_thermometer_sensor2_temperature') | float) | round(1) }}°C chladnejšie. 
            Otvorte okná!
```

### 5. Denný report teplôt

```yaml
automation:
  - alias: "Denný teplotný report"
    description: "Odošle denný súhrn teplôt"
    trigger:
      - platform: time
        at: "20:00:00"
    action:
      - service: notify.mobile_app_your_phone
        data:
          title: "📊 Denný teplotný report"
          message: >
            Dnešné teploty:
            
            Miestnosť:
            - Aktuálne: {{ states('sensor.esp32c6_thermometer_sensor1_temperature') }}°C
            - Min: {{ state_attr('sensor.esp32c6_thermometer_sensor1_temperature', 'min_value') }}°C
            - Max: {{ state_attr('sensor.esp32c6_thermometer_sensor1_temperature', 'max_value') }}°C
            
            Vonku:
            - Aktuálne: {{ states('sensor.esp32c6_thermometer_sensor2_temperature') }}°C
            - Min: {{ state_attr('sensor.esp32c6_thermometer_sensor2_temperature', 'min_value') }}°C
            - Max: {{ state_attr('sensor.esp32c6_thermometer_sensor2_temperature', 'max_value') }}°C
```

### 6. Záznam do Google Sheets (vyžaduje Google Sheets integration)

```yaml
automation:
  - alias: "Záznam teplôt do Google Sheets"
    description: "Každú hodinu zaznamená teploty do Google Sheets"
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

## Template senzory (utility)

V `configuration.yaml`:

```yaml
template:
  - sensor:
      # Priemerná teplota z oboch senzorov
      - name: "Priemerná teplota"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ ((sensor1 + sensor2) / 2) | round(1) }}
        icon: mdi:thermometer
        
      # Minimálna teplota
      - name: "Minimálna teplota"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ [sensor1, sensor2] | min | round(1) }}
        icon: mdi:thermometer-chevron-down
        
      # Maximálna teplota
      - name: "Maximálna teplota"
        unit_of_measurement: "°C"
        state: >
          {% set sensor1 = states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) %}
          {% set sensor2 = states('sensor.esp32c6_thermometer_sensor2_temperature') | float(0) %}
          {{ [sensor1, sensor2] | max | round(1) }}
        icon: mdi:thermometer-chevron-up

  - binary_sensor:
      # Detekcia zmrznutia
      - name: "Riziko mrazu"
        state: >
          {{ states('sensor.esp32c6_thermometer_sensor2_temperature') | float(100) < 3 }}
        icon: mdi:snowflake-alert
        device_class: cold
        
      # Vysoká teplota varovanie
      - name: "Vysoká teplota varovanie"
        state: >
          {{ states('sensor.esp32c6_thermometer_sensor1_temperature') | float(0) > 28 }}
        icon: mdi:fire-alert
        device_class: heat
```

## Grafy a štatistiky

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

### ApexCharts Card (vyžaduje HACS)

```yaml
type: custom:apexcharts-card
graph_span: 24h
header:
  show: true
  title: Teploty za posledných 24 hodín
series:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Miestnosť
    stroke_width: 2
    curve: smooth
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Vonku
    stroke_width: 2
    curve: smooth
apex_config:
  chart:
    height: 300px
  yaxis:
    - title:
        text: "Teplota (°C)"
```

## Node-RED integrácia

Ak používate Node-RED, môžete vytvoriť flow na spracovanie údajov:

### Príklad flow:

1. **MQTT In** node
   - Server: mqtt://your_mqtt_server:1883
   - Topic: `zigbee2mqtt/esp32c6_thermometer`

2. **Function** node (spracovanie):
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

3. **Debug/Output** nodes podľa potreby

## InfluxDB a Grafana

Pre pokročilé vizualizácie:

### InfluxDB konfigurácia v HA:

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

## Užitočné scripty

### Script na reset min/max hodnôt (ak používate Statistics)

```yaml
script:
  reset_temperature_stats:
    alias: "Reset teplotných štatistík"
    sequence:
      - service: recorder.purge_entities
        data:
          entity_id:
            - sensor.esp32c6_thermometer_sensor1_temperature
            - sensor.esp32c6_thermometer_sensor2_temperature
          keep_days: 0
      - service: notify.mobile_app_your_phone
        data:
          message: "Teplotné štatistiky boli resetované"
```

## Diagnostika a monitoring

### Sledovanie dostupnosti zariadenia

```yaml
automation:
  - alias: "ESP32-C6 Offline upozornenie"
    description: "Upozorní ak ESP32-C6 prestane komunikovať"
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
          message: "Teplotný senzor ESP32-C6 neodpovedá už 5 minút!"
          data:
            priority: high
```

---

**Tip:** Všetky tieto príklady môžete prispôsobiť vašim potrebám. Nezabudnite nahradiť `your_phone`, entity IDs a ďalšie špecifické hodnoty podľa vášho nastavenia!
