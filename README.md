# ESP32-C6 Zigbee Dual Thermometer

Projekt pre **Seeed Studio XIAO ESP32-C6** na meranie teploty pomocou dvoch **DS18B20** senzorov a odosielanie dát cez **Zigbee** do **Home Assistant** pomocou **Zigbee2MQTT**.

## 📋 Vlastnosti

- ✅ **Dual DS18B20 senzory** na jednej OneWire zbernici (GPIO5)
- ✅ **Automatická detekcia** DS18B20 senzorov a ich ROM adries
- ✅ **Zigbee Router** funkcia (posilňuje Zigbee sieť)
- ✅ **Inteligentné reportovanie** - posiela údaje len pri zmene teploty o ≥1°C
- ✅ **Home Assistant integrácia** cez Zigbee2MQTT
- ✅ **Dva nezávislé endpointy** v Zigbee (každý senzor samostatne)
- ✅ **Nízka spotreba** energie
- ✅ **Kompletná ESP-IDF implementácia** (C/C++)

## 🔧 Hardvér

### Potrebné komponenty:
- 1× **Seeed Studio XIAO ESP32-C6**
- 2× **DS18B20 digitálny teplotný senzor**
- 1× **Odpor 4.7kΩ** (pull-up pre OneWire)
- Vodiče a breadboard
- USB-C kábel (s dátovými vodičmi)

### Zapojenie:

```
ESP32-C6 (XIAO)          DS18B20 #1          DS18B20 #2
─────────────────────────────────────────────────────────
3.3V ----------------+--- VDD -----------+--- VDD
                     |                   |
                   [4.7kΩ]               |
                     |                   |
GPIO5 ---------------+--- DATA ----------+--- DATA
                     
GND -----------------+--- GND -----------+--- GND
```

**Pinout XIAO ESP32-C6:**
- **GPIO5** = D4 pin na XIAO karte
- **3.3V** = 3V3 pin
- **GND** = GND pin

## 📂 Štruktúra projektu

```
C6_Thermometer/
├── main/
│   ├── main.c              # Hlavný program (Zigbee + DS18B20)
│   ├── onewire_bus.c       # OneWire driver
│   ├── onewire_bus.h
│   ├── ds18b20.c           # DS18B20 driver
│   ├── ds18b20.h
│   └── CMakeLists.txt      # Build konfigurácia
├── CMakeLists.txt          # Root CMake
├── partitions.csv          # Partition table pre Zigbee
├── sdkconfig.defaults      # ESP-IDF konfigurácia
├── INSTALL.md              # Inštalačný návod
├── DS18B20_ADDRESS_DETECTION.md  # Návod na detekciu senzorov
├── ZIGBEE2MQTT_CONFIG.md   # Konfigurácia Z2M
└── README.md               # Tento súbor
```

## 🚀 Rýchly štart

### 1. Príprava vývojového prostredia

Postupujte podľa **[INSTALL.md](INSTALL.md)** pre podrobný návod na inštaláciu:
- ESP-IDF (v5.3 alebo novší)
- Visual Studio Code
- ESP-IDF VS Code extension

### 2. Klonovanie/otvorenie projektu

```powershell
# Otvorte VS Code
# File → Open Folder → vyberte priečinok C6_Thermometer
```

### 3. Nastavenie targetu

```powershell
# V VS Code: Ctrl+Shift+P
ESP-IDF: Set Espressif Device Target → esp32c6
```

### 4. Pripojenie hardvéru

1. Zapojte DS18B20 senzory podľa schémy vyššie
2. Pripojte XIAO ESP32-C6 cez USB-C k PC

### 5. Build a Flash

```powershell
# V VS Code: Ctrl+E D
# Alebo:
idf.py build flash monitor
```

### 6. Sledovanie výstupu

Po nahratí otvorte sériový monitor (115200 baud) a uvidíte:

```
I (xxx) ZIGBEE_THERMO: ESP32-C6 Zigbee Thermometer Starting...
I (xxx) ZIGBEE_THERMO: OneWire bus initialized on GPIO5
I (xxx) ZIGBEE_THERMO: Scanning for DS18B20 sensors...
I (xxx) ZIGBEE_THERMO: Found device 1 - ROM: 28 AA BB CC DD EE FF 00
I (xxx) ZIGBEE_THERMO: Sensor 1 assigned
I (xxx) ZIGBEE_THERMO: Found device 2 - ROM: 28 11 22 33 44 55 66 77
I (xxx) ZIGBEE_THERMO: Sensor 2 assigned
I (xxx) ZIGBEE_THERMO: Scan complete. Found 2 DS18B20 sensor(s)
I (xxx) ZIGBEE_THERMO: Initialize Zigbee stack
I (xxx) ZIGBEE_THERMO: Start network steering
I (xxx) ZIGBEE_THERMO: Joined network successfully
I (xxx) DS18B20: Temperature: 23.50°C
```

## 🏠 Integrácia s Home Assistant

### 1. Pripojenie do Zigbee siete

1. Otvorte **Zigbee2MQTT** web rozhranie
2. Povoľte **"Permit Join"**
3. Reštartujte ESP32-C6
4. Počkajte na automatické párovanie

### 2. Konfigurácia

Postupujte podľa **[ZIGBEE2MQTT_CONFIG.md](ZIGBEE2MQTT_CONFIG.md)** pre:
- Automatickú detekciu zariadenia
- Custom konvertor (ak je potrebný)
- Home Assistant entity a automatizácie

### 3. Výsledok v Home Assistant

Po úspešnom párovaní uvidíte:
- `sensor.esp32c6_thermometer_sensor1_temperature`
- `sensor.esp32c6_thermometer_sensor2_temperature`

## 📝 Dôležité informácie

### Zigbee Endpoints:
- **Endpoint 11** = Senzor 1 (prvý nájdený DS18B20)
- **Endpoint 12** = Senzor 2 (druhý nájdený DS18B20)

### Reportovanie teploty:
- **Periódické meranie:** každých 5 sekúnd
- **Odoslanie do Z2M:** iba pri zmene ≥ 1°C
- **Rozlíšenie:** 0.0625°C (12-bit ADC DS18B20)

### GPIO piny:
- **GPIO5** = OneWire zbernica pre DS18B20
- **GPIO14** = Výber antény (LOW = interná, HIGH = externá)
- **GPIO3** = Enable RF switch (musí byť LOW pre použitie antén)

## 🔍 Detekcia DS18B20 senzorov

Ak potrebujete zistiť ROM adresy vašich DS18B20 senzorov, pozrite:
**[DS18B20_ADDRESS_DETECTION.md](DS18B20_ADDRESS_DETECTION.md)**

Program automaticky skenuje a zobrazuje nájdené senzory pri štarte.

## ⚙️ Konfigurácia

### Zmena GPIO pre OneWire:

V súbore `main/main.c`:
```c
#define ONEWIRE_GPIO GPIO_NUM_5  // Zmeňte na požadovaný pin
```

### Zmena threshold pre reportovanie:

```c
#define TEMP_REPORT_THRESHOLD 1.0f  // Zmena v °C
```

### Zmena periódy merania:

V `temperature_sensor_task()`:
```c
vTaskDelay(pdMS_TO_TICKS(5000));  // 5000 ms = 5 sekúnd
```

## 🐛 Riešenie problémov

### Senzory sa nenašli:
- Skontrolujte zapojenie (VDD, GND, DATA)
- Overte pull-up rezistor 4.7kΩ
- Použite kratšie vodiče (max ~30m)
- Otestujte senzory samostatne

### Zigbee sa nepripája:
- Povoľte "Permit Join" v Z2M
- Reštartujte ESP32-C6
- Skontrolujte Zigbee kanál
- Overte logy v sériovom monitore

### Teploty sa neaktualizujú:
- Skontrolujte binding a reporting v Z2M
- Overte threshold (1°C)
- Sledujte logy v monitore

Podrobné riešenie problémov nájdete v dokumentácii.

## 📚 Dokumentácia

### Základná dokumentácia:
- **[INSTALL.md](INSTALL.md)** - Podrobný inštalačný návod pre Windows
- **[WIRING.md](WIRING.md)** - Schéma zapojenia a pinout
- **[DS18B20_ADDRESS_DETECTION.md](DS18B20_ADDRESS_DETECTION.md)** - Detekcia senzorov
- **[ZIGBEE2MQTT_CONFIG.md](ZIGBEE2MQTT_CONFIG.md)** - Konfigurácia Z2M

### Pokročilá dokumentácia:
- **[HOME_ASSISTANT_EXAMPLES.md](HOME_ASSISTANT_EXAMPLES.md)** - Príklady automatizácií a Lovelace kariet
- **[FAQ.md](FAQ.md)** - Často kladené otázky (50+ otázok)
- **[CHANGELOG.md](CHANGELOG.md)** - História zmien
- **[PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)** - Súhrn projektu

## 🔗 Užitočné odkazy

- [ESP-IDF dokumentácia](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32-C6 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf)
- [Seeed XIAO ESP32-C6 Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [DS18B20 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
- [Zigbee2MQTT dokumentácia](https://www.zigbee2mqtt.io/)

## 📄 Licencia

Tento projekt je voľne použiteľný pre osobné aj komerčné účely.

## 🤝 Podpora

Pri problémoch:
1. Skontrolujte dokumentáciu v tomto repozitári
2. Overte hardware zapojenie
3. Skontrolujte logy v sériovom monitore
4. Overte verzie softvéru (ESP-IDF 5.3+)

## 🎯 Budúce vylepšenia

- [ ] Podpora pre viac ako 2 senzory
- [ ] Konfigurovateľný threshold cez Zigbee
- [ ] OTA (Over-The-Air) update
- [ ] Deep sleep režim (pre batériové napájanie)
- [ ] Kalibrácia senzorov
- [ ] Detekcia chýb senzorov a obnova

---

**Vytvorené pre:** Seeed Studio XIAO ESP32-C6  
**Framework:** ESP-IDF v5.3+  
**Protokol:** Zigbee 3.0  
**Verzia:** 1.0  
**Dátum:** November 2025
