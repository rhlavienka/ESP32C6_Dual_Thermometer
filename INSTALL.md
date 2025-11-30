# Inštalačný návod - ESP32-C6 vývojové prostredie pre Windows

Tento návod vás krok za krokom prevedie inštaláciou všetkého potrebného softvéru na Windows PC pre vývoj, kompiláciu a nahrávanie programov do **Seeed Studio XIAO ESP32-C6** pomocou **ESP-IDF** a **Visual Studio Code**.

---

## Požiadavky

- **Windows 10/11** (64-bit)
- **8 GB RAM** (odporúčané 16 GB)
- **10 GB voľného miesta** na disku
- **Internetové pripojenie** pre sťahovanie softvéru
- **USB-C kábel** na pripojenie ESP32-C6
- **Administrátorské práva** (pre inštaláciu ovládačov)

---

## Časť 1: Inštalácia základného softvéru

### Krok 1.1: Inštalácia Visual Studio Code

1. **Stiahnite VS Code:**
   - Otvorte prehliadač a prejdite na: https://code.visualstudio.com/
   - Kliknite na **"Download for Windows"**
   - Stiahnite sa súbor `VSCodeUserSetup-x64-X.XX.X.exe`

2. **Inštalujte VS Code:**
   - Spustite stiahnutý inštalátor
   - Akceptujte licenčné podmienky
   - **DÔLEŽITÉ:** Zaškrtnite tieto možnosti:
     - ✅ Add "Open with Code" action to Windows Explorer file context menu
     - ✅ Add "Open with Code" action to Windows Explorer directory context menu
     - ✅ Register Code as an editor for supported file types
     - ✅ Add to PATH
   - Kliknite na **"Install"**
   - Po dokončení kliknite na **"Finish"**

### Krok 1.2: Inštalácia Git pre Windows

1. **Stiahnite Git:**
   - Prejdite na: https://git-scm.com/download/win
   - Stiahnite sa **64-bit Git for Windows Setup**

2. **Inštalujte Git:**
   - Spustite inštalátor
   - Použite **predvolené nastavenia** (stačí klikať "Next")
   - **DÔLEŽITÉ:** Pri výbere editora vyberte **"Use Visual Studio Code as Git's default editor"**
   - Dokončite inštaláciu

3. **Overte inštaláciu:**
   - Otvorte **PowerShell** (Windows + X → Windows PowerShell)
   - Zadajte príkaz:
     ```powershell
     git --version
     ```
   - Malo by sa zobraziť: `git version 2.xx.x`

### Krok 1.3: Inštalácia Python 3

1. **Stiahnite Python:**
   - Prejdite na: https://www.python.org/downloads/
   - Kliknite na **"Download Python 3.12.x"** (alebo najnovšiu verziu 3.x)

2. **Inštalujte Python:**
   - Spustite inštalátor
   - **VEĽMI DÔLEŽITÉ:** Zaškrtnite **"Add Python 3.xx to PATH"** (dole v okne)
   - Kliknite na **"Install Now"**
   - Počkajte na dokončenie
   - Kliknite na **"Close"**

3. **Overte inštaláciu:**
   - Otvorte **nový** PowerShell (zatvorte starý ak bol otvorený)
   - Zadajte:
     ```powershell
     python --version
     ```
   - Malo by sa zobraziť: `Python 3.12.x`
   - Zadajte:
     ```powershell
     pip --version
     ```
   - Malo by sa zobraziť: `pip 23.x.x from ...`

---

## Časť 2: Inštalácia ESP-IDF

### Krok 2.1: Stiahnutie ESP-IDF Installer

1. **Stiahnite ESP-IDF Offline Installer:**
   - Prejdite na: https://dl.espressif.com/dl/esp-idf/
   - Vyberte najnovšiu verziu (napr. **esp-idf-5.5** alebo novšiu)
   - Stiahnite: **esp-idf-tools-setup-offline-5.5.exe** (alebo online verziu ak máte dobrý internet)

   **Alternatíva - Online installer:**
   - https://dl.espressif.com/dl/esp-idf/
   - Stiahnite **esp-idf-tools-setup-online-X.X.exe**

### Krok 2.2: Inštalácia ESP-IDF

1. **Spustite inštalátor ESP-IDF:**
   - Spustite stiahnutý súbor `esp-idf-tools-setup-X.X.exe`
   - Ak sa zobrazí **User Account Control**, kliknite **"Yes"**

2. **Výber verzie ESP-IDF:**
   - Vyberte **"ESP-IDF v5.5"** alebo **"ESP-IDF v5.3"** (alebo najnovšiu stable verziu)
   - Kliknite **"Next"**

3. **Výber inštalačnej cesty:**
   - Odporúčaná cesta: `C:\Espressif`
   - **POZNÁMKA:** Nepoužívajte cestu s medzerami alebo s diakritikou!
   - Kliknite **"Next"**

4. **Výber nástrojov:**
   - Nechajte zaškrtnuté:
     - ✅ ESP-IDF Tools
     - ✅ ESP-IDF v5.5 (alebo vaša verzia)
     - ✅ CMake
     - ✅ Ninja
     - ✅ Python packages
   - Kliknite **"Next"**

5. **Začiatok inštalácie:**
   - Kliknite **"Install"**
   - **Čakajte** (môže trvať 10-30 minút v závislosti od rýchlosti internetu a PC)
   - Po dokončení kliknite **"Finish"**

### Krok 2.3: Overenie ESP-IDF inštalácie

1. **Otvorte ESP-IDF PowerShell:**
   - V ponuke Štart nájdite: **"ESP-IDF 5.3 PowerShell"** alebo **"ESP-IDF PowerShell"**
   - Spustite ho

2. **Overte verziu:**
   ```powershell
   idf.py --version
   ```
   - Malo by sa zobraziť: `ESP-IDF v5.3.x` (alebo vaša verzia)

3. **Overte kompilátor:**
   ```powershell
   xtensa-esp32-elf-gcc --version
   ```
   - Malo by sa zobraziť informácia o GCC kompilátore

---

## Časť 3: Konfigurácia Visual Studio Code pre ESP-IDF

### Krok 3.1: Inštalácia ESP-IDF rozšírenia

1. **Otvorte VS Code**

2. **Otvorte Extensions:**
   - Kliknite na ikonu **Extensions** v ľavom paneli (alebo stlačte `Ctrl+Shift+X`)

3. **Nainštalujte ESP-IDF Extension:**
   - Do vyhľadávacieho poľa zadajte: **"Espressif IDF"**
   - Nájdite rozšírenie: **"ESP-IDF"** od vydavateľa **Espressif**
   - Kliknite na **"Install"**
   - Počkajte na dokončenie inštalácie

4. **Nainštalujte C/C++ Extension:**
   - Vyhľadajte: **"C/C++"**
   - Nainštalujte rozšírenie **"C/C++"** od **Microsoft**

### Krok 3.2: Konfigurácia ESP-IDF rozšírenia

1. **Otvorte Command Palette:**
   - Stlačte `Ctrl+Shift+P`

2. **Spustite konfiguráciu:**
   - Zadajte: **"ESP-IDF: Configure ESP-IDF Extension"**
   - Stlačte Enter

3. **Vyberte Express Setup:**
   - Vyberte **"Express"** (odporúčané pre začiatočníkov)
   - Kliknite **"Continue"**

4. **Nastavte cesty:**
   - **ESP-IDF Path:** `C:\Espressif\frameworks\esp-idf-v5.3` (alebo vaša verzia)
   - **ESP-IDF Tools Path:** `C:\Espressif`
   - **Python Path:** Malo by sa automaticky nájsť (napr. `C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe`)
   - Kliknite **"Configure"**

5. **Počkajte na dokončenie:**
   - Extension sa nakonfiguruje (môže trvať 1-2 minúty)
   - Po dokončení sa zobrazí: **"ESP-IDF configured successfully"**

#### ⚠️ Ak sa vyskytne chyba "ERROR_INVALID_PIP":

Toto je známy problém s Python virtual environment. **Riešenie:**

**Variant 1 - Použite Advanced namiesto Express:**

1. V kroku 3 vyberte **"Advanced"** namiesto "Express"
2. Nastavte:
   - **Select ESP-IDF version:** `Find ESP-IDF in your system`
   - **Enter ESP-IDF directory:** `C:\Espressif\frameworks\esp-idf-v5.3`
   - **Select Python:** `Use existing Python` → `C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe`
   - **Select Custom tools folder:** `C:\Espressif\tools`
3. Kliknite **"Configure"**

**Variant 2 - Manuálne nastavenie v VS Code settings:**

1. Otvorte VS Code Settings (`Ctrl+,`)
2. Vyhľadajte: `idf`
3. Nastavte tieto hodnoty:
   - `Idf.espIdfPath`: `C:\Espressif\frameworks\esp-idf-v5.3`
   - `Idf.toolsPath`: `C:\Espressif\tools`
   - `Idf.pythonBinPath`: `C:\Espressif\python_env\idf5.3_py3.11_env\Scripts\python.exe`
   - `Idf.gitPath`: `C:\Program Files\Git\cmd\git.exe`
   - `Idf.customExtraPaths`: Nechajte prázdne
4. Reštartujte VS Code

**Variant 3 - Použite ESP-IDF PowerShell namiesto VS Code:**

Ak VS Code extension naďalej nefunguje, môžete používať priamo ESP-IDF PowerShell:

1. Otvorte **ESP-IDF 5.3 PowerShell** (z ponuky Štart)
2. Navigujte do projektu:
   ```powershell
   cd "C:\Users\rhlavienka\OneDrive - SOFTIP, a.s\Documents\DevOps\PlayGround\ESP32\C6_Thermometer"
   ```
3. Nastavte target:
   ```powershell
   idf.py set-target esp32c6
   ```
4. Build:
   ```powershell
   idf.py build
   ```
5. Flash (nahraďte COM port):
   ```powershell
   idf.py -p COM3 flash monitor
   ```

**Variant 4 - Preinštalujte Python environment:**

1. Otvorte **ESP-IDF PowerShell**
2. Spustite:
   ```powershell
   cd C:\Espressif\frameworks\esp-idf-v5.3
   python install.py
   ```
3. Potom v VS Code: `Ctrl+Shift+P` → **"ESP-IDF: Configure ESP-IDF Extension"** → Advanced

### Krok 3.3: Nastavenie USB ovládačov pre ESP32-C6

1. **Pripojte ESP32-C6 k PC:**
   - Použijte USB-C kábel (ktorý podporuje dáta!)
   - Pripojte XIAO ESP32-C6 k PC

2. **Overte rozpoznanie zariadenia:**
   - Otvorte **Device Manager** (Správca zariadení):
     - Windows + X → Device Manager
   - Rozbaľte sekciu **"Ports (COM & LPT)"**
   - Mali by ste vidieť: **"USB Serial Device (COMx)"** alebo **"USB-SERIAL CH340 (COMx)"**
   - Zapamätajte si číslo portu (napr. **COM3**, **COM5**, atď.)

3. **Ak sa nezobrazuje COM port:**
   - **Možný problém:** Chýbajúce ovládače alebo zlý USB kábel
   - **Riešenie 1:** Skúste iný USB kábel (niektoré káble sú iba na napájanie)
   - **Riešenie 2:** Nainštalujte CH340 ovládače:
     - Stiahnite z: https://www.wch.cn/downloads/CH341SER_EXE.html
     - Nainštalujte a reštartujte PC
   - **Riešenie 3:** Skúste režim bootloadera:
     - Odpojte ESP32-C6
     - Podržte tlačidlo **BOOT**
     - Pripojte USB kábel (stále držte BOOT)
     - Uvoľnite BOOT po 2 sekundách

---

## Časť 4: Kompilácia a nahratie projektu

### Krok 4.1: Otvorenie projektu v VS Code

1. **Otvorte VS Code**

2. **Otvorte projekt:**
   - `File` → `Open Folder...`
   - Vyberte priečinok s projektom: `C:\Users\...\C6_Thermometer`
   - Kliknite **"Select Folder"**

3. **Dôverujte priečinku:**
   - Ak sa zobrazí výzva "Do you trust the authors...", kliknite **"Yes, I trust the authors"**

### Krok 4.2: Výber cieľového čipu (Target)

1. **Otvorte Command Palette:**
   - `Ctrl+Shift+P`

2. **Nastavte target:**
   - Zadajte: **"ESP-IDF: Set Espressif Device Target"**
   - Vyberte: **"esp32c6"**
   - Kliknite Enter

3. **Počkajte na dokončenie:**
   - Extension nakonfiguruje projekt pre ESP32-C6

### Krok 4.3: Výber sériového portu

1. **Otvorte Command Palette:**
   - `Ctrl+Shift+P`

2. **Vyberte port:**
   - Zadajte: **"ESP-IDF: Select Port to Use"**
   - Vyberte port vášho zariadenia (napr. **COM3**)

### Krok 4.4: Konfigurácia projektu (menuconfig)

1. **Otvorte menuconfig:**
   - `Ctrl+Shift+P`
   - Zadajte: **"ESP-IDF: SDK Configuration Editor (menuconfig)"**
   - Otvorí sa grafické rozhranie

2. **Skontrolujte nastavenia:**
   - **Component config → Zigbee:**
     - ✅ Enable Zigbee
     - ✅ Zigbee ZCZR (Router)
   - **Serial flasher config:**
     - Flash size: **4 MB**
   - **Partition Table:**
     - Custom partition table CSV: `partitions.csv`

3. **Uložte a zatvorte:**
   - Kliknite na **"Save"** (hore vpravo)
   - Zatvorte menuconfig

### Krok 4.5: Kompilácia projektu (Build)

1. **Spustite build:**
   - **Spôsob 1:** Kliknite na ikonu **Build** v dolnom paneli VS Code (ikona kladiva)
   - **Spôsob 2:** `Ctrl+Shift+P` → **"ESP-IDF: Build your Project"**
   - **Spôsob 3:** Stlačte `Ctrl+E` → `B`

2. **Sledujte výstup:**
   - V termináli uvidíte priebeh kompilácie
   - **Prvá kompilácia** môže trvať 5-10 minút (sťahujú sa komponenty)
   - **Ďalšie kompilácie** sú rýchlejšie (1-2 minúty)

3. **Úspešná kompilácia:**
   - Na konci uvidíte:
     ```
     Project build complete. To flash, run:
     idf.py flash
     ```

4. **Ak sa vyskytli chyby:**
   - Skontrolujte výstup terminálu
   - Overte, že máte správne nastavený target (esp32c6)
   - Skontrolujte, či sú všetky súbory projektu prítomné

### Krok 4.6: Nahratie programu do ESP32-C6 (Flash)

1. **Overte pripojenie:**
   - ESP32-C6 je pripojený cez USB
   - Port je správne vybraný

2. **Nahrajte program:**
   - **Spôsob 1:** Kliknite na ikonu **Flash** (blesk) v dolnom paneli
   - **Spôsob 2:** `Ctrl+Shift+P` → **"ESP-IDF: Flash your Project"**
   - **Spôsob 3:** Stlačte `Ctrl+E` → `F`

3. **Sledujte priebeh:**
   - Zobrazí sa: `Connecting...`
   - Potom: `Writing at 0x...`
   - Na konci: `Hash of data verified`

4. **Ak sa vyskytne chyba "Failed to connect":**
   - **Riešenie:** Prejdite do bootloader režimu:
     1. Odpojte USB
     2. Podržte tlačidlo **BOOT** na XIAO
     3. Pripojte USB (stále držte BOOT)
     4. Počkajte 2 sekundy
     5. Uvoľnite BOOT
     6. Znovu spustite Flash

### Krok 4.7: Sledovanie výstupu (Monitor)

1. **Otvorte monitor:**
   - **Spôsob 1:** Kliknite na ikonu **Monitor** (obrazovka) v dolnom paneli
   - **Spôsob 2:** `Ctrl+Shift+P` → **"ESP-IDF: Monitor your Device"**
   - **Spôsob 3:** Stlačte `Ctrl+E` → `M`

2. **Sledujte logy:**
   - Uvidíte bootovací výstup ESP32-C6
   - Informácie o inicializácii Zigbee
   - Skenování DS18B20 senzorov
   - Hodnoty teplôt

3. **Ukončenie monitora:**
   - Stlačte `Ctrl+]`

### Krok 4.8: Build, Flash a Monitor naraz

Pre rýchly vývoj môžete spustiť všetko naraz:

1. **Spustite Flash & Monitor:**
   - `Ctrl+Shift+P` → **"ESP-IDF: Build, Flash and Start a Monitor"**
   - Alebo kliknite na ikonu **Flame** (oheň) v dolnom paneli

---

## Časť 5: Užitočné VS Code skratky pre ESP-IDF

| Skratka | Akcia |
|---------|-------|
| `Ctrl+E` `B` | Build project |
| `Ctrl+E` `F` | Flash project |
| `Ctrl+E` `M` | Monitor device |
| `Ctrl+E` `D` | Build, Flash & Monitor |
| `Ctrl+E` `C` | Clean project |
| `Ctrl+E` `S` | Size analysis |
| `Ctrl+Shift+P` | Command Palette |

---

## Časť 6: Riešenie bežných problémov

### Problém: "idf.py not found"

**Riešenie:**
1. Overte, že ste otvorili **ESP-IDF PowerShell** (nie bežný PowerShell)
2. Alebo v VS Code: `Ctrl+Shift+P` → **"ESP-IDF: Open ESP-IDF Terminal"**

### Problém: "Port is busy" alebo "Permission denied"

**Riešenie:**
1. Zatvorte všetky programy používajúce sériový port (Arduino IDE, PuTTY, atď.)
2. Zatvorte monitor (`Ctrl+]`)
3. Skúste znovu

### Problém: "Failed to connect to ESP32-C6"

**Riešenie:**
1. Odpojte a znovu pripojte USB
2. Skúste bootloader režim (BOOT tlačidlo)
3. Skúste iný USB port
4. Skúste iný USB kábel
5. Znížte baud rate: `idf.py -p COM3 -b 115200 flash`

### Problém: Pomalá kompilácia

**Riešenie:**
1. Pridajte výnimku do Windows Defender pre priečinok `C:\Espressif`
2. Vypnite antivírus počas kompilácie
3. Použite SSD disk
4. Zvýšte počet vlákien: `idf.py build -j8`

### Problém: "No module named 'serial'"

**Riešenie:**
```powershell
pip install pyserial
```

---

## Časť 7: Ďalšie kroky

### Pripojenie DS18B20 senzorov

Pozrite súbor: **DS18B20_ADDRESS_DETECTION.md**

### Konfigurácia Zigbee2MQTT

Pozrite súbor: **ZIGBEE2MQTT_CONFIG.md**

### Prispôsobenie kódu

1. **Zmena GPIO pinu pre OneWire:**
   - Upravte `main/main.c`:
     ```c
     #define ONEWIRE_GPIO GPIO_NUM_5  // Zmeňte na požadovaný pin
     ```

2. **Zmena threshold pre hlásenie:**
   - Upravte `main/main.c`:
     ```c
     #define TEMP_REPORT_THRESHOLD 1.0f  // Zmeňte na požadovanú hodnotu
     ```

3. **Zmena periódy merania:**
   - Upravte `temperature_sensor_task()` v `main/main.c`:
     ```c
     vTaskDelay(pdMS_TO_TICKS(5000));  // Zmeňte 5000 na požadovaný počet ms
     ```

---

## Zhrnutie krokov pre nový projekt

1. Otvorte **VS Code**
2. Otvorte priečinok projektu
3. `Ctrl+Shift+P` → **"ESP-IDF: Set Espressif Device Target"** → `esp32c6`
4. `Ctrl+Shift+P` → **"ESP-IDF: Select Port to Use"** → vybrať COM port
5. `Ctrl+E` `D` (Build, Flash & Monitor)
6. Sledujte logy v monitore

---

## Užitočné odkazy

- **ESP-IDF dokumentácia:** https://docs.espressif.com/projects/esp-idf/en/latest/
- **ESP32-C6 dokumentácia:** https://www.espressif.com/en/products/socs/esp32-c6
- **Seeed XIAO ESP32-C6 Wiki:** https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- **ESP-IDF VS Code Extension:** https://github.com/espressif/vscode-esp-idf-extension
- **Zigbee dokumentácia:** https://docs.espressif.com/projects/esp-zigbee-sdk/

---

**Gratulujem! Váše vývojové prostredie je pripravené na prácu s ESP32-C6! 🎉**
