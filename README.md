# BadWDSD

![firefox_A71vZ02dDF](https://github.com/user-attachments/assets/0287c52b-2bc0-4ac3-ac79-802790cfb90b)

This is a **hardware modchip** for Sony Playstation 3. By abusing a "feature" called **WDSD** serial register inside XDR ram. We can override what data to be written to memory through serial pin (32 bytes max). But not where and when.
So if we do it while first boot loader (bootldr/lv0ldr) is decrypting lv0 (second boot loader) and writing decrypted data to memory. In reality, our code will be written to memory instead.
If it hit address **0x100 (Reset vector)**, when PPU core starts our code will get executed instead. Gaining custom code execution very early on.

It is patchable by Sony with new hardware by simply read and verify the data after write. But it is too late for them now since ps3 is no longer made.

I avoid calling this modchip a "glitch" because XDR ram itself is working perfectly as intended. Every command we send to it is valid. WDSD register are for initialization purpose. But we "abuse" it to do benefit thing for us.

This modchip replace and unrelated to previous [BadHTAB](https://github.com/aomsin2526/BadHTAB) exploit. While both are exploiting XDR ram. Method it uses is completely different.

This modchip then allow full access to lv1 hypervisor and ability to run new variant of persistence CFW called **qCFW**. Unlock many features of CFW that HEN can't do.
It can also used to recover console such as exit FSM, or updating consoles with dead BD/BT module. Downgrading also possible.

With proper solder and wiring, this modchip is very stable **(100% success rate)** and should get you to XMB within 30 secs.

This exploit contain three major components:
  * **BadWDSD** - Hardware modchip. Raspberry Pi Pico (RP2040) based. It handles WDSD register writing part. 32 bytes long jump code (Stage0.S) to address **0x2401F031000** will be pushed to memory by it.
  * **Stagex.bin** - Main software payload of this modchip. It will be stored at address **0x31000 (NOR flash) / 0x2401F031000 (MMIO)**
  * **BadWDSD-SW** - Software utilites, was used for Stagex.bin/CoreOS.bin installation (now legacy). Now only used to boot OtherOS.

# Supported models

All 2500/3000 Slim.

4000 Superslim with **NOR** flash.

**4000 Superslim with eMMC flash (12GB) is NOT supported.**

# Operation Mode

This modchip supports two operation mode: **OFW** and **qCFW**

# OFW Mode

This is pretty much **HEN + lv1 peek/poke**. It is very safe since it doesn't modify any files. So you can just disable modchip and console will work just like before. But feature will be more limited than qCFW. And you will still need HEN to be useful. This mode is required to install qCFW so everyone must start here. You can downgrade/exit FSM/Overclock/OtherOS under this mode.

**Supports firmware 4.70 or later**

You only need **Stagex.bin** to run this mode, since Stagex.bin stay even after firmware update, you only need to install it once.

You can take Stagex.bin from any qCFW releases.

# qCFW Mode

You can't install any existing CFW or any custom PUP, so new variant of CFW must be made. This variant will be called qCFW (quasi-CFW). It is persistence, stable and much more powerful than HEN/OFW Mode.
It use fork of Cobra called [Cobra-qCFW](https://github.com/aomsin2526/Cobra-PS3-qCFW). Vanilla Cobra won't work here! Installation method also different.

Once installed, modchip must be active at all times until you reinstall OFW again.

At its peak, it should be capable of everything CFW can except one thing: **Dump eid_root_key**.

It's not at its peak yet at current state, but it is powerful and stable enough for daily uses.

**Quirks:**
  * Cobra must be active at all times no matter what, if you lost it by some reason, it can be loaded from USB.
  * PS1 Emu can't be modified
  * **VSH modules (self/sprx) at /dev_flash/vsh/module/** can't be modified, Don't modify these files or you will brick
  * If you turn on your console through PS button on your controller, it won't sync until you power cycle the controller by holding PS button until it turn off, then press it to turn on again. This is caused by load Cobra from USB.

**Stagex.bin and CoreOS.bin** will be released in pair. **So ensure that you install both of them and they're matched!**
