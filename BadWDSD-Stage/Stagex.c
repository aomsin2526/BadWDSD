#pragma GCC optimize("align-functions=8")
#pragma GCC diagnostic ignored "-Wunused-function"

#define FUNC_DECL __attribute__((section("code")))
#define FUNC_DEF FUNC_DECL

// branch code
#define FUNC_DECL_NONSTATIC __attribute__((section("bcode")))
#define FUNC_DEF_NONSTATIC FUNC_DECL_NONSTATIC

#define SC_PUTS_BUFFER_ENABLED 1

//#define LOGGING_ENABLED 1
//#define SC_LV1_LOGGING_ENABLED 1

//#define STAGE5_LOG_ENABLED 1

#define ZLIB_SPU_ONLY_ENABLED 1
#define STAGE0_DECRYPTLV0SELF_SPU_ENABLED 1

//#define DECRYPTLV2SELF_ENABLED 1

#if DECRYPTLV2SELF_ENABLED
#define AES_ENABLED 1
#endif

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

#define eieio()                \
    {                          \
        asm volatile("eieio"); \
        asm volatile("sync");  \
    }

#define dcbf(___in_addr_register)                                                  \
    {                                                                              \
        asm volatile("dcbf %0, %1" ::"r"(stage_zero), "r"(___in_addr_register) :); \
        asm volatile("sync");                                                      \
    }

#define sync()                 \
    {                          \
        asm volatile("eieio"); \
        asm volatile("isync"); \
        asm volatile("sync");  \
    }

FUNC_DECL void dead();

FUNC_DECL uint64_t GetTimeInNs();
FUNC_DECL void WaitInNs(uint64_t ns);

FUNC_DECL uint64_t GetTimeInUs();
FUNC_DECL void WaitInUs(uint64_t us);

FUNC_DECL uint64_t GetTimeInMs();
FUNC_DECL void WaitInMs(uint64_t ms);

FUNC_DECL_NONSTATIC uint64_t GetTimeInMs2();
FUNC_DECL_NONSTATIC void WaitInMs2(uint64_t ms);

FUNC_DECL void memset(void *buf, uint8_t v, uint64_t count);
FUNC_DECL void memcpy(void *dest, const void *src, uint64_t count);

FUNC_DECL uint64_t strlen(const char *str);

struct sc_packet_s
{
    uint8_t service_id;
    uint32_t communication_tag;
    uint16_t payload_size;

    uint8_t data[256] __attribute__((aligned(8)));
};

struct sc_real_packet_header_s
{
    uint8_t service_id;
    uint8_t version;
    uint16_t tag;
    uint8_t res[2];
    uint16_t cksum;
    uint32_t communication_tag;
    uint16_t payload_size[2];
};

FUNC_DECL uint16_t sc_real_packet_header_calc_cksum(const struct sc_real_packet_header_s *pkt_hdr);

FUNC_DECL void sc_send_packet(const struct sc_packet_s *in, struct sc_packet_s *out);

FUNC_DECL void sc_triple_beep();
FUNC_DECL void sc_continuous_beep();

FUNC_DECL void sc_soft_restart();
FUNC_DECL void sc_hard_restart();

FUNC_DECL void puts(const char *str);

FUNC_DECL void print_hex(uint64_t v);

// reserve register...

//register uint64_t r13 asm("r13");
//register uint64_t r14 asm("r14");
//register uint64_t r15 asm("r15");
//register uint64_t r16 asm("r16");

//register uint64_t r23 asm("r23");
//register uint64_t r24 asm("r24");

register uint64_t is_lv1 asm("r17"); // 0x9669, 0x9666 (stage5)
register uint64_t lv1_rtoc asm("r18");
register uint64_t lv1_sp asm("r25");

register uint64_t stage_entry_ra asm("r19");
register uint64_t stage_rtoc asm("r20");
register uint64_t stage_sp asm("r21");
register uint64_t stage_zero asm("r22");

//

struct Stagex_Context_s
{
    uint64_t cached_myappldrElfAddress;
    uint64_t cached_mylv2ldrElfAddress;
    uint64_t cached_mymetldrElfAddress;
    
    uint8_t cached_os_bank_indicator;
    uint8_t cached_qcfw_lite_flag;

    uint8_t stage3_alreadyDone;

    uint8_t stage6_isAppldr;
    uint8_t stage6_isLv2ldr;

    uint8_t stage6_spu_id; // 0xff = unknown
};

FUNC_DEF struct Stagex_Context_s* GetStagexContext()
{
    return (struct Stagex_Context_s*)0xA00;
}

FUNC_DEF uint8_t IsLv1()
{
    return ((is_lv1 == 0x9669) || (is_lv1 == 0x9666)) ? 1 : 0;
}

FUNC_DEF uint8_t IsStage5()
{
    return (is_lv1 == 0x9666) ? 1 : 0;
}

FUNC_DEF uint8_t IsLogEnabled()
{
#if !LOGGING_ENABLED
    return 0;
#endif

#if !SC_LV1_LOGGING_ENABLED
    if (IsLv1())
        return 0;
#endif

#if !STAGE5_LOG_ENABLED
    if (IsStage5())
        return 0;
#endif

    return 1;
}

// PPU core init
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

// timebase = 79800000

// #define MUL_NS 1000000000
// #define MUL_US 1000000
// #define MUL_MS 1000

#define SYS_TIMEBASE_GET(tb)                          \
    do                                                \
    {                                                 \
        __asm__ volatile("1: mftb %[current_tb];"     \
                         "cmpwi 7, %[current_tb], 0;" \
                         "beq-  7, 1b;"               \
                         : [current_tb] "=r"(tb) :    \
                         : "cr7");                    \
    } while (0)

FUNC_DEF uint64_t GetTimeInNs()
{
    uint64_t my_timebase = 79800000;
    uint64_t MUL_NS = 1000000000;

    uint64_t cur_tb_ns;
    SYS_TIMEBASE_GET(cur_tb_ns);

    cur_tb_ns *= MUL_NS;

    return (cur_tb_ns / my_timebase);
}

FUNC_DEF void WaitInNs(uint64_t ns)
{
    uint64_t start = GetTimeInNs();
    uint64_t end = start + ns;

    while (1)
    {
        uint64_t cur = GetTimeInNs();

        if (cur >= end)
            return;
    }
}

FUNC_DEF uint64_t GetTimeInUs()
{
    uint64_t my_timebase = 79800000;
    uint64_t MUL_US = 1000000;

    uint64_t cur_tb_us;
    SYS_TIMEBASE_GET(cur_tb_us);

    cur_tb_us *= MUL_US;

    return (cur_tb_us / my_timebase);
}

FUNC_DEF void WaitInUs(uint64_t us)
{
    uint64_t start = GetTimeInUs();
    uint64_t end = start + us;

    while (1)
    {
        uint64_t cur = GetTimeInUs();

        if (cur >= end)
            return;
    }
}

FUNC_DEF uint64_t GetTimeInMs()
{
    uint64_t my_timebase = 79800000;
    uint64_t MUL_MS = 1000;

    uint64_t cur_tb_ms;
    SYS_TIMEBASE_GET(cur_tb_ms);

    cur_tb_ms *= MUL_MS;

    return (cur_tb_ms / my_timebase);
}

FUNC_DEF void WaitInMs(uint64_t ms)
{
    uint64_t start = GetTimeInMs();
    uint64_t end = start + ms;

    while (1)
    {
        uint64_t cur = GetTimeInMs();

        if (cur >= end)
            return;
    }
}

FUNC_DEF_NONSTATIC uint64_t GetTimeInMs2()
{
    register uint64_t my_timebase asm("r4") = 79800000;
    register uint64_t MUL_MS asm("r5") = 1000;

    register uint64_t cur_tb_ms asm("r6");
    SYS_TIMEBASE_GET(cur_tb_ms);

    cur_tb_ms *= MUL_MS;

    return (cur_tb_ms / my_timebase);
}

FUNC_DEF_NONSTATIC void WaitInMs2(uint64_t ms)
{
    // save lr
    register uint64_t lr asm("r0");
    asm volatile("mflr %0" : "=r"(lr)::);

    // ms2 = ms
    register uint64_t ms2 asm("r7");
    asm volatile("mr %0, %1" : "=r"(ms2) : "r"(ms) :);

    // start = GetTimeInMs2();
    register uint64_t start asm("r8");
    asm volatile("bl GetTimeInMs2");
    asm volatile("mr %0, 3" : "=r"(start)::);

    // end = start + ms2
    register uint64_t end asm("r9") = start + ms2;

    //

    while (1)
    {
        // cur = GetTimeInMs2();
        register uint64_t cur asm("r10");
        asm volatile("bl GetTimeInMs2");
        asm volatile("mr %0, 3" : "=r"(cur)::);

        if (cur >= end)
            break;
    }

    //

    // restore lr
    asm volatile("mtlr %0" ::"r"(lr) :);
}

FUNC_DEF void memset(void *buf, uint8_t v, uint64_t count)
{
    if ((((uint64_t)buf % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t *buff = (uint64_t *)buf;

        for (uint64_t i = 0; i < (count / 8); ++i)
            buff[i] = v;
    }
    else if ((((uint64_t)buf % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t *buff = (uint32_t *)buf;

        for (uint64_t i = 0; i < (count / 4); ++i)
            buff[i] = v;
    }
    else if ((((uint64_t)buf % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t *buff = (uint16_t *)buf;

        for (uint64_t i = 0; i < (count / 2); ++i)
            buff[i] = v;
    }
    else
    {
        uint8_t *buff = (uint8_t *)buf;

        for (uint64_t i = 0; i < count; ++i)
            buff[i] = v;
    }
}

FUNC_DEF void memcpy(void *dest, const void *src, uint64_t count)
{
    if ((((uint64_t)dest % 8) == 0) && (((uint64_t)src % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t *destt = (uint64_t *)dest;
        const uint64_t *srcc = (const uint64_t *)src;

        for (uint64_t i = 0; i < (count / 8); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint64_t)dest % 4) == 0) && (((uint64_t)src % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t *destt = (uint32_t *)dest;
        const uint32_t *srcc = (const uint32_t *)src;

        for (uint64_t i = 0; i < (count / 4); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint64_t)dest % 2) == 0) && (((uint64_t)src % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t *destt = (uint16_t *)dest;
        const uint16_t *srcc = (const uint16_t *)src;

        for (uint64_t i = 0; i < (count / 2); ++i)
            destt[i] = srcc[i];
    }
    else
    {
        uint8_t *destt = (uint8_t *)dest;
        const uint8_t *srcc = (const uint8_t *)src;

        for (uint64_t i = 0; i < count; ++i)
            destt[i] = srcc[i];
    }
}

FUNC_DEF void memcpy32(void *dest, const void *src, uint64_t count)
{
    uint32_t *destt = (uint32_t *)dest;
    const uint32_t *srcc = (const uint32_t *)src;

    for (uint64_t i = 0; i < (count / 4); ++i)
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

FUNC_DEF uint8_t memcmp32(const void *p1, const void *p2, uint64_t count)
{
    const uint32_t *pp1 = (const uint32_t *)p1;
    const uint32_t *pp2 = (const uint32_t *)p2;

    for (uint64_t i = 0; i < (count / 4); ++i)
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

FUNC_DEF uint8_t SearchAndReplace(void *in_data, uint64_t dataSize, const void *in_searchData, uint64_t searchDataSize, const void *in_replaceData, uint64_t replaceDataSize)
{
    uint8_t *data = (uint8_t *)in_data;

    const uint8_t *searchData = (const uint8_t *)in_searchData;
    const uint8_t *replaceData = (const uint8_t *)in_replaceData;

    for (uint64_t i = 0; i < dataSize; ++i)
    {
        if (!memcmp(&data[i], searchData, searchDataSize))
        {
            memcpy(&data[i], replaceData, replaceDataSize);
            return 1;
        }
    }

    return 0;
}

FUNC_DEF uint8_t SearchMemory(const void *in_data, uint64_t dataSize, const void *in_searchData, uint64_t searchDataSize, uint64_t *outFoundAddr)
{
    const uint8_t *data = (const uint8_t *)in_data;

    const uint8_t *searchData = (const uint8_t *)in_searchData;

    for (uint64_t i = 0; i < dataSize; ++i)
    {
        if (!memcmp(&data[i], searchData, searchDataSize))
        {
            if (outFoundAddr != NULL)
                *outFoundAddr = (uint64_t)&data[i];

            // puts("foundAddr: ");
            // print_hex((uint64_t)&data[i]);
            // puts("\n");

            return 1;
        }
    }

    return 0;
}

FUNC_DEF uint16_t sc_real_packet_header_calc_cksum(const struct sc_real_packet_header_s *pkt_hdr)
{
    uint8_t *ptr;
    uint32_t sum;

    ptr = (uint8_t *)pkt_hdr;
    sum = 0;

    for (int32_t i = 0; i < 6; i++)
        sum += *ptr++;

    sum += 0x8000;

    return (sum & 0xffff);
}

FUNC_DEF void sc_send_packet(const struct sc_packet_s *in, struct sc_packet_s *out)
{
    if (in->payload_size > 256)
    {
        // printf("payload too big!\n");
        dead();
    }

    // 0x24000000000
    uint64_t sb_base_addr = 0x24;
    sb_base_addr <<= 36;

    //
    uint64_t send_packet_counter_addr[2];

    // cell
    // 0x2400008DFF0
    send_packet_counter_addr[0] = sb_base_addr;
    send_packet_counter_addr[0] |= 0x8DFF0;

    // sc
    // 0x2400008CFF4
    send_packet_counter_addr[1] = sb_base_addr;
    send_packet_counter_addr[1] |= 0x8CFF4;

    // 0x2400008D000
    uint64_t send_packet_data_addr = sb_base_addr;
    send_packet_data_addr |= 0x8D000;

    // 0x2400008E100
    uint64_t send_packet_kick_addr = sb_base_addr;
    send_packet_kick_addr |= 0x8E100;

    //
    uint64_t recieve_packet_counter_addr[2];

    // sc
    // 0x2400008CFF0
    recieve_packet_counter_addr[0] = sb_base_addr;
    recieve_packet_counter_addr[0] |= 0x8CFF0;

    // cell
    // 0x2400008DFF4
    recieve_packet_counter_addr[1] = sb_base_addr;
    recieve_packet_counter_addr[1] |= 0x8DFF4;

    // 0x2400008E000
    uint64_t recieve_packet_test_addr = sb_base_addr;
    recieve_packet_test_addr |= 0x8E000;

    // 0x2400008C000
    uint64_t recieve_packet_data_addr = sb_base_addr;
    recieve_packet_data_addr |= 0x8C000;

    //

    volatile uint32_t *send_packet_counter_cell = (volatile uint32_t *)send_packet_counter_addr[0];

    volatile uint32_t *send_packet_data = (volatile uint32_t *)send_packet_data_addr;
    volatile uint32_t *send_packet_kick = (volatile uint32_t *)send_packet_kick_addr;

    //

    volatile uint32_t *recieve_packet_counter_sc = (volatile uint32_t *)recieve_packet_counter_addr[0];
    volatile uint32_t *recieve_packet_counter_cell = (volatile uint32_t *)recieve_packet_counter_addr[1];

    volatile uint32_t *recieve_packet_test = (volatile uint32_t *)recieve_packet_test_addr;

    volatile uint32_t *recieve_packet_data = (volatile uint32_t *)recieve_packet_data_addr;

    //

    {
        uint16_t tag = (uint16_t)GetTimeInMs();

#if 1

        {
            uint8_t buf[512] __attribute__((aligned(8)));
            memset(buf, 0, 512);

            struct sc_real_packet_header_s in_real_pkt_hdr;

            in_real_pkt_hdr.service_id = in->service_id;
            in_real_pkt_hdr.version = 1;

            in_real_pkt_hdr.tag = tag;

            in_real_pkt_hdr.res[0] = 0;
            in_real_pkt_hdr.res[1] = 0;

            in_real_pkt_hdr.cksum = sc_real_packet_header_calc_cksum(&in_real_pkt_hdr);

            in_real_pkt_hdr.communication_tag = in->communication_tag;

            in_real_pkt_hdr.payload_size[0] = in->payload_size;
            in_real_pkt_hdr.payload_size[1] = in->payload_size;

            uint64_t curSize = 0;

            memcpy(&buf[curSize], &in_real_pkt_hdr, sizeof(struct sc_real_packet_header_s));
            curSize += sizeof(struct sc_real_packet_header_s);

            uint64_t payload_size = in_real_pkt_hdr.payload_size[0];

            memcpy(&buf[curSize], &in->data[0], payload_size);
            curSize += payload_size;

            // padding...

            uint64_t align = 4;
            uint64_t zz = (payload_size % align);

            if (zz != 0)
            {
                uint64_t y = (align - zz);

                memset(&buf[curSize], 0, y);
                curSize += y;
            }

            // cksum...

            uint32_t cksum = 0;

            for (int32_t i = 0; i < curSize; i++)
                cksum -= buf[i];

            cksum = cksum & 0xffff;

            memcpy(&buf[curSize], &cksum, sizeof(uint32_t));
            curSize += sizeof(uint32_t);

            {
                uint32_t *p = (uint32_t *)buf;

                for (uint32_t i = 0; i < (512 / 4); ++i)
                    send_packet_data[i] = p[i];
            }
        }

#else

        {
            tag = 0x1620;

            send_packet_data[0] = 0x16011620;
            send_packet_data[1] = 0x0000804d;
            send_packet_data[2] = 0x00000001;
            send_packet_data[3] = 0x00080008;
            send_packet_data[4] = 0x20290a00;
            send_packet_data[5] = 0x000001b6;
            send_packet_data[6] = 0x0000fdcb;
        }

#endif

        {
            uint32_t value = *send_packet_counter_cell;

            value = value + 1;
            value &= 0xffff;
            value = (value << 16) | value;

            *send_packet_counter_cell = value;
        }

        uint32_t old_recieve_packet_counter_sc = *recieve_packet_counter_sc;

        eieio();
        *send_packet_kick = 0x1;
        eieio();

        *recieve_packet_test |= 0xFFFFFFFD;
        eieio();

        while (1)
        {
            if (*recieve_packet_counter_sc != old_recieve_packet_counter_sc)
                break;
        }

        {
            uint32_t value = *recieve_packet_counter_cell;

            value = value + 1;
            value &= 0xffff;
            value = (value << 16) | value;

            *recieve_packet_counter_cell = value;
        }

        struct sc_real_packet_header_s out_real_pkt_hdr;

        {
            uint32_t *p = (uint32_t *)&out_real_pkt_hdr;

            for (uint32_t i = 0; i < 4; i++)
                p[i] = recieve_packet_data[i];
        }

        if (out_real_pkt_hdr.tag != tag)
            dead();

        if (out != NULL)
        {
            out->service_id = out_real_pkt_hdr.service_id;
            out->communication_tag = out_real_pkt_hdr.communication_tag;

            out->payload_size = out_real_pkt_hdr.payload_size[0];

            {
                uint32_t *p = (uint32_t *)out->data;

                for (uint32_t i = 0; i < (256 / 4); ++i)
                    p[i] = recieve_packet_data[i + 4];
            }
        }
    }
}

FUNC_DEF void sc_triple_beep()
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x16;
    pkt.communication_tag = 1;

    pkt.payload_size = 8;

    pkt.data[0] = 0x20;
    pkt.data[1] = 0x29;
    pkt.data[2] = 0x0a;
    pkt.data[3] = 0x00;

    pkt.data[4] = 0x00;
    pkt.data[5] = 0x00;

    pkt.data[6] = 0x01;
    pkt.data[7] = 0xb6;

    sc_send_packet(&pkt, NULL);
}

FUNC_DEF void sc_continuous_beep()
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x16;
    pkt.communication_tag = 1;

    pkt.payload_size = 8;

    pkt.data[0] = 0x20;
    pkt.data[1] = 0x29;
    pkt.data[2] = 0x0a;
    pkt.data[3] = 0x00;

    pkt.data[4] = 0x00;
    pkt.data[5] = 0x00;

    pkt.data[6] = 0x0f;
    pkt.data[7] = 0xff;

    sc_send_packet(&pkt, NULL);
}

FUNC_DEF void sc_soft_restart()
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x13;
    pkt.communication_tag = 1;

    pkt.payload_size = 4;

    pkt.data[0] = 0x11;
    pkt.data[1] = 0x00;
    pkt.data[2] = 0x00;
    pkt.data[3] = 0x01;

    sc_send_packet(&pkt, NULL);
}

FUNC_DEF void sc_hard_restart()
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x13;
    pkt.communication_tag = 1;

    pkt.payload_size = 4;

    pkt.data[0] = 0x11;
    pkt.data[1] = 0x00;
    pkt.data[2] = 0x00;
    pkt.data[3] = 0x02;

    sc_send_packet(&pkt, NULL);
}

FUNC_DEF void real_sc_puts_init()
{
#if SC_PUTS_BUFFER_ENABLED
    uint64_t *sc_puts_buflen = (uint64_t *)0xD000000;
    char *sc_puts_buf = (char *)0xD000010;

    *sc_puts_buflen = 0;
    sc_puts_buf[*sc_puts_buflen] = 0;
#endif
}

FUNC_DEF void sc_puts_init()
{
    if (!IsLogEnabled())
        return;

    real_sc_puts_init();
}

FUNC_DEF void sc_puts(const char *str)
{
    uint64_t len = strlen(str);

    if (len == 0)
        return;

    if (len > 199)
        dead();

#if SC_PUTS_BUFFER_ENABLED

    uint64_t *sc_puts_buflen = (uint64_t *)0xD000000;
    char *sc_puts_buf = (char *)0xD000010;

    // just drop the whole string
    if ((*sc_puts_buflen + len) > 199)
    {
        *sc_puts_buflen = 0;
        sc_puts_buf[*sc_puts_buflen] = 0;

        return;
    }

    memcpy(&sc_puts_buf[*sc_puts_buflen], str, len);

    *sc_puts_buflen += len;
    sc_puts_buf[*sc_puts_buflen] = 0;

    if (str[(len - 1)] == '\n')
    {
        struct sc_packet_s pkt;

        pkt.service_id = 0x20;
        pkt.communication_tag = 1;

        pkt.payload_size = 1;
        pkt.data[0] = 0x00;

        memcpy(&pkt.data[pkt.payload_size], sc_puts_buf, (*sc_puts_buflen + 1));
        pkt.payload_size += (*sc_puts_buflen + 1);

        sc_send_packet(&pkt, NULL);

        *sc_puts_buflen = 0;
        sc_puts_buf[*sc_puts_buflen] = 0;
    }

#else

    struct sc_packet_s pkt;

    pkt.service_id = 0x20;
    pkt.communication_tag = 1;

    pkt.payload_size = 1;
    pkt.data[0] = 0x00;

    memcpy(&pkt.data[pkt.payload_size], str, (len + 1));
    pkt.payload_size += len + 1;

    sc_send_packet(&pkt, NULL);

#endif
}

FUNC_DEF uint8_t sc_read_eeprom8(uint8_t block_id, uint8_t offset)
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x14;
    pkt.communication_tag = 1;

    pkt.payload_size = 4;
    pkt.data[0] = 0x20;

    pkt.data[1] = block_id;  // block id
    pkt.data[2] = offset; // offset
    pkt.data[3] = 0x1;  // size

    struct sc_packet_s outpkt;
    sc_send_packet(&pkt, &outpkt);

    if (outpkt.payload_size != 5)
        dead();

    return outpkt.data[4];
}

FUNC_DEF void sc_write_eeprom8(uint8_t block_id, uint8_t offset, uint8_t value)
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x14;
    pkt.communication_tag = 1;

    pkt.payload_size = 5;
    pkt.data[0] = 0x10;

    pkt.data[1] = block_id;  // block id
    pkt.data[2] = offset; // offset
    pkt.data[3] = 0x1;  // size

    pkt.data[4] = value; // value

    struct sc_packet_s outpkt;
    sc_send_packet(&pkt, &outpkt);
}

FUNC_DEF uint8_t sc_read_os_bank_indicator()
{
    // block id (0x48C00)
    // offset (0x48C24)
    return sc_read_eeprom8(0x2, 0x24);
}

FUNC_DEF void sc_write_os_bank_indicator(uint8_t val)
{
    // block id (0x48C00)
    // offset (0x48C24)
    sc_write_eeprom8(0x2, 0x24, val);
}

FUNC_DEF uint8_t sc_read_recovery_mode_flag()
{
    // block id (0x48C00)
    // offset (0x48C61)
    return sc_read_eeprom8(0x2, 0x61);
}

FUNC_DEF void sc_write_recovery_mode_flag(uint8_t val)
{
    // block id (0x48C00)
    // offset (0x48C61)
    sc_write_eeprom8(0x2, 0x61, val);
}

FUNC_DEF uint8_t sc_read_qcfw_lite_flag()
{
    // block id (0x3000)
    // offset (0x3000)
    return sc_read_eeprom8(0x20, 0x0);
}

FUNC_DEF uint8_t get_qcfw_lite_flag()
{
    if (!IsLv1())
        return sc_read_qcfw_lite_flag();

    return GetStagexContext()->cached_qcfw_lite_flag;
}

FUNC_DEF uint8_t sc_read_shadow_os_bank_indicator()
{
    // block id (0x3000)
    // offset (0x3001)
    return sc_read_eeprom8(0x20, 0x1);
}

FUNC_DEF void sc_write_shadow_os_bank_indicator(uint8_t val)
{
    // block id (0x3000)
    // offset (0x3001)
    sc_write_eeprom8(0x20, 0x1, val);
}

FUNC_DEF uint8_t sc_read_request_os_bank_indicator()
{
    // block id (0x3000)
    // offset (0x3002)
    return sc_read_eeprom8(0x20, 0x2);
}

FUNC_DEF void sc_write_request_os_bank_indicator(uint8_t val)
{
    // block id (0x3000)
    // offset (0x3002)
    sc_write_eeprom8(0x20, 0x2, val);
}

FUNC_DEF void sc_query_system_power_up_cause(uint64_t* outCause) // [2]
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x13;
    pkt.communication_tag = 1;

    pkt.payload_size = 1;
    pkt.data[0] = 0x10;

    struct sc_packet_s outpkt;
    sc_send_packet(&pkt, &outpkt);

    if (outpkt.payload_size != 16)
        dead();

    if (outCause != NULL)
        memcpy(outCause, outpkt.data, 16);
}

FUNC_DEF void real_puts(const char *str)
{
    if (!IsLogEnabled())
        return;

    sc_puts(str);
}

FUNC_DEF void real_print_decimal(uint64_t v)
{
    if (!IsLogEnabled())
        return;

    char charArr[16];

    charArr[0] = '0';
    charArr[1] = '1';
    charArr[2] = '2';
    charArr[3] = '3';

    charArr[4] = '4';
    charArr[5] = '5';
    charArr[6] = '6';
    charArr[7] = '7';

    charArr[8] = '8';
    charArr[9] = '9';
    charArr[10] = 'a';
    charArr[11] = 'b';

    charArr[12] = 'c';
    charArr[13] = 'd';
    charArr[14] = 'e';
    charArr[15] = 'f';

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
    real_puts(outBuf);
}

FUNC_DEF void real_print_hex(uint64_t v)
{
    if (!IsLogEnabled())
        return;

    char charArr[16];

    charArr[0] = '0';
    charArr[1] = '1';
    charArr[2] = '2';
    charArr[3] = '3';

    charArr[4] = '4';
    charArr[5] = '5';
    charArr[6] = '6';
    charArr[7] = '7';

    charArr[8] = '8';
    charArr[9] = '9';
    charArr[10] = 'a';
    charArr[11] = 'b';

    charArr[12] = 'c';
    charArr[13] = 'd';
    charArr[14] = 'e';
    charArr[15] = 'f';

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

    outBuf[curOutBufLen] = '0';
    ++curOutBufLen;

    outBuf[curOutBufLen] = 'x';
    ++curOutBufLen;

    for (uint64_t i = 0; i < curBufLen; i++)
        outBuf[(curOutBufLen + i)] = buf[((curBufLen - 1) - i)];

    curOutBufLen += curBufLen;

    if (curOutBufLen == 2)
    {
        outBuf[curOutBufLen] = '0';
        ++curOutBufLen;
    }

    // outBuf[curOutBufLen] = '\n';
    //++curOutBufLen;

    outBuf[curOutBufLen] = 0;
    real_puts(outBuf);
}

#if LOGGING_ENABLED
#define puts real_puts
#define print_decimal real_print_decimal
#define print_hex real_print_hex
#else
#define puts(...)
#define print_decimal(...)
#define print_hex(...)
#endif

#if SC_LV1_LOGGING_ENABLED
#define lv1_puts puts
#define lv1_print_decimal print_decimal
#define lv1_print_hex print_hex
#else
#define lv1_puts(...)
#define lv1_print_decimal(...)
#define lv1_print_hex(...)
#endif

FUNC_DEF void dead_beep()
{
    WaitInMs(2000);

    sc_continuous_beep();
    dead();
}

#define SCMD_SBW 0x1 /* Serial Broadcast Write */
#define SCMD_SDW 0x0 /* Serial Device Write */

#define XDR_CFG 0x02  /* Configuration */
#define XDR_PM 0x03   /* Power management */
#define XDR_WDSL 0x04 /* Write data serial load control */
#define XDR_DLY 0x1f  /* Delay control */

#define XDR_SCMD(CMD, SSID, REG) (((CMD) << 28) | 0x04000000 | ((SSID) << 16) | ((REG) << 8))

#define BE_MMIO_BASE 0x20000000000UL
#define MMIO_BE_MIC (0x50A000 | BE_MMIO_BASE)

// ch0
#define YREG_YDRAM_DTA_0 0x108
#define MIC_YREG_STAT_0 0x110

// ch1
#define YREG_YDRAM_DTA_1 0x148
#define MIC_YREG_STAT_1 0x150

FUNC_DEF void XdrRegWrite(uint32_t data)
{
    volatile uint32_t *reg = (volatile uint32_t *)(MMIO_BE_MIC | YREG_YDRAM_DTA_0);
    *reg = data;
    eieio();

    volatile uint32_t *xxx = (volatile uint32_t *)(MMIO_BE_MIC | MIC_YREG_STAT_0);
    volatile uint32_t yyy = *xxx;
    ++yyy;
    eieio();
}

FUNC_DEF uint64_t calc_myspu_id()
{
    uint8_t found = 0;
    uint64_t myspu_id = 0;

    uint32_t spu_avail = *((const volatile uint32_t*)0x20000509C38);

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint32_t mask = (1U << i);

        if ((spu_avail & mask) != 0)
        {
            found = 1;
            myspu_id = (7 - i);
            break;
        }
    }

    if (!found)
    {
        puts("calc_myspu_id failed!!\n");
        dead_beep();
    }

    return myspu_id;
}

struct coreos_header_s
{
    uint64_t unknown0;
    uint64_t length_region;
    uint32_t unknown1;
    uint32_t entry_count;
    uint64_t length_region2;
};

struct coreos_entry_s
{
    uint64_t offset;
    uint64_t length;
    char file_name[32];
};

// startAddress = CoreOS start
FUNC_DEF uint8_t CoreOS_FindFileEntry(uint64_t startAddress, const char *fileName, uint64_t *outFileAddress, uint64_t *outFileSize)
{
    uint64_t curAddress = startAddress;

    const struct coreos_header_s *header = (const struct coreos_header_s *)curAddress;
    curAddress += sizeof(struct coreos_header_s);

    uint32_t entry_count = header->entry_count;

    for (uint32_t i = 0; i < entry_count; ++i)
    {
        const struct coreos_entry_s *entry = (const struct coreos_entry_s *)curAddress;
        curAddress += sizeof(struct coreos_entry_s);

        // puts(entry->file_name);
        // puts("\n");

        if (!strcmp(entry->file_name, fileName))
        {
            if (outFileAddress != NULL)
                *outFileAddress = (startAddress + (entry->offset + 16));

            if (outFileSize != NULL)
                *outFileSize = entry->length;

            return 1;
        }
    }

    return 0;
}

FUNC_DEF uint8_t CoreOS_FindFileEntry_Bank(uint8_t os_bank_indicator, const char *fileName, uint64_t *outFileAddress, uint64_t *outFileSize)
{
    uint64_t coreOSStartAddress = (os_bank_indicator == 0xff) ? 0x2401F0C0000 : 0x2401F7C0000;

    return CoreOS_FindFileEntry(coreOSStartAddress, fileName, outFileAddress, outFileSize);
}

FUNC_DEF uint16_t CoreOS_Bank_GetFWVersion(uint8_t os_bank_indicator)
{
    uint64_t addr;
    
    if (!CoreOS_FindFileEntry_Bank(os_bank_indicator, "sdk_version", &addr, NULL))
        return 0;

    const char* str = (const char*)addr;

    uint16_t ver = 0;

    // 492.000
    ver += ((str[0] - '0') * 100);
    ver += ((str[1] - '0') * 10);
    ver += (str[2] - '0');

    return ver;
}

FUNC_DEF uint8_t CoreOS_Bank_IsqCFW(uint8_t os_bank_indicator)
{
    return CoreOS_FindFileEntry_Bank(os_bank_indicator, "qcfw", NULL, NULL);
}

FUNC_DEF uint8_t calc_os_bank_indicator()
{
    return sc_read_os_bank_indicator();
}

FUNC_DEF uint8_t get_os_bank_indicator()
{
    if (!IsLv1())
        return calc_os_bank_indicator();

    return GetStagexContext()->cached_os_bank_indicator;
}

FUNC_DEF uint8_t CoreOS_FindFileEntry_CurrentBank(const char *fileName, uint64_t *outFileAddress, uint64_t *outFileSize)
{
    uint8_t os_bank_indicator = get_os_bank_indicator();

    return CoreOS_FindFileEntry_Bank(os_bank_indicator, fileName, outFileAddress, outFileSize);
}

FUNC_DEF uint8_t CoreOS_FindFileEntry_Aux(const char *fileName, uint64_t *outFileAddress, uint64_t *outFileSize)
{
    return CoreOS_FindFileEntry(0x2401FF21000, fileName, outFileAddress, outFileSize);
}

FUNC_DEF uint16_t CoreOS_CurrentBank_GetFWVersion()
{
    return CoreOS_Bank_GetFWVersion(get_os_bank_indicator());
}

FUNC_DEF uint8_t CoreOS_CurrentBank_IsqCFW()
{
    return CoreOS_Bank_IsqCFW(get_os_bank_indicator());
}

struct ElfHeader_s
{
    uint8_t e_ident[16];  /* ELF identification */
    uint16_t e_type;      /* object file type */
    uint16_t e_machine;   /* machine type */
    uint32_t e_version;   /* object file version */
    uint64_t e_entry;     /* entry point address */
    uint64_t e_phoff;     /* program header offset */
    uint64_t e_shoff;     /* section header offset */
    uint32_t e_flags;     /* processor-specific flags */
    uint16_t e_ehsize;    /* ELF header size */
    uint16_t e_phentsize; /* size of program header entry */
    uint16_t e_phnum;     /* number of program header entries */
    uint16_t e_shentsize; /* size of section header entry */
    uint16_t e_shnum;     /* number of section header entries */
    uint16_t e_shstrndx;  /* section name string table index */
} __attribute__((packed));

struct ElfPhdr_s
{
    uint32_t p_type;   /* Segment type */
    uint32_t p_flags;  /* Segment flags */
    uint64_t p_offset; /* Segment file offset */
    uint64_t p_vaddr;  /* Segment virtual address */
    uint64_t p_paddr;  /* Segment physical address */
    uint64_t p_filesz; /* Segment size in file */
    uint64_t p_memsz;  /* Segment size in memory */
    uint64_t p_align;  /* Segment alignment */
};

struct ElfHeader32_s
{
    uint8_t e_ident[16];  /* ELF identification */
    uint16_t e_type;      /* object file type */
    uint16_t e_machine;   /* machine type */
    uint32_t e_version;   /* object file version */
    uint32_t e_entry;     /* entry point address */
    uint32_t e_phoff;     /* program header offset */
    uint32_t e_shoff;     /* section header offset */
    uint32_t e_flags;     /* processor-specific flags */
    uint16_t e_ehsize;    /* ELF header size */
    uint16_t e_phentsize; /* size of program header entry */
    uint16_t e_phnum;     /* number of program header entries */
    uint16_t e_shentsize; /* size of section header entry */
    uint16_t e_shnum;     /* number of section header entries */
    uint16_t e_shstrndx;  /* section name string table index */
} __attribute__((packed));

struct ElfPhdr32_s
{
    uint32_t p_type;   /* Segment type */
    uint32_t p_offset; /* Segment file offset */
    uint32_t p_vaddr;  /* Segment virtual address */
    uint32_t p_paddr;  /* Segment physical address */
    uint32_t p_filesz; /* Segment size in file */
    uint32_t p_memsz;  /* Segment size in memory */
    uint32_t p_flags;  /* Segment flags */
    uint32_t p_align;  /* Segment alignment */
};

FUNC_DEF void LoadElf(uint64_t elfFileAddress, uint64_t destAddressOffset, uint8_t doZero)
{
    const struct ElfHeader_s *elfHdr = (const struct ElfHeader_s *)elfFileAddress;

    if (*((const uint32_t *)elfHdr->e_ident) != 0x7F454C46)
    {
        puts("LoadElf e_ident check failed!\n");
        dead();
    }

    puts("destAddressOffset = ");
    print_hex(destAddressOffset);
    puts("\n");

    puts("e_phoff = ");
    print_hex(elfHdr->e_phoff);
    puts("\n");

    puts("e_phnum = ");
    print_hex(elfHdr->e_phnum);
    puts("\n");

    const struct ElfPhdr_s *phdr = (const struct ElfPhdr_s *)(elfFileAddress + elfHdr->e_phoff);

    for (uint16_t i = 0; i < elfHdr->e_phnum; ++i)
    {
        puts("p_offset = ");
        print_hex(phdr->p_offset);

        puts(", p_vaddr = ");
        print_hex(phdr->p_vaddr);

        puts(", p_paddr = ");
        print_hex(phdr->p_paddr);

        puts(", p_filesz = ");
        print_hex(phdr->p_filesz);

        puts(", p_memsz = ");
        print_hex(phdr->p_memsz);

        uint64_t loadAddress = phdr->p_vaddr;
        loadAddress += destAddressOffset;

        if (loadAddress >= 0x8000000000000000)
            loadAddress -= 0x8000000000000000;

        puts(", loadAddress = ");
        print_hex(loadAddress);

        puts("\n");

        if (doZero)
        {
            uint64_t clearSize = (phdr->p_memsz - phdr->p_filesz);
            memset((void *)(loadAddress + phdr->p_filesz), 0, clearSize);
        }
            
        memcpy((void *)(loadAddress), (const void *)(elfFileAddress + phdr->p_offset), phdr->p_filesz);

        ++phdr;
    }

    eieio();
}

#include "Spu.c"
#include "SpuAux.c"

struct SceHeader_s
{
    uint32_t magic;
    uint32_t version;
    uint16_t attribute;
    uint16_t category;
    uint32_t ext_header_size;
    uint64_t file_offset;
    uint64_t file_size;
};

struct SceMetaInfo_s
{
    uint64_t key[2];
    uint8_t key_pad[16];
    uint64_t iv[2];
    uint8_t iv_pad[16];
};

struct SceMetaHeader_s
{
    uint64_t sign_offset;
    uint32_t sign_algorithm; // 1 = ECDSA160, 2 = HMACSHA1, 3 = SHA1, 5 = RSA2048, 6 = HMACSHA256 (?not used?)
    uint32_t section_entry_num;
    uint32_t key_entry_num;
    uint32_t optional_header_size;
    uint64_t pad;
};

struct SceMetaSectionHeader_s
{
    uint64_t segment_offset;
    uint64_t segment_size;
    uint32_t segment_type;   // 1 = shdr, 2 = phdr, 3 = sceversion
    uint32_t segment_id;     // 0,1,2,3,etc for phdr, always 3 for shdrs, sceversion shdr number for sceversion
    uint32_t sign_algorithm; // 1 = ECDSA160 (not used), 2 = HMACSHA1, 3 = SHA1, 5 = RSA2048 (not used), 6 = HMACSHA256
    uint32_t sign_idx;
    uint32_t enc_algorithm;  // 1 = none, 2 = aes128cbccfb, 3 = aes128ctr
    uint32_t key_idx;        // -1 when enc_algorithm = none
    uint32_t iv_idx;         // -1 when enc_algorithm = none
    uint32_t comp_algorithm; // 1 = plain, 2 = zlib
};

struct SceMetaKey_s
{
    uint64_t key[2];
};

#if AES_ENABLED
// can't compile as seperate files because of global registers
#include "Aes/Aes.c"
#endif

#if !STAGE0_DECRYPTLV0SELF_SPU_ENABLED

FUNC_DEF void DecryptLv0Self(void *inDest, const void *inSrc, uint8_t use_spu)
{
    puts("DecryptLv0Self()\n");

    uint8_t *dest = (uint8_t *)inDest;
    const uint8_t *src = (const uint8_t *)inSrc;

    uint64_t curSrcOffset = 0;

    struct SceHeader_s sceHeader;
    memcpy(&sceHeader, &src[curSrcOffset], sizeof(struct SceHeader_s));

    if ((sceHeader.magic) != 0x53434500)
    {
        puts("SCE magic check failed!, ");
        print_hex(sceHeader.magic);
        puts("\n");

        dead_beep();
        return;
    }

    //puts("sceHeader:\n");

    //puts("ext_header_size = ");
    //print_hex(sceHeader.ext_header_size);
    //puts("\n");

    //puts("file_offset = ");
    //print_hex(sceHeader.file_offset);
    //puts("\n");

    //puts("file_size = ");
    //print_hex(sceHeader.file_size);
    //puts("\n");

    // erk=CA7A24EC38BDB45B 98CCD7D363EA2AF0 C326E65081E0630C B9AB2D215865878A
    // riv=F9205F46F6021697 E670F13DFA726212

    //puts("1\n");

    uint64_t meta_key[4];
    meta_key[0] = (0xCA7A24EC38BDB45B);
    meta_key[1] = (0x98CCD7D363EA2AF0);
    meta_key[2] = (0xC326E65081E0630C);
    meta_key[3] = (0xB9AB2D215865878A);

    uint64_t meta_iv[2];
    meta_iv[0] = (0xF9205F46F6021697);
    meta_iv[1] = (0xE670F13DFA726212);

    //puts("2\n");

    WORD meta_aes_key[60];
    aes_key_setup((const uint8_t *)meta_key, meta_aes_key, 256);

    //puts("3\n");

    curSrcOffset = 0x200;

    struct SceMetaInfo_s metaInfo;
    aes_decrypt_cbc(

        &src[curSrcOffset],
        sizeof(struct SceMetaInfo_s),

        (uint8_t *)&metaInfo,

        meta_aes_key,
        256,

        (const uint8_t *)meta_iv);

    //puts("4\n");

    curSrcOffset += sizeof(struct SceMetaInfo_s);

    //puts("metaInfo:\n");

    //puts("metaInfo.key[0] = ");
    //print_hex(metaInfo.key[0]);
    //puts("\n");

    //puts("metaInfo.key[1] = ");
    //print_hex(metaInfo.key[1]);
    //puts("\n");

    //puts("metaInfo.iv[0] = ");
    //print_hex(metaInfo.iv[0]);
    //puts("\n");

    //puts("metaInfo.iv[1] = ");
    //print_hex(metaInfo.iv[1]);
    //puts("\n");

    WORD meta_header_key[60];
    aes_key_setup((const uint8_t *)metaInfo.key, meta_header_key, 128);

    uint64_t metasSize = (sceHeader.file_offset) - sizeof(struct SceHeader_s) + (sceHeader.ext_header_size) + sizeof(struct SceMetaInfo_s);

    //puts("metasSize = ");
    //print_decimal(metasSize);
    //puts("\n");

    uint8_t metasBuf[16384];

    aes_decrypt_ctr(

        &src[curSrcOffset],
        metasSize,

        metasBuf,

        meta_header_key,
        128,

        (const uint8_t *)metaInfo.iv

    );

    const struct SceMetaHeader_s *metaHeader = (const struct SceMetaHeader_s *)&metasBuf[0];

    //puts("metaHeader:\n");

    //puts("section_entry_num = ");
    //print_decimal(metaHeader->section_entry_num);
    //puts("\n");

    //puts("key_entry_num = ");
    //print_decimal(metaHeader->key_entry_num);
    //puts("\n");

    const struct SceMetaSectionHeader_s *metaSectionHeaders = (const struct SceMetaSectionHeader_s *)&metasBuf[sizeof(struct SceMetaHeader_s)];

#if 0

    for (uint32_t i = 0; i < (metaHeader->section_entry_num); ++i)
    {
        const struct SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        puts("section_headers[");
        print_decimal(i);
        puts("]:\n");

        puts("segment_id = ");
        print_decimal(h->segment_id);

        puts(", segment_offset = ");
        print_hex(h->segment_offset);

        puts(", segment_size = ");
        print_hex(h->segment_size);

        puts(", enc_algorithm = ");
        print_decimal(h->enc_algorithm);

        puts(", key_idx = ");
        print_decimal(h->key_idx);

        puts(", iv_idx = ");
        print_decimal(h->iv_idx);

        puts("\n");
    }

#endif

    const struct SceMetaKey_s *metaKeys = (const struct SceMetaKey_s *)&metasBuf[sizeof(struct SceMetaHeader_s) + ((metaHeader->section_entry_num) * sizeof(struct SceMetaSectionHeader_s))];

#if 0

    for (uint32_t i = 0; i < (metaHeader->key_entry_num); ++i)
    {
        const struct SceMetaKey_s *k = &metaKeys[i];

        puts("keys[");
        print_decimal(i);
        puts("]: ");

        puts("key[0] = ");
        print_hex(k->key[0]);

        puts(", key[1] = ");
        print_hex(k->key[1]);

        puts("\n");
    }

#endif

    const struct ElfHeader_s *elfHeader = (const struct ElfHeader_s *)&src[0x90];

    memcpy(dest, elfHeader, sizeof(struct ElfHeader_s));
    memcpy(dest + (elfHeader->e_phoff), &src[0x90 + (elfHeader->e_phoff)], (elfHeader->e_phentsize) * (elfHeader->e_phnum));

    const struct ElfPhdr_s *elfPhdrs = (const struct ElfPhdr_s *)(dest + (elfHeader->e_phoff));

    uint64_t spu_id = calc_myspu_id();
    uint64_t spu_old_mfc_sr1;

    if (use_spu)
        spu_old_mfc_sr1 = SpuAux_Init(spu_id);

    for (uint16_t i = 0; i < (elfHeader->e_phnum); ++i)
    {
        const struct ElfPhdr_s *phdr = &elfPhdrs[i];

        puts("decrypting phdr ");
        print_decimal(i);
        puts("...\n");

        const struct SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        const struct SceMetaKey_s *key = &metaKeys[(h->key_idx)];
        const struct SceMetaKey_s *iv = &metaKeys[(h->iv_idx)];

        puts("segment_offset = ");
        print_hex(h->segment_offset);
        puts(", segment_size = ");
        print_hex(h->segment_size);

        puts(", p_offset = ");
        print_hex(h->segment_offset);

        uint64_t in_addr = (uint64_t)&src[(h->segment_offset)];
        uint64_t out_addr = (uint64_t)dest + phdr->p_offset;

        puts(", in_addr = ");
        print_hex(in_addr);

        puts(", out_addr = ");
        print_hex(out_addr);

        puts("\n");

        if (use_spu)
        {
            spu_aes128_decrypt_ctr(
                spu_id,

                (const uint8_t *)in_addr,
                (h->segment_size),

                (uint8_t *)out_addr,

                (const uint8_t *)key->key,
                (const uint8_t *)iv->key
            );
        }
        else
        {
            WORD aes_key[60];
            aes_key_setup((const uint8_t *)key->key, aes_key, 128);

            aes_decrypt_ctr(

                (const uint8_t *)in_addr,
                (h->segment_size),

                (uint8_t *)out_addr,

                aes_key,
                128,

                (const uint8_t *)iv->key

            );
        }
    }

    if (use_spu)
        SpuAux_Uninit(spu_id, spu_old_mfc_sr1);

    puts("DecryptLv0Self() done.\n");
}

#endif

#if !ZLIB_SPU_ONLY_ENABLED

#include "tinf/tinf.h"

#include "tinf/adler32.c"
#include "tinf/crc32.c"
#include "tinf/tinflate.c"
#include "tinf/tinfzlib.c"

#endif

struct ZelfHeader_s
{
    uint64_t magic;

    uint64_t original_size;
    uint64_t compressed_size;
};

struct Zelf2Header_s
{
    uint64_t magic;

    uint64_t original_size;
    uint64_t compressed_size;

    uint64_t padding; // for spu
};

FUNC_DEF void ZelfDecompress(uint64_t zelfFileAddress, void *destAddress, uint64_t *destSize, uint8_t use_spu)
{
    puts("ZelfDecompress()\n");

    puts("zelfFileAddress = ");
    print_hex(zelfFileAddress);

    puts(", destAddress = ");
    print_hex((uint64_t)destAddress);

    puts(", destSize = ");
    print_decimal(*destSize);

    puts("\n");

    const struct ZelfHeader_s *zelfHeader = (const struct ZelfHeader_s *)zelfFileAddress;
    //const struct Zelf2Header_s *zelf2Header = (const struct Zelf2Header_s *)zelfFileAddress;

    uint8_t isZelf1 = (zelfHeader->magic == 0x5A454C465A454C46); // ZELFZELF
    uint8_t isZelf2 = (zelfHeader->magic == 0x5A454C465A454C32); // ZELFZEL2

    if (!isZelf1 && !isZelf2)
    {
        puts("ZELF magic check fail!\n");

        dead_beep();
    }

    if (isZelf2)
        puts("ZELF2\n");
    else if (isZelf1)
        puts("ZELF1\n");

    uint64_t original_size = zelfHeader->original_size;
    puts("original_size = ");
    print_decimal(original_size);

    uint64_t compressed_size = zelfHeader->compressed_size;
    puts(", compressed_size = ");
    print_decimal(compressed_size);

    puts("\n");

    if (original_size > *destSize)
    {
        puts("original_size too big!\n");

        dead_beep();
    }

    uint64_t headerSize = isZelf2 ? sizeof(struct Zelf2Header_s) : sizeof(struct ZelfHeader_s);

    const void* compressed_data = (const void *)(zelfFileAddress + headerSize);

    uint32_t xxx = original_size;

    int32_t decompress_result;
    
    if (use_spu && isZelf2)
    {
        uint64_t yyy;

        //

        uint64_t spu_id = calc_myspu_id();
        uint64_t spu_old_mfc_sr1 = SpuAux_Init(spu_id);

        spu_zlib_decompress(spu_id, compressed_data, compressed_size, destAddress, &yyy);

        SpuAux_Uninit(spu_id, spu_old_mfc_sr1);

        //

        decompress_result = 0;
        xxx = (uint32_t)yyy;
    }
    else
    {

#if !ZLIB_SPU_ONLY_ENABLED

        decompress_result = tinf_zlib_uncompress(
            destAddress, &xxx, compressed_data, (uint32_t)compressed_size);

#else

        puts("PPU zlib disabled!\n");
        dead_beep();

#endif

    }

    puts("decompress_result = ");
    print_decimal(decompress_result);

    puts(", xxx = ");
    print_decimal(xxx);

    puts("\n");

    if ((decompress_result != 0) || (xxx != original_size))
    {
        puts("decompress failed!\n");

        dead_beep();
    }

    *destSize = xxx;
}

#if DECRYPTLV2SELF_ENABLED

FUNC_DEF void DecryptLv2Self(void *inDest, const void *inSrc, void* decryptBuf, uint8_t use_spu)
{
    puts("DecryptLv2Self()\n");

    puts("decryptBuf = ");
    print_hex((uint64_t)decryptBuf);
    puts("\n");

    uint8_t *dest = (uint8_t *)inDest;
    const uint8_t *src = (const uint8_t *)inSrc;

    uint64_t curSrcOffset = 0;

    struct SceHeader_s sceHeader;
    memcpy(&sceHeader, &src[curSrcOffset], sizeof(struct SceHeader_s));

    if ((sceHeader.magic) != 0x53434500)
    {
        puts("SCE magic check failed!, ");
        print_hex(sceHeader.magic);
        puts("\n");

        dead_beep();
        return;
    }

    //puts("sceHeader:\n");

    //puts("ext_header_size = ");
    //print_hex(sceHeader.ext_header_size);
    //puts("\n");

    //puts("file_offset = ");
    //print_hex(sceHeader.file_offset);
    //puts("\n");

    //puts("file_size = ");
    //print_hex(sceHeader.file_size);
    //puts("\n");

    // erk=0CAF212B6FA53C0D A7E2C575ADF61DBE 68F34A33433B1B89 1ABF5C4251406A03
    // riv=9B79374722AD888E B6A35A2DF25A8B3E

    //puts("1\n");

    uint64_t meta_key[4];
    meta_key[0] = (0x0CAF212B6FA53C0D);
    meta_key[1] = (0xA7E2C575ADF61DBE);
    meta_key[2] = (0x68F34A33433B1B89);
    meta_key[3] = (0x1ABF5C4251406A03);

    uint64_t meta_iv[2];
    meta_iv[0] = (0x9B79374722AD888E);
    meta_iv[1] = (0xB6A35A2DF25A8B3E);

    //puts("2\n");

    WORD meta_aes_key[60];
    aes_key_setup((const uint8_t *)meta_key, meta_aes_key, 256);

    //puts("3\n");

    //curSrcOffset = 0x200;

    for (uint64_t i = 0; i < 0x1000; i += 0x10)
    {
        uint32_t* v = (uint32_t*)(&src[i]);

        if (*v == 0x627CB180)
        {
            curSrcOffset = (i + 0x30);
            break;
        }
    }

    struct SceMetaInfo_s metaInfo;
    aes_decrypt_cbc(

        &src[curSrcOffset],
        sizeof(struct SceMetaInfo_s),

        (uint8_t *)&metaInfo,

        meta_aes_key,
        256,

        (const uint8_t *)meta_iv);

    //puts("4\n");

    curSrcOffset += sizeof(struct SceMetaInfo_s);

    //puts("metaInfo:\n");

    //puts("metaInfo.key[0] = ");
    //print_hex(metaInfo.key[0]);
    //puts("\n");

    //puts("metaInfo.key[1] = ");
    //print_hex(metaInfo.key[1]);
    //puts("\n");

    //puts("metaInfo.iv[0] = ");
    //print_hex(metaInfo.iv[0]);
    //puts("\n");

    //puts("metaInfo.iv[1] = ");
    //print_hex(metaInfo.iv[1]);
    //puts("\n");

    WORD meta_header_key[60];
    aes_key_setup((const uint8_t *)metaInfo.key, meta_header_key, 128);

    uint64_t metasSize = (sceHeader.file_offset) - sizeof(struct SceHeader_s) + (sceHeader.ext_header_size) + sizeof(struct SceMetaInfo_s);

    //puts("metasSize = ");
    //print_decimal(metasSize);
    //puts("\n");

    uint8_t metasBuf[4096];

    aes_decrypt_ctr(

        &src[curSrcOffset],
        metasSize,

        metasBuf,

        meta_header_key,
        128,

        (const uint8_t *)metaInfo.iv

    );

    struct SceMetaHeader_s *metaHeader = (struct SceMetaHeader_s *)&metasBuf[0];

    //puts("metaHeader:\n");

    //puts("section_entry_num = ");
    //print_decimal(metaHeader->section_entry_num);
    //puts("\n");

    //puts("key_entry_num = ");
    //print_decimal(metaHeader->key_entry_num);
    //puts("\n");

    struct SceMetaSectionHeader_s *metaSectionHeaders = (struct SceMetaSectionHeader_s *)&metasBuf[sizeof(struct SceMetaHeader_s)];

#if 0

    for (uint32_t i = 0; i < (metaHeader->section_entry_num); ++i)
    {
        struct SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        puts("section_headers[");
        print_decimal(i);
        puts("]:\n");

        puts("segment_id = ");
        print_decimal(h->segment_id);

        puts(", segment_offset = ");
        print_hex(h->segment_offset);

        puts(", segment_size = ");
        print_hex(h->segment_size);

        puts(", enc_algorithm = ");
        print_decimal(h->enc_algorithm);

        puts(", key_idx = ");
        print_decimal(h->key_idx);

        puts(", iv_idx = ");
        print_decimal(h->iv_idx);

        puts("\n");
    }

#endif

    struct SceMetaKey_s *metaKeys = (struct SceMetaKey_s *)&metasBuf[sizeof(struct SceMetaHeader_s) + ((metaHeader->section_entry_num) * sizeof(struct SceMetaSectionHeader_s))];

#if 0

    for (uint32_t i = 0; i < (metaHeader->key_entry_num); ++i)
    {
        struct SceMetaKey_s *k = &metaKeys[i];

        puts("keys[");
        print_decimal(i);
        puts("]: ");

        puts("key[0] = ");
        print_hex(k->key[0]);

        puts(", key[1] = ");
        print_hex(k->key[1]);

        puts("\n");
    }

#endif

    struct ElfHeader_s *elfHeader = (struct ElfHeader_s *)&src[0x90];

    memcpy(dest, elfHeader, sizeof(struct ElfHeader_s));
    memcpy(dest + (elfHeader->e_phoff), &src[0x90 + (elfHeader->e_phoff)], (elfHeader->e_phentsize) * (elfHeader->e_phnum));

    struct ElfPhdr_s *elfPhdrs = (struct ElfPhdr_s *)(dest + (elfHeader->e_phoff));

    uint64_t spu_id = calc_myspu_id();
    uint64_t spu_old_mfc_sr1;

    if (use_spu)
        spu_old_mfc_sr1 = SpuAux_Init(spu_id);

    for (uint16_t i = 0; i < (elfHeader->e_phnum); ++i)
    {
        struct ElfPhdr_s *phdr = &elfPhdrs[i];

        puts("decrypting phdr ");
        print_decimal(i);
        puts("...\n");

        struct SceMetaSectionHeader_s *h = &metaSectionHeaders[i];

        struct SceMetaKey_s *key = &metaKeys[(h->key_idx)];
        struct SceMetaKey_s *iv = &metaKeys[(h->iv_idx)];

        puts("segment_offset = ");
        print_hex(h->segment_offset);
        puts(", segment_size = ");
        print_hex(h->segment_size);

        puts(", p_offset = ");
        print_hex(h->segment_offset);

        uint64_t in_addr = (uint64_t)&src[(h->segment_offset)];
        uint64_t out_addr = (uint64_t)dest + phdr->p_offset;

        puts(", in_addr = ");
        print_hex(in_addr);

        puts(", out_addr = ");
        print_hex(out_addr);

        puts("\n");

        memcpy(decryptBuf, (const void*)in_addr, h->segment_size);

        if (use_spu)
        {
            spu_aes128_decrypt_ctr(
                spu_id,

                (const uint8_t *)decryptBuf,
                (h->segment_size),

                (uint8_t *)decryptBuf,

                (const uint8_t *)key->key,
                (const uint8_t *)iv->key
            );
        }
        else
        {
            WORD aes_key[60];
            aes_key_setup((const uint8_t *)key->key, aes_key, 128);

            aes_decrypt_ctr(

                (const uint8_t *)decryptBuf,
                h->segment_size,

                (uint8_t *)decryptBuf,

                aes_key,
                128,

                (const uint8_t *)iv->key

            );
        }

        if (h->comp_algorithm == 2)
        {
            puts("decompressing...\n");

            uint32_t sz = phdr->p_filesz;

            int32_t res;
            
            if (use_spu)
            {
                uint64_t yyy;

                spu_zlib_decompress(spu_id, decryptBuf, h->segment_size, (void*)out_addr, &yyy);
                
                res = 0;
                sz = (uint32_t)yyy;
            }
            else
            {
                
#if !ZLIB_SPU_ONLY_ENABLED

                res = tinf_zlib_uncompress(
                    (void*)out_addr, &sz, 
                    decryptBuf, (uint32_t)h->segment_size
                );

#else

                puts("PPU zlib disabled!\n");
                dead_beep();

#endif

            }

            if ((res != 0) || (sz != phdr->p_filesz))
            {
                puts("Decompress failed!");

                puts(", sz = ");
                print_decimal(sz);

                puts(", p_filesz = ");
                print_decimal(phdr->p_filesz);

                puts("\n");

                dead();
            }
        }
        else
            memcpy((void*)out_addr, decryptBuf, h->segment_size);
    }

    if (use_spu)
        SpuAux_Uninit(spu_id, spu_old_mfc_sr1);

    puts("DecryptLv2Self() done.\n");
}

#endif

#include "Stage1.c"
#include "Stage2.c"
#include "Stage3.c"
#include "Stage4.c"
#include "Stage5.c"
#include "Stage6.c"

void stage_link_entry()
{
    asm volatile("bl stage1_entry");
    asm volatile("bl stage2_entry");
    asm volatile("bl stage3_entry");
    asm volatile("bl stage5_entry");
    asm volatile("bl stage6_entry");
}