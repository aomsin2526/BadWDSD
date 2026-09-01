//

FUNC_DEF uint64_t msr_load()
{
    uint64_t v;
    ASM("mfmsr %0":"=r"(v)::);

    return v;
}

FUNC_DEF void msr_store(uint64_t v)
{
    ASM("mtmsrd %0"::"r"(v):);
}

FUNC_DEF uint64_t intr_disable()
{
    uint64_t old_val = msr_load();

    uint64_t new_val = old_val;
    new_val &= ~(1ULL << 15); // EE
    msr_store(new_val);

    return old_val;
}

//

FUNC_DEF_BCODE void HW_Init()
{
    register uint64_t lr asm("r9");
    ASM("mflr %0" : "=r"(lr)::);

    register uint64_t data_start asm("r10");
    ASM("bl 4");
    ASM("mflr %0" : "=r"(data_start)::);
    ASM("b jump");

    // [0] = 0x4B00000000
    // [1] = 0x9C30104000000000
    // [2] = 0x9E30100000000000
    // [3] = 0x3F0000000000
    // [4] = 0x803F0000000000
    // [5] = 0x0
    // [6] = 0x1400000000
    // [7] = 0x0

    ASM(".quad 0x4B00000000");
    ASM(".quad 0x9C30104000000000");
    ASM(".quad 0x9E30100000000000");
    ASM(".quad 0x3F0000000000");
    ASM(".quad 0x803F0000000000");
    ASM(".quad 0x0");
    ASM(".quad 0x1400000000");
    ASM(".quad 0x0");

    // [8] = 0x6
    // [9] = 0xF8040000
    // [10] = 0x0
    // [11] = 0x9000000000000000

    ASM(".quad 0x6");
    ASM(".quad 0xF8040000");
    ASM(".quad 0x0");
    ASM(".quad 0x9000000000000000");

    // [12] = 0x7FFFFFFF
    // [13] = 0x0
    // [14] = 0x1001400000000

    ASM(".quad 0x7FFFFFFF");
    ASM(".quad 0x0");
    ASM(".quad 0x1001400000000");

    ASM("jump:");
    data_start += 8;

    // r3 = 6; [8]
    ASM("ld 3, %0(%1)" ::"i"(8 * 8), "r"(data_start) :);
    sync();
    // mtspr lpcr, r3
    ASM(".long 0x7C7E4BA6");
    sync();

    // slbia
    ASM("slbia");
    sync();

    // r4 = 0x9C30104000000000; // [1]
    ASM("ld 4, %0(%1)" ::"i"(1 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    ASM(".long 0x7C91FBA6");
    sync();

    // r4 = 0x3F0000000000; // [3]
    ASM("ld 4, %0(%1)" ::"i"(3 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    ASM(".long 0x7C94FBA6");
    sync();

    // r4 = 0x9E30100000000000; // [2]
    ASM("ld 4, %0(%1)" ::"i"(2 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    ASM(".long 0x7C91FBA6");
    sync();

    // r4 = 0x803F0000000000; // [4]
    ASM("ld 4, %0(%1)" ::"i"(4 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    ASM(".long 0x7C94FBA6");
    sync();

    // r4 = 0x9C30104000000000; // [1]
    ASM("ld 4, %0(%1)" ::"i"(1 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    ASM(".long 0x7C91FBA6");
    sync();

    // r4 = 0x3F0000000000; // [3]
    ASM("ld 4, %0(%1)" ::"i"(3 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    ASM(".long 0x7C94FBA6");
    sync();

    // r4 = 0x4B00000000; // [0]
    ASM("ld 4, %0(%1)" ::"i"(0 * 8), "r"(data_start) :);
    sync();
    // mtspr hid0, r4
    ASM(".long 0x7C90FBA6");
    sync();

    // r4 = 0x0; // [5]
    ASM("ld 4, %0(%1)" ::"i"(5 * 8), "r"(data_start) :);
    sync();
    // mtspr hid5, r4
    ASM(".long 0x7C96FBA6");
    sync();

    // r4 = 0x1400000000; // [6]
    ASM("ld 4, %0(%1)" ::"i"(6 * 8), "r"(data_start) :);
    sync();
    // mtspr hid6, r4
    ASM(".long 0x7C99FBA6");
    sync();

    // r4 = 0x0; // [7]
    ASM("ld 4, %0(%1)" ::"i"(7 * 8), "r"(data_start) :);
    sync();
    // mtspr hid7, r4
    ASM(".long 0x7C9AFBA6");
    sync();

    // r4 = 0xF8040000; [9]
    ASM("ld 4, %0(%1)" ::"i"(9 * 8), "r"(data_start) :);
    sync();
    // mtspr tscr, r4
    ASM(".long 0x7C99E3A6");
    sync();

    // r4 = 0x0; [10]
    ASM("ld 4, %0(%1)" ::"i"(10 * 8), "r"(data_start) :);
    sync();
    // mtspr lpidr, r4
    ASM(".long 0x7C9F4BA6");
    sync();

    // r3 = 0x9000000000000000; [11]
    ASM("ld 3, %0(%1)" ::"i"(11 * 8), "r"(data_start) :);
    sync();
    // mtmsrd r3
    ASM(".long 0x7C600164");
    sync();

    // r4 = 0x7FFFFFFFLL
    ASM("ld 4, %0(%1)" ::"i"(12 * 8), "r"(data_start) :);
    sync();
    ASM("mtdec 4");
    sync();

    // r0 = 0x0
    ASM("ld 0, %0(%1)" ::"i"(13 * 8), "r"(data_start) :);
    sync();
    // mtspr tblw, r0
    ASM(".long 0x7C1C43A6");
    sync();
    // mtspr tbuw, r0
    ASM(".long 0x7C1D43A6");
    sync();
    // mtspr tblw, r0
    ASM(".long 0x7C1C43A6");
    sync();

    // r0 = 0x1001400000000
    ASM("ld 0, %0(%1)" ::"i"(14 * 8), "r"(data_start) :);
    sync();
    // mtspr hid6, r0
    ASM(".long 0x7C19FBA6");
    sync();

    ASM("mtlr %0" : "=r"(lr)::);
}

//