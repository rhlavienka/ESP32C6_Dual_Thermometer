# Installation Guide - ESP32-C6 Development Environment for Windows

This guide will step-by-step walk you through installing all necessary software on a Windows PC for developing, compiling, and uploading programs to **Seeed Studio XIAO ESP32-C6** using **ESP-IDF** and **Visual Studio Code**.

---

## Requirements

- **Windows 10/11** (64-bit)
- **8 GB RAM** (16 GB recommended)
- **10 GB free disk space**
- **Internet connection** for downloading software
- **USB-C cable** for connecting ESP32-C6
- **Administrator rights** (for driver installation)

---

## Part 1: Installing Basic Software

### Step 1.1: Installing Visual Studio Code

1. **Download VS Code:**
   - Open your browser and go to: https://code.visualstudio.com/
   - Click on **"Download for Windows"**
   - Download the file `VSCodeUserSetup-x64-X.XX.X.exe`

2. **Install VS Code:**
   - Run the downloaded installer
   - Accept the license terms
   - **IMPORTANT:** Check these options:
     - ✅ Add "Open with Code" action to Windows Explorer file context menu
     - ✅ Add "Open with Code" action to Windows Explorer directory context menu
     - ✅ Register Code as an editor for supported file types
     - ✅ Add to PATH
   - Click on **"Install"**
   - After completion, click on **"Finish"**

### Step 1.2: Installing Git for Windows

1. **Download Git:**
   - Go to: https://git-scm.com/download/win
   - Download **64-bit Git for Windows Setup**

2. **Install Git:**
   - Run the installer
   - Use **default settings** (just keep clicking "Next")
   - **IMPORTANT:** When selecting the editor, choose **"Use Visual Studio Code as Git's default editor"**
   - Complete the installation

3. **Verify installation:**
   - Open **PowerShell** (Windows + X → Windows PowerShell)
   - Enter the command:
     ```powershell
     git --version
     ```
   - You should see: `git version 2.xx.x`

### Step 1.3: Installing Python 3

1. **Download Python:**
   - Go to: https://www.python.org/downloads/
   - Click on **"Download Python 3.12.x"** (or the latest 3.x version)

2. **Install Python:**
   - Run the installer
   - **VERY IMPORTANT:** Check **"Add Python 3.xx to PATH"** (at the bottom of the window)
   - Click **"Install Now"**
   - Wait for completion
   - Click **"Close"**

3. **Verify installation:**
   - Open a **new** PowerShell (close the old one if it was open)
   - Enter:
     ```powershell
     python --version
     ```
   - You should see: `Python 3.12.x`
   - Enter:
     ```powershell
     pip --version
     ```
   - You should see: `pip 23.x.x from ...`

---

## Part 2: Installing ESP-IDF

### Step 2.1: Downloading ESP-IDF Installer

1. **Download ESP-IDF Offline Installer:**
   - Go to: https://dl.espressif.com/dl/esp-idf/
   - Select version **esp-idf-5.5.1** (or newer if available)
   - Download: **esp-idf-tools-setup-offline-5.5.1.exe** (or online version if you have good internet)

   **Alternative - Online installer:**
   - https://dl.espressif.com/dl/esp-idf/
   - Download **esp-idf-tools-setup-online-X.X.exe**

### Step 2.2: Installing ESP-IDF

1. **Run ESP-IDF installer:**
   - Run the downloaded file `esp-idf-tools-setup-X.X.exe`
   - If **User Account Control** appears, click **"Yes"**

2. **Select ESP-IDF version:**
   - Select **"ESP-IDF v5.5.1"** (this is the version used in this project)
   - Click **"Next"

3. **Select installation path:**
   - Recommended path: `C:\Espressif`
   - **NOTE:** Don't use paths with spaces or diacritics!
   - Click **"Next"**

4. **Select tools:**
   - Leave checked:
     - ✅ ESP-IDF Tools
     - ✅ ESP-IDF v5.5.1
     - ✅ CMake
     - ✅ Ninja
     - ✅ Python packages
   - Click **"Next"

5. **Start installation:**
   - Click **"Install"**
   - **Wait** (can take 10-30 minutes depending on internet speed and PC)
   - After completion, click **"Finish"**

### Step 2.3: Verifying ESP-IDF Installation

1. **Open ESP-IDF PowerShell:**
   - In the Start menu find: **"ESP-IDF 5.5.1 PowerShell"** or **"ESP-IDF PowerShell"**
   - Run it

2. **Verify version:**
   ```powershell
   idf.py --version
   ```
   - You should see: `ESP-IDF v5.5.1`

3. **Verify compiler:**
   ```powershell
   xtensa-esp32-elf-gcc --version
   ```
   - You should see information about the GCC compiler

---

## Part 3: Configuring Visual Studio Code for ESP-IDF

### Step 3.1: Installing ESP-IDF Extension

1. **Open VS Code**

2. **Open Extensions:**
   - Click on the **Extensions** icon in the left panel (or press `Ctrl+Shift+X`)

3. **Install ESP-IDF Extension:**
   - In the search box, type: **"Espressif IDF"**
   - Find the extension: **"ESP-IDF"** by publisher **Espressif**
   - Click **"Install"**
   - Wait for installation to complete

4. **Install C/C++ Extension:**
   - Search for: **"C/C++"**
   - Install the **"C/C++"** extension by **Microsoft**

### Step 3.2: Configuring ESP-IDF Extension

1. **Open Command Palette:**
   - Press `Ctrl+Shift+P`

2. **Run configuration:**
   - Type: **"ESP-IDF: Configure ESP-IDF Extension"**
   - Press Enter

3. **Select Express Setup:**
   - Select **"Express"** (recommended for beginners)
   - Click **"Continue"**

4. **Set paths:**
   - **ESP-IDF Path:** `C:\Espressif\frameworks\esp-idf-v5.5.1`
   - **ESP-IDF Tools Path:** `C:\Espressif`
   - **Python Path:** Should be found automatically (e.g. `C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe`)
   - Click **"Configure"

5. **Wait for completion:**
   - The extension will configure (may take 1-2 minutes)
   - After completion you will see: **"ESP-IDF configured successfully"**

#### ⚠️ If you encounter the error "ERROR_INVALID_PIP":

This is a known issue with Python virtual environment. **Solution:**

**Variant 1 - Use Advanced instead of Express:**

1. In step 3, select **"Advanced"** instead of "Express"
2. Set:
   - **Select ESP-IDF version:** `Find ESP-IDF in your system`
   - **Enter ESP-IDF directory:** `C:\Espressif\frameworks\esp-idf-v5.5.1`
   - **Select Python:** `Use existing Python` → `C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe`
   - **Select Custom tools folder:** `C:\Espressif\tools`
3. Click **"Configure"

**Variant 2 - Manual setup in VS Code settings:**

1. Open VS Code Settings (`Ctrl+,`)
2. Search for: `idf`
3. Set these values:
   - `Idf.espIdfPath`: `C:\Espressif\frameworks\esp-idf-v5.5.1`
   - `Idf.toolsPath`: `C:\Espressif\tools`
   - `Idf.pythonBinPath`: `C:\Espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe`
   - `Idf.gitPath`: `C:\Program Files\Git\cmd\git.exe`
   - `Idf.customExtraPaths`: Leave empty
4. Restart VS Code

**Variant 3 - Use ESP-IDF PowerShell instead of VS Code:**

If VS Code extension still doesn't work, you can use ESP-IDF PowerShell directly:

1. Open **ESP-IDF 5.5.1 PowerShell** (from Start menu)
2. Navigate to the project:
   ```powershell
   cd "C:\Users\rhlavienka\OneDrive - SOFTIP, a.s\Documents\DevOps\PlayGround\ESP32\C6_Thermometer"
   ```
3. Set target:
   ```powershell
   idf.py set-target esp32c6
   ```
4. Build:
   ```powershell
   idf.py build
   ```
5. Flash (replace COM port):
   ```powershell
   idf.py -p COM3 flash monitor
   ```

**Variant 4 - Reinstall Python environment:**

1. Open **ESP-IDF PowerShell**
2. Run:
   ```powershell
   cd C:\Espressif\frameworks\esp-idf-v5.5.1
   python install.py
   ```
3. Then in VS Code: `Ctrl+Shift+P` → **"ESP-IDF: Configure ESP-IDF Extension"** → Advanced

### Step 3.3: Setting Up USB Drivers for ESP32-C6

1. **Connect ESP32-C6 to PC:**
   - Use a USB-C cable (that supports data!)
   - Connect XIAO ESP32-C6 to PC

2. **Verify device recognition:**
   - Open **Device Manager**:
     - Windows + X → Device Manager
   - Expand the **"Ports (COM & LPT)"** section
   - You should see: **"USB Serial Device (COMx)"** or **"USB-SERIAL CH340 (COMx)"**
   - Remember the port number (e.g. **COM3**, **COM5**, etc.)

3. **If COM port doesn't appear:**
   - **Possible issue:** Missing drivers or bad USB cable
   - **Solution 1:** Try a different USB cable (some cables are power-only)
   - **Solution 2:** Install CH340 drivers:
     - Download from: https://www.wch.cn/downloads/CH341SER_EXE.html
     - Install and restart PC
   - **Solution 3:** Try bootloader mode:
     - Disconnect ESP32-C6
     - Hold the **BOOT** button
     - Connect USB cable (still hold BOOT)
     - Release BOOT after 2 seconds

---

## Part 4: Compiling and Uploading the Project

### Step 4.1: Opening the Project in VS Code

1. **Open VS Code**

2. **Open project:**
   - `File` → `Open Folder...`
   - Select project folder: `C:\Users\...\C6_Thermometer`
   - Click **"Select Folder"**

3. **Trust the folder:**
   - If prompted "Do you trust the authors...", click **"Yes, I trust the authors"**

### Step 4.2: Selecting Target Chip (Target)

1. **Open Command Palette:**
   - `Ctrl+Shift+P`

2. **Set target:**
   - Type: **"ESP-IDF: Set Espressif Device Target"**
   - Select: **"esp32c6"**
   - Press Enter

3. **Wait for completion:**
   - Extension will configure the project for ESP32-C6

### Step 4.3: Selecting Serial Port

1. **Open Command Palette:**
   - `Ctrl+Shift+P`

2. **Select port:**
   - Type: **"ESP-IDF: Select Port to Use"**
   - Select your device's port (e.g. **COM3**)

### Step 4.4: Project Configuration (menuconfig)

1. **Open menuconfig:**
   - `Ctrl+Shift+P`
   - Type: **"ESP-IDF: SDK Configuration Editor (menuconfig)"**
   - A graphical interface will open

2. **Check settings:**
   - **Component config → Zigbee:**
     - ✅ Enable Zigbee
     - ✅ Zigbee ZCZR (Router)
   - **Serial flasher config:**
     - Flash size: **4 MB**
   - **Partition Table:**
     - Custom partition table CSV: `partitions.csv`

3. **Save and close:**
   - Click **"Save"** (top right)
   - Close menuconfig

### Step 4.5: Compiling the Project (Build)

1. **Start build:**
   - **Method 1:** Click the **Build** icon in VS Code's bottom panel (hammer icon)
   - **Method 2:** `Ctrl+Shift+P` → **"ESP-IDF: Build your Project"**
   - **Method 3:** Press `Ctrl+E` → `B`

2. **Monitor output:**
   - In the terminal you will see the compilation progress
   - **First compilation** may take 5-10 minutes (downloading components)
   - **Subsequent compilations** are faster (1-2 minutes)

3. **Successful compilation:**
   - At the end you will see:
     ```
     Project build complete. To flash, run:
     idf.py flash
     ```

4. **If errors occurred:**
   - Check the terminal output
   - Verify that you have the correct target set (esp32c6)
   - Check that all project files are present

### Step 4.6: Uploading Program to ESP32-C6 (Flash)

1. **Verify connection:**
   - ESP32-C6 is connected via USB
   - Port is correctly selected

2. **Upload program:**
   - **Method 1:** Click the **Flash** icon (lightning) in the bottom panel
   - **Method 2:** `Ctrl+Shift+P` → **"ESP-IDF: Flash your Project"**
   - **Method 3:** Press `Ctrl+E` → `F`

3. **Monitor progress:**
   - You will see: `Connecting...`
   - Then: `Writing at 0x...`
   - At the end: `Hash of data verified`

4. **If "Failed to connect" error occurs:**
   - **Solution:** Enter bootloader mode:
     1. Disconnect USB
     2. Hold the **BOOT** button on XIAO
     3. Connect USB (still hold BOOT)
     4. Wait 2 seconds
     5. Release BOOT
     6. Run Flash again

### Step 4.7: Monitoring Output (Monitor)

1. **Open monitor:**
   - **Method 1:** Click on **Monitor** icon (screen) in bottom panel
   - **Method 2:** `Ctrl+Shift+P` → **"ESP-IDF: Monitor your Device"**
   - **Method 3:** Press `Ctrl+E` → `M`

2. **Monitor logs:**
   - You will see ESP32-C6 boot output
   - Zigbee initialization info
   - DS18B20 sensor scanning
   - Temperature values

3. **Exit monitor:**
   - Press `Ctrl+]`

### Step 4.8: Build, Flash and Monitor at once

For quick development you can run everything at once:

1. **Run Flash & Monitor:**
   - `Ctrl+Shift+P` → **"ESP-IDF: Build, Flash and Start a Monitor"**
   - Or click on **Flame** icon (fire) in bottom panel

---

## Part 5: Useful VS Code Shortcuts for ESP-IDF

| Shortcut | Action |
|---------|-------|
| `Ctrl+E` `B` | Build project |
| `Ctrl+E` `F` | Flash project |
| `Ctrl+E` `M` | Monitor device |
| `Ctrl+E` `D` | Build, Flash & Monitor |
| `Ctrl+E` `C` | Clean project |
| `Ctrl+E` `S` | Size analysis |
| `Ctrl+Shift+P` | Command Palette |

---

## Part 6: Troubleshooting Common Problems

### Problem: "idf.py not found"

**Solution:**
1. Verify that you opened **ESP-IDF PowerShell** (not regular PowerShell)
2. Or in VS Code: `Ctrl+Shift+P` → **"ESP-IDF: Open ESP-IDF Terminal"**

### Problem: "Port is busy" or "Permission denied"

**Solution:**
1. Close all programs using the serial port (Arduino IDE, PuTTY, etc.)
2. Close monitor (`Ctrl+]`)
3. Try again

### Problem: "Failed to connect to ESP32-C6"

**Solution:**
1. Disconnect and reconnect USB
2. Try bootloader mode (BOOT button)
3. Try a different USB port
4. Try a different USB cable
5. Reduce baud rate: `idf.py -p COM3 -b 115200 flash`

### Problem: Slow compilation

**Solution:**
1. Add exception in Windows Defender for folder `C:\Espressif`
2. Disable antivirus during compilation
3. Use SSD disk
4. Increase thread count: `idf.py build -j8`

### Problem: "No module named 'serial'"

**Solution:**
```powershell
pip install pyserial
```

---

## Part 7: Next Steps

### Connecting DS18B20 Sensors

See file: **DS18B20_ADDRESS_DETECTION.md**

### Configuring Zigbee2MQTT

See file: **ZIGBEE2MQTT_CONFIG.md**

### Customizing Code

1. **Change GPIO pin for OneWire:**
   - Edit `main/main.c`:
     ```c
     #define ONEWIRE_GPIO GPIO_NUM_5  // Change to desired pin
     ```

2. **Change threshold for reporting:**
   - Edit `main/main.c`:
     ```c
     #define TEMP_REPORT_THRESHOLD 1.0f  // Change to desired value
     ```

3. **Change measurement period:**
   - Edit `temperature_sensor_task()` in `main/main.c`:
     ```c
     vTaskDelay(pdMS_TO_TICKS(5000));  // Change 5000 to desired number of ms
     ```

---

## Summary of Steps for New Project

1. Open **VS Code**
2. Open project folder
3. `Ctrl+Shift+P` → **"ESP-IDF: Set Espressif Device Target"** → `esp32c6`
4. `Ctrl+Shift+P` → **"ESP-IDF: Select Port to Use"** → select COM port
5. `Ctrl+E` `D` (Build, Flash & Monitor)
6. Monitor logs in monitor

---

## Useful Links

- **ESP-IDF documentation:** https://docs.espressif.com/projects/esp-idf/en/latest/
- **ESP32-C6 documentation:** https://www.espressif.com/en/products/socs/esp32-c6
- **Seeed XIAO ESP32-C6 Wiki:** https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/
- **ESP-IDF VS Code Extension:** https://github.com/espressif/vscode-esp-idf-extension
- **Zigbee documentation:** https://docs.espressif.com/projects/esp-zigbee-sdk/

---

**Congratulations! Your development environment is ready to work with ESP32-C6! 🎉**
