SECTIONS
{
    . = 0x11000;
    entry : { *(entry) }

    . = 0x11100;
    .toc : { *(.toc) }

    . = 0x11120;
    bcode : { *(bcode) }
    main : { *(main) }
    code : { *(code) }

    .text : { *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }

    .toc1 : { *(.toc1) }

    .opd : { *(.opd) }
}