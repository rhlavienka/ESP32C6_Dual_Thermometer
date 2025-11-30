# CHANGELOG

Všetky významné zmeny v tomto projekte budú zdokumentované v tomto súbore.

## [1.0.0] - 2025-11-29

### Pridané
- Kompletný ESP-IDF projekt pre ESP32-C6 s Zigbee podporou
- OneWire driver pre komunikáciu s DS18B20 senzormi
- DS18B20 driver s podporou pre viacero senzorov na jednej zbernici
- Automatická detekcia DS18B20 senzorov a ich ROM adries
- Zigbee Router funkcia (posilňuje Zigbee sieť)
- Dva nezávislé Zigbee endpointy (11 a 12) pre každý senzor
- Inteligentné reportovanie - posiela údaje len pri zmene ≥1°C
- Konfigurácia pre Zigbee2MQTT (external converter)
- Kompletná dokumentácia:
  - README.md - Prehľad projektu
  - INSTALL.md - Podrobný inštalačný návod pre Windows
  - DS18B20_ADDRESS_DETECTION.md - Návod na detekciu senzorov
  - ZIGBEE2MQTT_CONFIG.md - Konfigurácia Z2M
  - WIRING.md - Schéma zapojenia
  - HOME_ASSISTANT_EXAMPLES.md - Príklady automatizácií
  - FAQ.md - Často kladené otázky
- .gitignore pre ESP-IDF projekty
- Partition table optimalizovaná pre Zigbee
- sdkconfig.defaults so základnou konfiguráciou

### Technické detaily
- ESP-IDF verzia: 5.3+
- Target: ESP32-C6 (Seeed Studio XIAO)
- OneWire GPIO: GPIO5 (D4)
- Zigbee kanál: Konfigurovateľný cez PRIMARY_CHANNEL_MASK
- Meranie každých: 5 sekúnd
- Report threshold: 1.0°C
- DS18B20 rozlíšenie: 12-bit (0.0625°C)

### Známe problémy
- Žiadne v tejto verzii

---

## Budúce verzie (plánované)

### [1.1.0] - TBD
- [ ] Podpora pre > 2 DS18B20 senzory
- [ ] Konfigurovateľný threshold cez Zigbee attribute
- [ ] Web interface pre konfiguráciu
- [ ] MQTT publish (paralelne s Zigbee)

### [1.2.0] - TBD
- [ ] OTA (Over-The-Air) update podpora
- [ ] Deep sleep režim pre batériový prevod
- [ ] Kalibrácia senzorov
- [ ] Detekcia a reporting chýb senzorov

### [2.0.0] - TBD
- [ ] Podpora pre ďalšie typy senzorov (DHT22, BME280, ...)
- [ ] Multi-sensor endpoint agregácia
- [ ] LCD displej podpora (pre standalone režim)
- [ ] SD karta logging

---

## Formát zápisu

- **[Major.Minor.Patch]** - Dátum
  - **Pridané** - Nové funkcie
  - **Zmenené** - Zmeny v existujúcich funkciách
  - **Opravené** - Bugfixy
  - **Odstránené** - Odstránené funkcie
  - **Bezpečnosť** - Security patches

---

**Poznámka:** Tento projekt dodržiava [Semantic Versioning](https://semver.org/).
