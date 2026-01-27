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

# Installation (Software)

**FOR FIRST INSTALLATION, BACKUP FLASH FIRST!!!. IF SOMETHINGS GOES TOO WRONG AND YOU DONT HAVE BACKUP, YOUR CONSOLE MAY BE PERMANENTLY BRICKED**

1. Prepare the USB drive by **DELETING old qcfw folder if existed, DO NOT OVERWRITE!!**
then download [qCFW](https://github.com/aomsin2526/BadWDSD/releases) and extract it into your drive like this:

<img width="617" height="174" alt="explorer_71wt3KBo5T" src="https://github.com/user-attachments/assets/b63da1b3-3982-4703-b07b-8ae8b209349a" />

2. Install HEN version that include qCFW installer
3. Plug your USB drive into **RIGHTMOST** USB port of your ps3
4. On XMB, Enable HEN then use **Network -> Hybrid Firmware Tools -> QCFW Installer -> Install Stagex** option. It must show **Success**
5. If not already, Install the modchip by following **Installation (Hardware)** section
6. After modchip installed and power plugged in, wait until LED of modchip becomes solid. If it doesn't solid after a while, check **SC_RX/SC_TX** wire
7. Turn on the console, modchip LED should flash briefly with triple beep right after. This means exploit is successful. If your console keep turning off and on, check **CMD/CLK** wire and **Stagex.bin**
8. You should be on XMB now, now Enable HEN then use **Install qCFW** option
9. If it tell you to reinstall and try again, do it **ONCE**.
10. Your screen will appear frozen, it is installing. This process take 10-20 minutes. If something goes wrong during this step, you should be still able to recover by entering safe mode and reinstall firmware normally
11. Then it will reboot itself, you should be on qCFW and see Evilnat logo now.
12. Congrats! qCFW installation is complete

From now on, modchip will be required to boot the console until you go back to OFW again

This can be done by reinstalling OFW/HFW firmware normally. Then after this you can disable or uninstall the modchip

If thing goes too bad to the point of not being able to enter safe mode at all, you can use **BANKSEL** pin instead.

# Installation (Hardware)

Currently, **Raspberry Pi Pico (RP2040)** and **RP2040-Zero** is supported.

**Only install modchip after Stagex is installed to console flash from above section. Otherwise it won't boot, if you already installed the modchip. You can use HOLD pin to temporary disable the modchip without unsoldering it.**

<details>
  <summary> <b>Pico</b> </summary>
<p>
<img width="1100" height="800" alt="raspberry_pi_pico_pinout - Copy" src="https://github.com/user-attachments/assets/e1393136-d60f-4822-a818-f27cf2b1456b" />
</p>
</details>

<details>
  <summary> <b>RP2040-Zero</b> </summary>
<p>
<img src="https://github.com/user-attachments/assets/8304c258-386b-4f2c-84ee-5fd5f6f90217" />
</p>
</details>

<details>
  <summary> <b>3000</b> </summary>
<p>
<img src="https://github.com/user-attachments/assets/6787886e-58a6-4fe9-877c-7ce4efbf8af7" />
</p>
</details>

<details>
  <summary> <b>4x00</b> </summary>
<p>
<img src="https://github.com/user-attachments/assets/9910be97-5c85-4b48-9edb-c2d7a4ecabd9" />

<img width="481" height="384" alt="firefox_hjbEN8ZhUV" src="https://github.com/user-attachments/assets/fb19f60f-76ee-4e76-a164-83b988cdf286" />

<img width="501" height="400" alt="firefox_ybeL3zep1j" src="https://github.com/user-attachments/assets/71580063-2a03-4b6e-8433-e5f99e925e89" />
</p>
</details>

To flash .uf2 file (it is included in qCFW zip), simply connect modchip USB port into your PC while pressing **BOOTSEL** button. Then new drive will appear, simply drag .uf2 file into it.

You should see LED blinking. Flash successful and ready to use. You can disconnect it from your PC.

Exclude power and ground, you only need to solder 4 wires that marked red **(CMD, CLK, SC_RX, SC_TX)**. Other pin is optional.

It is possible to power the modchip using external power as long as it is active during ps3 standby

# Pin description

**SIGNAL PIN:**

**CLK** - XDR CLK signal

**CMD** - XDR CMD signal

**SC_TX/SC_RX** - Syscon UART signal

**DEBUG** - Optional modchip UART signal, for debugging and accessing syscon **(baud 576000, NOT 57600!)**


**CONFIG PIN:**

Short to ground to activate

**HOLD** - Disable the modchip without needing to remove power or unsolder

**LITE** - TODO

**BANKSEL** - Go back to OFW forcefully. It is equal to syscon command **w 1224 00**. Only use when absolutely needed. You can't turn on the console while this pin is shorted

# Update qCFW

You can't update qCFW while on qCFW. you must go back to OFW first.

Simply reinstall firmware normally, then use **Install qCFW** with updated files on USB again. No need to do anything else

# Go back to OFW using PUP method (Recommended)

Always use this method when possible. Simply reinstall firmware as normal. No extra steps required.

If you want to uninstall the modchip, you can do so after this

# Go back to OFW using BANKSEL pin

**Avoid this unless absolutely needed.**

1. Unplug your console
2. Short **BANKSEL** pin to ground
3. Plug in your console, wait until modchip LED flashes very fast. Then it is successful. You can't turn on the console while this pin is shorted
4. Unplug your console and unshort the pin. **If necessary** remove or use HOLD pin to disable the modchip
5. Plug in your console again and turn it on, you will likely to get black screen, this is expected since dev_flash is still qCFW but you're on OFW now
6. Enter safe mode and reinstall firmware normally to get full recovery

# Downgrading

After booting the console with modchip, It is possible to downgrade the firmware up to 4.80. It can't be done in XMB. You must use safe mode.

# OtherOS

It is different from CFW. Simply follow these steps.

1. Download [dtbImage.ps3.zfself](https://github.com/aomsin2526/ps3-petitboot-kexec-patched/releases) and put it into root of your USB drive
2. Plug your USB drive into **RIGHTMOST** USB port of your ps3
3. On XMB, use **Network -> Custom Firmware Tools -> OtherOS Tools -> Install OtherOS (qCFW) option**. It should show **Success**
4. Use **Boot OtherOS (qCFW)** option. It should enter petitboot right away

# Accessing Syscon

You can't access syscon the old ways anymore. It must be done through modchip. Simply connect **DEBUG** pin of modchip into your UART adapter.

<img width="1206" height="644" alt="Termite_s1m3OjonO8" src="https://github.com/user-attachments/assets/b8ba3786-d2ab-488f-b6c2-85032f0615de" />

# NoBT

TODO. It requires LITE pin and hardware flasher for first installation if you are already on update loop.
