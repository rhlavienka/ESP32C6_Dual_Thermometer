# Zigbee2MQTT konfigurácia pre ESP32-C6 Dual Thermometer

## Automatická detekcia

ESP32-C6 s implementovaným Zigbee stackom by mal byť automaticky rozpoznaný Zigbee2MQTT ako teplotný senzor s dvomi endpointmi.

Po pripojení zariadenia do Zigbee siete by ste mali vidieť v Home Assistant:
- **Sensor 1**: Temperature sensor (endpoint 11)
- **Sensor 2**: Temperature sensor (endpoint 12)

## Postup párovania

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

## Manuálna konfigurácia (ak je potrebná)

Ak by automatická detekcia nefungovala správne, môžete vytvoriť vlastný konvertor.

### Variant 1: External Converter (JavaScript)

Vytvorte súbor: `esp32c6_thermometer.js` v adresári Zigbee2MQTT:

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

### Aktivácia external convertora:

1. Uložte súbor `esp32c6_thermometer.js` do adresára s Zigbee2MQTT konfiguráciou
2. Upravte `configuration.yaml` Zigbee2MQTT:

```yaml
# Zigbee2MQTT configuration.yaml
external_converters:
  - esp32c6_thermometer.js

# Ostatné nastavenia...
advanced:
  log_level: info
  
mqtt:
  base_topic: zigbee2mqtt
  server: mqtt://localhost:1883
```

3. Reštartujte Zigbee2MQTT

### Variant 2: Jednoduchšia konfigurácia (YAML)

Ak používate novšiu verziu Z2M, môžete pridať zariadenie priamo v `devices.yaml`:

```yaml
'0x00124b001234abcd':  # Nahraďte IEEE adresou vášho zariadenia
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
          change: 100  # 1°C = 100 (hodnota * 100)
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

## Home Assistant integrácia

Po úspešnom párovaní sa v Home Assistant automaticky vytvoria entity:

```yaml
# Príklad entít
sensor.esp32c6_thermometer_sensor1_temperature
sensor.esp32c6_thermometer_sensor2_temperature
```

### Príklad automatizácie v Home Assistant:

```yaml
# configuration.yaml alebo automations.yaml
automation:
  - alias: "Upozornenie pri zmene teploty"
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
          title: "Zmena teploty"
          message: >
            Teplota na {{ trigger.to_state.name }} sa zmenila z 
            {{ trigger.from_state.state }}°C na {{ trigger.to_state.state }}°C
```

### Lovelace karta:

```yaml
type: entities
title: ESP32-C6 Teploty
entities:
  - entity: sensor.esp32c6_thermometer_sensor1_temperature
    name: Senzor 1 (DS18B20)
    icon: mdi:thermometer
  - entity: sensor.esp32c6_thermometer_sensor2_temperature
    name: Senzor 2 (DS18B20)
    icon: mdi:thermometer
```

## Overenie funkčnosti

### V Zigbee2MQTT web rozhraní:

1. Prejdite na záložku "Devices"
2. Nájdite ESP32C6 zariadenie
3. Mali by ste vidieť dva endpointy s teplotnými senzormi
4. Kliknite na "Exposes" a overte, že sa zobrazujú teploty

### V logoch:

```
Zigbee2MQTT:info  Device 'esp32c6_thermometer' joined
Zigbee2MQTT:info  Starting interview of 'esp32c6_thermometer'
Zigbee2MQTT:info  Successfully interviewed 'esp32c6_thermometer'
Zigbee2MQTT:info  Device 'esp32c6_thermometer' announced itself
```

### Testovanie zmeny teploty:

1. Zohrievte jeden zo senzorov (napr. prstami)
2. Počkajte, kým sa teplota zmení o viac ako 1°C
3. V Z2M by ste mali vidieť aktualizáciu hodnoty
4. V Home Assistant sa hodnota automaticky aktualizuje

## Riešenie problémov

### Zariadenie sa nepripojí:
1. Overte, že je Z2M v režime "Permit Join"
2. Skontrolujte Zigbee kanál (v ESP32 kóde nastavený PRIMARY_CHANNEL_MASK)
3. Reštartujte ESP32-C6
4. Skontrolujte logy v sériovom monitore

### Teploty sa neaktualizujú:
1. Overte, že binding a reporting sú správne nastavené
2. Skontrolujte threshold (1°C) v kóde
3. Overte funkčnosť senzorov
4. Skontrolujte MQTT spojenie

### Zobrazuje sa iba jeden senzor:
1. Overte multi-endpoint konfiguráciu
2. Skontrolujte, či sú nájdené oba DS18B20 senzory v logoch
3. Overte endpoint konfiguráciu (11 a 12)

## Užitočné príkazy

### Zigbee2MQTT log monitoring:
```bash
# Linux/Docker
docker logs -f zigbee2mqtt

# Home Assistant addon
ha addons logs zigbee2mqtt -f
```

### MQTT monitoring:
```bash
# Subscribe k teplotným topikom
mosquitto_sub -h localhost -t 'zigbee2mqtt/esp32c6_thermometer/#' -v
```

## Poznámky

- ESP32-C6 funguje ako Zigbee router (nie end device), takže posilňuje sieť
- Zmeny teploty sa odosielajú iba ak je rozdiel ≥ 1°C (nastaviteľné v kóde)
- Meranie prebieha každých 5 sekúnd
- IEEE adresa zariadenia je generovaná z ESP32-C6 MAC adresy
