#pragma GCC optimize("align-functions=8")
#pragma GCC diagnostic ignored "-Wunused-function"

#define FUNC_DECL __attribute__((section("code")))
#define FUNC_DEF FUNC_DECL

// branch code
#define FUNC_DECL_NONSTATIC __attribute__((section("bcode")))
#define FUNC_DEF_NONSTATIC FUNC_DECL_NONSTATIC

#define LOGGING_ENABLED 1

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef uint64_t size_t;

typedef uint64_t uintptr_t;

#define NULL 0

#define sync()                 \
    {                          \
        asm volatile("eieio"); \
        asm volatile("isync"); \
        asm volatile("sync");  \
    }

FUNC_DEF_NONSTATIC void HW_Init()
{
    register uint64_t lr asm("r9");
    asm volatile("mflr %0" : "=r"(lr)::);

    register uint64_t data_start asm("r10");
    asm volatile("bl 4");
    asm volatile("mflr %0" : "=r"(data_start)::);
    asm volatile("b jump");

    // [0] = 0x4B00000000
    // [1] = 0x9C30104000000000
    // [2] = 0x9E30100000000000
    // [3] = 0x3F0000000000
    // [4] = 0x803F0000000000
    // [5] = 0x0
    // [6] = 0x1400000000
    // [7] = 0x0

    asm volatile(".quad 0x4B00000000");
    asm volatile(".quad 0x9C30104000000000");
    asm volatile(".quad 0x9E30100000000000");
    asm volatile(".quad 0x3F0000000000");
    asm volatile(".quad 0x803F0000000000");
    asm volatile(".quad 0x0");
    asm volatile(".quad 0x1400000000");
    asm volatile(".quad 0x0");

    // [8] = 0x6
    // [9] = 0xF8040000
    // [10] = 0x0
    // [11] = 0x9000000000000000

    asm volatile(".quad 0x6");
    asm volatile(".quad 0xF8040000");
    asm volatile(".quad 0x0");
    asm volatile(".quad 0x9000000000000000");

    // [12] = 0x7FFFFFFF
    // [13] = 0x0
    // [14] = 0x1001400000000

    asm volatile(".quad 0x7FFFFFFF");
    asm volatile(".quad 0x0");
    asm volatile(".quad 0x1001400000000");

    asm volatile("jump:");
    data_start += 8;

    // r3 = 6; [8]
    asm volatile("ld 3, %0(%1)" ::"i"(8 * 8), "r"(data_start) :);
    sync();
    // mtspr lpcr, r3
    asm volatile(".long 0x7C7E4BA6");
    sync();

#if 1

    // slbia
    asm volatile("slbia");
    sync();

    // r4 = 0x9C30104000000000; // [1]
    asm volatile("ld 4, %0(%1)" ::"i"(1 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    asm volatile(".long 0x7C91FBA6");
    sync();

    // r4 = 0x3F0000000000; // [3]
    asm volatile("ld 4, %0(%1)" ::"i"(3 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    asm volatile(".long 0x7C94FBA6");
    sync();

    // r4 = 0x9E30100000000000; // [2]
    asm volatile("ld 4, %0(%1)" ::"i"(2 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    asm volatile(".long 0x7C91FBA6");
    sync();

    // r4 = 0x803F0000000000; // [4]
    asm volatile("ld 4, %0(%1)" ::"i"(4 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    asm volatile(".long 0x7C94FBA6");
    sync();

    // r4 = 0x9C30104000000000; // [1]
    asm volatile("ld 4, %0(%1)" ::"i"(1 * 8), "r"(data_start) :);
    sync();
    // mtspr hid1, r4
    asm volatile(".long 0x7C91FBA6");
    sync();

    // r4 = 0x3F0000000000; // [3]
    asm volatile("ld 4, %0(%1)" ::"i"(3 * 8), "r"(data_start) :);
    sync();
    // mtspr hid4, r4
    asm volatile(".long 0x7C94FBA6");
    sync();

    // r4 = 0x4B00000000; // [0]
    asm volatile("ld 4, %0(%1)" ::"i"(0 * 8), "r"(data_start) :);
    sync();
    // mtspr hid0, r4
    asm volatile(".long 0x7C90FBA6");
    sync();

    // r4 = 0x0; // [5]
    asm volatile("ld 4, %0(%1)" ::"i"(5 * 8), "r"(data_start) :);
    sync();
    // mtspr hid5, r4
    asm volatile(".long 0x7C96FBA6");
    sync();

    // r4 = 0x1400000000; // [6]
    asm volatile("ld 4, %0(%1)" ::"i"(6 * 8), "r"(data_start) :);
    sync();
    // mtspr hid6, r4
    asm volatile(".long 0x7C99FBA6");
    sync();

    // r4 = 0x0; // [7]
    asm volatile("ld 4, %0(%1)" ::"i"(7 * 8), "r"(data_start) :);
    sync();
    // mtspr hid7, r4
    asm volatile(".long 0x7C9AFBA6");
    sync();

    // r4 = 0xF8040000; [9]
    asm volatile("ld 4, %0(%1)" ::"i"(9 * 8), "r"(data_start) :);
    sync();
    // mtspr tscr, r4
    asm volatile(".long 0x7C99E3A6");
    sync();

    // r4 = 0x0; [10]
    asm volatile("ld 4, %0(%1)" ::"i"(10 * 8), "r"(data_start) :);
    sync();
    // mtspr lpidr, r4
    asm volatile(".long 0x7C9F4BA6");
    sync();

    // r3 = 0x9000000000000000; [11]
    asm volatile("ld 3, %0(%1)" ::"i"(11 * 8), "r"(data_start) :);
    sync();
    // mtmsrd r3
    asm volatile(".long 0x7C600164");
    sync();

#endif

    //

#if 1

    // r4 = 0x7FFFFFFFLL
    asm volatile("ld 4, %0(%1)" ::"i"(12 * 8), "r"(data_start) :);
    sync();
    asm volatile("mtdec 4");
    sync();

    // r0 = 0x0
    asm volatile("ld 0, %0(%1)" ::"i"(13 * 8), "r"(data_start) :);
    sync();
    // mtspr tblw, r0
    asm volatile(".long 0x7C1C43A6");
    sync();
    // mtspr tbuw, r0
    asm volatile(".long 0x7C1D43A6");
    sync();
    // mtspr tblw, r0
    asm volatile(".long 0x7C1C43A6");
    sync();

    // r0 = 0x1001400000000
    asm volatile("ld 0, %0(%1)" ::"i"(14 * 8), "r"(data_start) :);
    sync();
    // mtspr hid6, r0
    asm volatile(".long 0x7C19FBA6");
    sync();

#endif

    // asm volatile("b DBG_RET");

    asm volatile("mtlr %0" : "=r"(lr)::);
}

FUNC_DEF void dead()
{
    while (1)
    {
    }
}

FUNC_DEF void memset(void *buf, uint8_t v, uint64_t count)
{
    uint8_t *buff = (uint8_t *)buf;

    for (uint64_t i = 0; i < count; ++i)
        buff[i] = v;
}

FUNC_DEF void memcpy(void *dest, const void *src, uint64_t count)
{
    uint8_t *destt = (uint8_t *)dest;
    const uint8_t *srcc = (const uint8_t *)src;

    for (uint64_t i = 0; i < count; ++i)
        destt[i] = srcc[i];
}

FUNC_DEF uint8_t memcmp(const void *p1, const void *p2, uint64_t count)
{
    const uint8_t *pp1 = (const uint8_t *)p1;
    const uint8_t *pp2 = (const uint8_t *)p2;

    for (uint64_t i = 0; i < count; ++i)
    {
        if (pp1[i] != pp2[i])
            return 1;
    }

    return 0;
}

FUNC_DEF uint64_t strlen(const char *str)
{
    uint64_t len = 0;

    while (1)
    {
        if (str[len] == 0)
            break;

        ++len;
    }

    return len;
}

FUNC_DEF uint8_t strcmp(const char *str1, const char *str2)
{
    uint64_t str1_len = strlen(str1);
    uint64_t str2_len = strlen(str2);

    if (str1_len != str2_len)
        return 1;

    for (uint64_t i = 0; i < str2_len; ++i)
    {
        if (str1[i] != str2[i])
            return 1;
    }

    return 0;
}

#if LOGGING_ENABLED

FUNC_DEF void sb_putc(char c)
{
    const volatile uint32_t* status = (const volatile uint32_t*)0x24000FFF308;
    while ((*status & 0x100) == 0) {}

    volatile uint32_t* putc = (volatile uint32_t*)0x24000FFF31C;
    *putc = c;
}

FUNC_DEF void sb_puts(const char* str)
{
    while (*str)
    {
        sb_putc(*str);
        ++str;
    }
}

FUNC_DEF void sb_print_decimal(uint64_t v)
{
    const char* charArr = "0123456789abcdef";

    char buf[64];
    uint64_t curBufLen = 0;

    uint64_t base = 10;

    {
        uint64_t resultX;
        uint64_t resultY;

        resultX = v;
        resultY = 0;

        while (resultX > 0)
        {
            resultY = resultX % base;
            resultX = resultX / base;

            buf[curBufLen] = charArr[resultY];
            ++curBufLen;
        }
    }

    char outBuf[64];
    uint64_t curOutBufLen = 0;

    for (uint64_t i = 0; i < curBufLen; i++)
        outBuf[(curOutBufLen + i)] = buf[((curBufLen - 1) - i)];

    curOutBufLen += curBufLen;

    if (curOutBufLen == 0)
    {
        outBuf[curOutBufLen] = '0';
        ++curOutBufLen;
    }

    // outBuf[curOutBufLen] = '\n';
    //++curOutBufLen;

    outBuf[curOutBufLen] = 0;
    sb_puts(outBuf);
}

FUNC_DEF void sb_do_print_hex(uint64_t v, uint8_t prefix, uint64_t leadingZeros)
{
    const char* charArr = "0123456789abcdef";

    char buf[64];
    uint64_t curBufLen = 0;

    uint64_t base = 16;

    {
        uint64_t resultX;
        uint64_t resultY;

        resultX = v;
        resultY = 0;

        while (resultX > 0)
        {
            resultY = resultX % base;
            resultX = resultX / base;

            buf[curBufLen] = charArr[resultY];
            ++curBufLen;
        }
    }

    char outBuf[64];
    uint64_t curOutBufLen = 0;

    for (uint64_t i = 0; i < curBufLen; i++)
        outBuf[(curOutBufLen + i)] = buf[((curBufLen - 1) - i)];

    curOutBufLen += curBufLen;

    if (prefix)
        sb_puts("0x");

    if (curBufLen < leadingZeros)
    {
        for (uint64_t i = 0; i < (leadingZeros - curBufLen); ++i)
            sb_puts("0");
    }

    outBuf[curOutBufLen] = 0;
    sb_puts(outBuf);
}

FUNC_DEF void sb_print_hex(uint64_t v)
{
    sb_do_print_hex(v, 1, 1);
}

FUNC_DEF void sb_hexdump(const void* ptr, uint64_t sz)
{
    if (sz == 0)
        return;

    const uint8_t* buf = (const uint8_t*)ptr;

    uint64_t col = 32;
    uint64_t i = 0;

    while (1)
    {
        uint8_t v = buf[i];
        sb_do_print_hex(v, 0, 2);

        ++i;

        if (i == sz)
            break;

        if ((i % col) == 0)
        {
            sb_puts("\n");
            continue;
        }

        sb_puts(" ");
    }
}

#endif

#if LOGGING_ENABLED
#define puts sb_puts
#define print_decimal sb_print_decimal
#define do_print_hex sb_do_print_hex
#define print_hex sb_print_hex
#define hexdump sb_hexdump
#else
#define puts(...)
#define print_decimal(...)
#define do_print_hex(...)
#define print_hex(...)
#define hexdump(...)
#endif

// initial crc should be 0
FUNC_DEF uint32_t qcfw_crc32c(uint32_t crc, const uint8_t* buf, uint64_t len)
{
    int32_t k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return ~crc;
}

FUNC_DEF void Stagexldr()
{
    puts("\nHello from Stagexldr!!\n");

    {
        uint32_t payload_size = *((const volatile uint32_t*)0xff8);
        uint32_t payload_crc32 = *((const volatile uint32_t*)0xffc);

        puts("payload_size = ");
        print_decimal(payload_size);
        puts("\n");

        puts("payload_crc32 = ");
        print_hex(payload_crc32);
        puts("\n");

        if ((payload_size > (1 * 1024 * 1024)) || (payload_crc32 == 0))
        {
            puts("bad!!!\n");
            dead();
        }

        uint32_t crc32 = qcfw_crc32c(0, (const uint8_t*)0x1000, payload_size);

        if (crc32 != payload_crc32)
        {
            puts("bad crc!!!\n");
            dead();
        }

        puts("crc32 check ok!\n");
    }

    dead();
}

__attribute__((section("main"))) void stagexldr_main()
{
    Stagexldr();

    dead();
}

__attribute__((noreturn, section("entry"))) void stagexldr_entry()
{
    // .toc
    asm volatile("li %r2, 0x1100");

    // += 0x8000
    asm volatile("addi %r2, %r2, 0x7fff");
    asm volatile("addi %r2, %r2, 1");

    // set sp to 0x2000000
    asm volatile("lis %r1, 0x200");

    // init ppu
    asm volatile("bl HW_Init");

    //
    asm volatile("b stagexldr_main");

    //
    __builtin_unreachable();
}

void stagexldr_link_entry()
{
    asm volatile("bl stagexldr_entry");
}