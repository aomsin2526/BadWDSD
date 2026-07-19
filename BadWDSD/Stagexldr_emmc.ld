SECTIONS
{
    . = 0x1000;
    entry : { *(entry) }

    . = 0x1100;
    .toc : { *(.toc) }

    . = 0x1120;
    bcode : { *(bcode) }
    main : { *(main) }
    code : { *(code) }

    .rodata : { *(.rodata) }
    .text : { *(.text) }

    .opd : { *(.opd) }
}