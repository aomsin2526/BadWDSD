SECTIONS
{
    . = 0x2401FF21000;
    entry1 : { *(entry1) }

    . = 0x2401FF21100;
    entry2 : { *(entry2) }

    . = 0x2401FF21200;
    entry3 : { *(entry3) }

    . = 0x2401FF21400;
    entry5 : { *(entry5) }

    . = 0x2401FF21700;
    entry6 : { *(entry6) }

    . = 0x2401FF21900;
    .toc : { *(.toc) }

    . = 0x2401FF21908;
    signature :
    {
        QUAD(0x5446072c5516c2c6)
    }

    . = 0x2401FF21910;
    tocinfo :
    {
        QUAD(LOADADDR(.toc1))
        QUAD(SIZEOF(.toc1))
    }

    . = 0x2401FF21920;
    bcode : { *(bcode) }

    main1 : { *(main1) }
    main2 : { *(main2) }
    main3 : { *(main3) }
    main5 : { *(main5) }
    main6 : { *(main6) }

    code : { *(code) }

    .rodata : { *(.rodata) }
    .text : { *(.text) }

    .opd : { *(.opd) }
}