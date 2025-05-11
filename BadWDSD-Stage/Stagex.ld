SECTIONS
{
    . = 0x2401F031000;
    entry1 : { *(entry1) }

    . = 0x2401F031100;
    entry2 : { *(entry2) }

    . = 0x2401F031200;
    entry3 : { *(entry3) }

    . = 0x2401F031300;
    entry5 : { *(entry5) }

    . = 0x2401F031800;
    .toc : { *(.toc) }

    . = 0x2401F032300;
    bcode : { *(bcode) }

    //. = 0x2401F033000;
    main1 : { *(main1) }

    //. = 0x2401F033100;
    main2 : { *(main2) }

    //. = 0x2401F033200;
    main3 : { *(main3) }

    //. = 0x2401F033500;
    main5 : { *(main5) }

    code : { *(code) }

    .text : { *(.text) }

    .opd : { *(.opd) }
}