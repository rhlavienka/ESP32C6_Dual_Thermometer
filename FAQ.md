# FAQ - Často kladené otázky

## Všeobecné otázky

### Q: Aký je rozdiel medzi Zigbee Router a Zigbee End Device?

**A:** 
- **Zigbee Router** (tento projekt):
  - Vždy zapnutý (trvalo napájaný)
  - Posilňuje Zigbee sieť
  - Preposiela správy od iných zariadení
  - Ideálny pre stacionárne zariadenia
  
- **Zigbee End Device:**
  - Môže spať (úspora energie)
  - Nevyžaduje trvalé napájanie
  - Ideálny pre batériové zariadenia
  - Neposilňuje sieť

### Q: Prečo ESP32-C6 a nie ESP32 alebo ESP8266?

**A:** ESP32-C6 má natívnu podporu Zigbee (802.15.4), zatiaľ čo starší čipy ESP32/ESP8266 túto funkciu nemajú. ESP32-C6 je prvý čip od Espressif s Zigbee podporou.

### Q: Môžem použiť viac ako 2 DS18B20 senzory?

**A:** Áno! OneWire protokol podporuje až 127 zariadení na jednej zbernici. V kóde `main.c` len upravte skenér a pridajte ďalšie endpointy pre každý senzor.

## Hardware

### Q: Funguje to s inými verziami DS18B20 (káblové, vodotesné)?

**A:** Ano, všetky verzie DS18B20 používajú rovnaký protokol:
- TO-92 (klasické 3-pinové)
- Káblová verzia (3 vodiče)
- Vodotesná verzia (nerezová sonda)
- SMD verzia

### Q: Môžem použiť iný pull-up rezistor ako 4.7kΩ?

**A:** Áno, možnosti:
- **2.2kΩ - 4.7kΩ:** Odporúčané pre väčšinu aplikácií
- **1kΩ - 2.2kΩ:** Pre dlhé káble (> 10m)
- **4.7kΩ - 10kΩ:** Pre krátke káble (< 3m)

### Q: Aká môže byť maximálna dĺžka kábla pre DS18B20?

**A:** 
- **Štandardne:** ~30m s 4.7kΩ pull-up
- **S optimalizáciou:** až 100m+ (znížený pull-up, tienený kábel, kondenzátory)

### Q: Môžem použiť ESP32-C6 na batériu?

**A:** Áno, ale:
- V tomto projekte je ESP32-C6 nastavený ako Zigbee Router (vyžaduje trvalé napájanie)
- Pre batériový prevod zmeňte na Zigbee End Device a pridajte deep sleep
- XIAO ESP32-C6 má vstavaný nabíjací obvod pre Li-Po batériu

### Q: Ktorý GPIO môžem použiť pre OneWire?

**A:** Na XIAO ESP32-C6 môžete použiť takmer akýkoľvek GPIO:
- **Odporúčané:** GPIO4, GPIO5, GPIO6, GPIO7
- **Vyhnite sa:** GPIO0, GPIO8, GPIO9 (používané pre boot/flash)

## Softvér

### Q: Prečo ESP-IDF a nie Arduino?

**A:** 
- **ESP-IDF:** Natívna podpora Zigbee, lepšia kontrola, profesionálny framework
- **Arduino:** Jednoduchší, ale chýba oficiálna Zigbee podpora pre ESP32-C6

**Poznámka:** Arduino IDE podporuje ESP32-C6, ale Zigbee stack je dostupný iba v ESP-IDF.

### Q: Aká verzia ESP-IDF je potrebná?

**A:** Minimálne **ESP-IDF v5.1**, odporúčané **v5.3** alebo novšie.

### Q: Môžem použiť platformio namiesto ESP-IDF?

**A:** Áno, ale budete musieť:
1. Vytvoriť `platformio.ini` konfiguráciu
2. Manuálne pridať Zigbee komponenty
3. Upraviť build process

Odporúčame držať sa ESP-IDF pre jednoduchosť.

### Q: Ako zmením GPIO pin pre OneWire?

**A:** V `main/main.c`:
```c
#define ONEWIRE_GPIO GPIO_NUM_X  // Nahraďte X číslom pinu
```

### Q: Ako zmením threshold pre hlásenie teploty?

**A:** V `main/main.c`:
```c
#define TEMP_REPORT_THRESHOLD 1.0f  // Zmeňte na požadovanú hodnotu v °C
```

## Zigbee a Zigbee2MQTT

### Q: Prečo sa zariadenie nepripája do Zigbee siete?

**A:** Skontrolujte:
1. Je "Permit Join" povolené v Z2M?
2. Používa Z2M správny Zigbee kanál?
3. Sú Zigbee koordinátor a ESP32-C6 na rovnakom kanále?
4. Je Zigbee koordinátor dosť blízko? (max ~10m pre prvé párovanie)

### Q: Ako resetujem Zigbee párovanie?

**A:** 
1. Vymazanie NVS pamäte:
   ```bash
   idf.py erase-flash
   idf.py flash
   ```
2. Alebo v Z2M odstráňte zariadenie a povoľte nové pripojenie

### Q: Môžem použiť iný Zigbee koordinátor (nie Z2M)?

**A:** Áno:
- **ZHA** (Home Assistant Zigbee integration)
- **deCONZ**
- **IoBroker**
- Akýkoľvek Zigbee 3.0 koordinátor

### Q: Prečo sa teploty neaktualizujú v Home Assistant?

**A:** Možné príčiny:
1. Binding nie je správne nastavený v Z2M
2. Reporting intervaly nie sú správne
3. Teplota sa nezmenila o viac ako threshold (1°C)
4. MQTT spojenie je prerušené

## Meranie teploty

### Q: Aká je presnosť DS18B20?

**A:** 
- **Presnosť:** ±0.5°C (-10°C až +85°C)
- **Rozlíšenie:** 0.0625°C (12-bit ADC)
- **Rozsah:** -55°C až +125°C

### Q: Ako dlho trvá meranie teploty?

**A:** 
- **9-bit:** ~94ms
- **10-bit:** ~188ms
- **11-bit:** ~375ms
- **12-bit:** ~750ms (predvolené)

### Q: Prečo sa zobrazuje nesprávna teplota?

**A:** Možné problémy:
1. **Parasite power mode:** Použite externe napájanie (VDD)
2. **Slabý pull-up:** Znížte rezistor na 2.2kΩ
3. **Dlhé káble:** Pridajte kondenzátory (100nF)
4. **Chybný senzor:** Vymeňte za nový

### Q: DS18B20 ukazuje 85°C alebo -127°C - čo je zle?

**A:** 
- **85°C:** DS18B20 "power-on" hodnota - čítanie pred dokončením konverzie
  - **Riešenie:** Počkajte 750ms po spustení konverzie
- **-127°C (0x00):** Chyba komunikácie
  - **Riešenie:** Skontrolujte zapojenie, pull-up rezistor

## Build a Flash

### Q: VS Code Extension hlási "ERROR_INVALID_PIP"

**A:** Problém s Python virtual environment. Riešenia:

1. **Použite Advanced setup:**
   - `Ctrl+Shift+P` → "ESP-IDF: Configure ESP-IDF Extension"
   - Vyberte "Advanced" namiesto "Express"
   - Manuálne nastavte všetky cesty

2. **Alebo používajte ESP-IDF PowerShell:**
   - Otvorte "ESP-IDF 5.3 PowerShell"
   - Používajte `idf.py` príkazy priamo

3. **Alebo manuálne nastavte VS Code settings.json:**
   ```json
   {
     "idf.espIdfPath": "C:\\Espressif\\frameworks\\esp-idf-v5.3",
     "idf.toolsPath": "C:\\Espressif\\tools",
     "idf.pythonBinPath": "C:\\Espressif\\python_env\\idf5.3_py3.11_env\\Scripts\\python.exe"
   }
   ```

### Q: Build zlyhá s chybou "zigbee not found"

**A:** 
1. Overte verziu ESP-IDF (min. 5.1)
2. Skontrolujte `sdkconfig.defaults`:
   ```
   CONFIG_ZB_ENABLED=y
   ```
3. Vykonajte `idf.py fullclean` a znovu buildnite

### Q: Flash zlyhá s "Failed to connect"

**A:** 
1. Podržte tlačidlo **BOOT** počas pripojenia USB
2. Vyskúšajte iný USB port
3. Vyskúšajte iný USB kábel (musí podporovať dáta!)
4. Znížte baud rate: `idf.py -b 115200 flash`

### Q: Ako môžem zrýchliť build?

**A:** 
1. Pridajte výnimku v antivíruse pre `C:\Espressif`
2. Použite viac vlákien: `idf.py build -j8`
3. Použite SSD disk
4. Vypnite Windows Defender real-time scanning pre Espressif adresár

## Home Assistant

### Q: Ako premenujem entity v Home Assistant?

**A:** 
1. Prejdite do: **Configuration** → **Entities**
2. Vyhľadajte `esp32c6_thermometer`
3. Kliknite na entitu a zmeňte **Entity ID** a **Friendly Name**

### Q: Ako vytvoriť grafy teplôt?

**A:** Pozrite súbor [HOME_ASSISTANT_EXAMPLES.md](HOME_ASSISTANT_EXAMPLES.md) pre komplexné príklady.

### Q: Môžem mať notifikácie na mobile?

**A:** Áno! Použite Home Assistant Companion App:
1. Nainštalujte [Home Assistant Companion](https://companion.home-assistant.io/)
2. Použite service `notify.mobile_app_YOUR_PHONE`

## Riešenie problémov

### Q: V sériovom monitore vidím "Guru Meditation Error"

**A:** 
1. Kernel panic - skontrolujte stack trace
2. Pravdepodobne problém s pamäťou alebo neplatný pointer
3. Skúste `idf.py erase-flash` a znovu flash

### Q: ESP32-C6 sa reštartuje v slučke

**A:** 
1. **Brownout:** Slabé napájanie - použite kvalitný USB zdroj
2. **Watchdog:** Task trvá príliš dlho - zvýšte `CONFIG_ESP_TASK_WDT_TIMEOUT_S`
3. **Stack overflow:** Zvýšte veľkosť stacku taskiev

### Q: OneWire zbernica nefunguje

**A:** 
1. Skontrolujte pull-up rezistor (4.7kΩ medzi DATA a VDD)
2. Overte GPIO konfiguráciu (INPUT_OUTPUT_OD)
3. Skúste iný GPIO pin
4. Použite multimeter na overenie napätia

### Q: Zigbee spojenie je nestabilné

**A:** 
1. **Interferencia:** Zmeňte Zigbee kanál (1-26)
2. **Vzdialennosť:** Pridajte ďalšie Zigbee routery
3. **WiFi konflikt:** Zigbee a WiFi používajú 2.4GHz - zmeňte kanály
4. **Anténa:** Skúste externú anténu cez U.FL konektor

## Vývoj a rozšírenie

### Q: Ako pridám podporu pre viac senzorov?

**A:** 
1. Upravte `scan_ds18b20_sensors()` na ukladanie viacerých ROM adries
2. Vytvorte pole `ds18b20_device_t sensors[N]`
3. Pridajte Zigbee endpointy (13, 14, 15, ...)
4. Upravte `temperature_sensor_task()` na loop cez všetky senzory

### Q: Môžem pridať iné typy senzorov (vlhkosť, tlak)?

**A:** Áno! Pridajte:
- I2C senzory (BME280, SHT31, ...)
- Analógové senzory na ADC pinoch
- SPI senzory
- Vytvorte nové Zigbee endpointy pre každý typ

### Q: Ako implementovať OTA update?

**A:** 
1. Pridajte OTA partition do `partitions.csv`
2. Použite `esp_https_ota` alebo `esp_zigbee_ota`
3. Hostujte firmware na web serveri
4. Implementujte OTA download a update logiku

### Q: Môžem použiť deep sleep pre úsporu energie?

**A:** 
- **Nie** ako Zigbee Router (musí byť vždy aktívny)
- **Áno** ako Zigbee End Device (zmeňte `ESP_ZB_DEVICE_TYPE_ROUTER` → `ESP_ZB_DEVICE_TYPE_ED`)

## Bezpečnosť a údržba

### Q: Je Zigbee komunikácia šifrovaná?

**A:** Áno! Zigbee 3.0 používa AES-128 šifrovanie pre všetky správy.

### Q: Ako zabezpečím zariadenie?

**A:** 
1. Použite **install code** pre párovanie (nastavte `INSTALLCODE_POLICY_ENABLE true`)
2. Pravidelne aktualizujte ESP-IDF na najnovšiu verziu
3. Nepoužívajte predvolené kľúče v produkcii

### Q: Ako často mám aktualizovať firmware?

**A:** 
- **ESP-IDF:** Pri critical security fixes
- **Váš kód:** Podľa potreby (bugfixes, nové featury)

### Q: Ako zálohujem konfiguráciu ESP32-C6?

**A:** 
1. **Kód:** Uložte do Git repository
2. **NVS:** `idf.py nvs-partition read` (experimentálne)
3. **Flash:** `esptool.py read_flash 0x0 0x400000 backup.bin`

## Performance

### Q: Ako znížim CPU záťaž?

**A:** 
1. Znížte frekvenciu merania (napr. každých 10s namiesto 5s)
2. Použite FreeRTOS priorities optimálne
3. Vypnite debug logy (`CONFIG_LOG_DEFAULT_LEVEL_ERROR`)

### Q: Koľko RAM spotrebuje aplikácia?

**A:** 
- **Statická:** ~150-200 KB (Zigbee stack, WiFi, FreeRTOS)
- **Dynamická:** ~50-100 KB (tasky, buffre)
- **Celkom:** ~250-300 KB z 512 KB dostupných

### Q: Môžem súčasne používať WiFi a Zigbee?

**A:** Áno! ESP32-C6 podporuje koexistenciu:
- Zdieľajú 2.4GHz RF
- Časové multiplexovanie
- Možný výkon degradácia pri vysokom trafficu

---

## Ďalšie otázky?

Ak máte otázku, ktorá tu nie je zodpovedaná:

1. Skontrolujte [ESP-IDF dokumentáciu](https://docs.espressif.com/projects/esp-idf/)
2. Pozrite [Seeed Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
3. Skontrolujte [Zigbee2MQTT dokumentáciu](https://www.zigbee2mqtt.io/)
4. Pozrite logy v sériovom monitore
5. Otvorte issue na GitHub (ak používate tento projekt z repozitára)

---

**Naposledy aktualizované:** November 2025
