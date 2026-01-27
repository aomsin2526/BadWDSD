# BadWDSD

This is a hardware modchip for Sony PlayStation 3. By using Raspberry Pi Pico (RP2040), It is possible for non-CFW compatible models to boot qCFW.

# Supported models

All **CECH-2500**

All **CECH-3000**

**CECH-4x00** with **NOR** flash

<img width="284" height="370" alt="firefox_Z4WaABYPQH" src="https://github.com/user-attachments/assets/7066c760-a097-45ba-9697-6022c9cf1e07" />

**CECH-4x00** with **eMMC** flash is **NOT** supported

<img width="220" height="285" alt="firefox_LGBpLg82NH" src="https://github.com/user-attachments/assets/6592b99e-f80f-4319-a450-10a894aa5164" />

# What is qCFW?

You still can't install CFW PUP, so new variant of CFW must be made. This is called **quasi-CFW**.

It is heavily based from **Evilnat 4.92.2 PEX CFW**. And will support every feature except: **Dumping eid_root_key and everything that needed it**.

Cobra must be active at all times or some feature will not work properly.

# qCFW quirks

For some unknown reason, When you turn on the console using wireless controller it won't sync. You must power cycle the controller for it to sync.

# Note on DEX mode

DEX mode is fully supported. But any kind of firmware installation or update is not possible while in this mode.

This means if you somehow need to reinstall the firmware such as corrupted HDD, you are stuck.

To recover, use **BANKSEL** pin on the modchip to go back to OFW.

# Installation (Hardware)

Currently, **Raspberry Pi Pico (RP2040)** and **RP2040-Zero** is supported.

<img width="1100" height="800" alt="raspberry_pi_pico_pinout - Copy" src="https://github.com/user-attachments/assets/e1393136-d60f-4822-a818-f27cf2b1456b" />

![900px-RP2040-Zero-details-7](https://github.com/user-attachments/assets/8304c258-386b-4f2c-84ee-5fd5f6f90217)

