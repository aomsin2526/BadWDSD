# BadWDSD

![firefox_A71vZ02dDF](https://github.com/user-attachments/assets/0287c52b-2bc0-4ac3-ac79-802790cfb90b)

This is a **hardware modchip** for Sony Playstation 3. By abusing a "feature" called **WDSD** serial register inside XDR ram. We can override what data to be written to memory through serial pin (32 bytes max). But not where and when.
So if we do it while first boot loader (bootldr/lv0ldr) is decrypting lv0 (second boot loader) and writing decrypted data to memory. In reality, our code will be written instead.
If it hit address 0x100 (Reset vector), when PPU core starts our code will get executed instead. Gaining code execution very early on.

I avoid calling this modchip a "glitch" because XDR ram itself is working perfectly as intended. Every command we send to it is valid. WDSD register are for initialization purpose. But we "abuse" it to do benefit thing for us.

This modchip replace and unrelated to previous **BadHTAB** exploit. While both are exploiting XDR ram. Method it uses is completely different.

This modchip then allow full access to lv1 hypervisor and ability to run new variant of persistence CFW called **qCFW**. Unlock many features of CFW that HEN can't do.
It can also used to recover console such as exit FSM, or updating consoles with dead BD/BT module. Downgrading also possible.

With proper solder and wiring, this modchip is very stable and should get you to XMB within 30 secs.

This exploit contain three major components:
  * **BadWDSD** - Hardware modchip. Raspberry Pi Pico (RP2040) based. It handles WDSD register writing part. 32 bytes long jump code (Stage0.S) to address **0x2401F031000** will be pushed to memory by it.
  * **Stagex.bin** - Main software payload of this modchip. It will be stored at address **0x31000 (NOR flash) / 0x2401F031000 (MMIO)**
  * **BadWDSD-SW** - Software utilites, was used for Stagex.bin/CoreOS.bin installation (now legacy). Now only used to boot OtherOS.

# Supported models

All 2500/3000 Slim.

4000 Superslim with **NOR** flash.

**4000 Superslim with eMMC flash (12GB) is NOT supported.**
