# Schéma zapojenia - ESP32-C6 + DS18B20

## Základné zapojenie

```
                    SEEED XIAO ESP32-C6
                    ┌─────────────────┐
                    │                 │
                    │  [USB-C]        │
                    │                 │
                    │                 │
        3.3V  ──────┤ 3V3         D0  │
                    │                 │
         GND  ──────┤ GND         D1  │
                    │                 │
                    │             D2  │
                    │                 │
                    │             D3  │
                    │                 │
   OneWire  ────────┤ D4 (GPIO5)  D5  │
                    │                 │
                    │             D6  │
                    │                 │
                    │             D7  │
                    │                 │
                    │             D8  │
                    │                 │
                    │             D9  │
                    │                 │
                    │             D10 │
                    │                 │
                    └─────────────────┘
```

## Detailné zapojenie s DS18B20

```
     3.3V                                              3.3V
       │                                                 │
       │                ┌────────────┐                   │
       ├────────────────┤ 4.7kΩ     ├───────────────────┤
       │                └────────────┘                   │
       │                      │                          │
       │                      │                          │
       │         ┌────────────┼─────────────┐            │
       │         │            │             │            │
       │         │            │             │            │
    ┌──┴──┐   ┌──┴──┐     ┌──┴──┐       ┌──┴──┐     ┌──┴──┐
    │ 3V3 │   │ VDD │     │ VDD │       │ VDD │     │ VDD │
    │     │   │     │     │     │       │     │     │     │
    │ESP  │   │DS1  │     │DS2  │       │DS.. │     │DS.. │
    │32C6 │   │8B20 │     │8B20 │       │8B20 │     │8B20 │
    │     │   │ #1  │     │ #2  │       │ #n  │     │ #m  │
    │ D4  ├───┤DATA │─────┤DATA │───────┤DATA │─────┤DATA │
    │     │   │     │     │     │       │     │     │     │
    │ GND ├───┤ GND │─────┤ GND │───────┤ GND │─────┤ GND │
    └─────┘   └─────┘     └─────┘       └─────┘     └─────┘
       │         │            │             │           │
       └─────────┴────────────┴─────────────┴───────────┘
                              │
                             GND
```

## DS18B20 Pinout (TO-92 balenie)

```
      ┌─────┐
      │  ─  │  (pohľad spredu, plocha smerom k vám)
      └──┬──┘
         │
    ┌────┼────┐
    │    │    │
   GND  DATA VDD
   (1)  (2)  (3)
```

**Piny DS18B20:**
1. **GND** - Zem (čierny vodič)
2. **DATA** - Dátový pin OneWire (žltý vodič)
3. **VDD** - Napájanie 3.0-5.5V (červený vodič)

## Farby vodičov (štandard pre DS18B20 vodomerne káble)

- **Červený** = VDD (3.3V)
- **Čierny** = GND
- **Žltý** = DATA (OneWire)

## Alternatívne zapojenie - Parasite Power Mode

V tomto režime sú VDD a GND spojené (šetrí jeden vodič):

```
    ┌─────┐
    │ESP  │          ┌──────┐         ┌──────┐
    │32C6 │          │DS1   │         │DS2   │
    │     │          │8B20  │         │8B20  │
    │     │   4.7kΩ  │      │         │      │
    │ 3V3 ├────┬─────┤VDD   │   ┌─────┤VDD   │
    │     │    │     │      │   │     │      │
    │ D4  ├────┼─────┤DATA  ├───┼─────┤DATA  │
    │     │    │     │      │   │     │      │
    │ GND ├────┴─────┤GND   ├───┴─────┤GND   │
    └─────┘          └──────┘         └──────┘
```

**Poznámka:** V parasite power mode je stále potrebný pull-up rezistor!

## Výber antény (voliteľné)

Ak chcete použiť externú anténu:

```
    ┌─────────────────┐
    │  ESP32-C6       │
    │                 │
    │  GPIO3  ────────┼──── LOW (enable RF switch)
    │                 │
    │  GPIO14 ────────┼──── HIGH (select external antenna)
    │                 │          LOW  (select internal antenna - default)
    │                 │
    │  [U.FL konektor]│──── Externá anténa
    └─────────────────┘
```

## Kompletné zapojenie pre testovanie

### Potrebný materiál:
- 1× Seeed Studio XIAO ESP32-C6
- 2× DS18B20 (TO-92 alebo káblová verzia)
- 1× Rezistor 4.7kΩ
- 1× Breadboard
- Vodiče (Male-Male)
- USB-C kábel

### Postup zapojenia:

1. **Umiestnite XIAO ESP32-C6 na breadboard**

2. **Pripojte napájanie:**
   ```
   ESP32-C6 pin 3V3 → + rail breadboardu (červená)
   ESP32-C6 pin GND → - rail breadboardu (modrá/čierna)
   ```

3. **Pridajte pull-up rezistor:**
   ```
   4.7kΩ medzi + rail a GPIO5 (D4)
   ```

4. **Pripojte prvý DS18B20:**
   ```
   DS18B20 #1 VDD  → + rail
   DS18B20 #1 DATA → GPIO5 (D4)
   DS18B20 #1 GND  → - rail
   ```

5. **Pripojte druhý DS18B20:**
   ```
   DS18B20 #2 VDD  → + rail
   DS18B20 #2 DATA → GPIO5 (D4) (paralelne s DS1)
   DS18B20 #2 GND  → - rail
   ```

6. **Pripojte USB-C kábel k PC**

## Elektrické parametre

| Parameter | Min | Typ | Max | Jednotka |
|-----------|-----|-----|-----|----------|
| Napájacie napätie (VDD) | 3.0 | 3.3 | 5.5 | V |
| Prúdový odber (aktívny) | - | 1 | 1.5 | mA |
| Prúdový odber (idle) | - | 1 | - | µA |
| Teplota prevádzky | -55 | - | +125 | °C |
| Presnosť merania | - | ±0.5 | - | °C |
| Dĺžka kábla (OneWire) | - | - | 30 | m |
| Pull-up rezistor | 2.2 | 4.7 | 10 | kΩ |

## Odporúčania pre dlhé káble

Pre dĺžky kábla nad 3 metre:

1. **Použite kvalitný tienený kábel** (napr. CAT5e)
2. **Znížte pull-up rezistor** na 2.2kΩ
3. **Pridajte 100nF kondenzátor** medzi VDD a GND pri každom senzore
4. **Znížte rýchlosť OneWire** ak sa vyskytujú chyby

## Problémy a riešenia

### Senzory sa nenachádzajú:
- ✓ Skontrolujte všetky tri vodiče (VDD, DATA, GND)
- ✓ Overte pull-up rezistor (4.7kΩ)
- ✓ Použite multimeter na overenie napätia na VDD (3.3V)

### Nestabilné merania:
- ✓ Pridajte 100nF kondenzátor medzi VDD a GND
- ✓ Skráťte káble
- ✓ Použite tienený kábel

### Nájdený iba jeden senzor:
- ✓ Skontrolujte zapojenie druhého senzora
- ✓ Vymeňte senzory na test funkčnosti
- ✓ Overte že DATA piny sú paralelne spojené

---

**Poznámka:** Toto zapojenie je určené pre Seeed Studio XIAO ESP32-C6. Pri použití iného ESP32 modulu overte GPIO mapovanie!
