#include "Stagexldr_emmc_critical_asm.c"

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

register uint64_t is_emmc asm("r16");

#include "Stagexldr_emmc_critical_hw.c"

#include "Stagexldr_emmc_critical_dead.c"

#define SYS_TIMEBASE_GET(tb)                          \
    do                                                \
    {                                                 \
        ASM("1: mftb %[current_tb];"                  \
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

//

#include "Stagexldr_emmc_critical_volatile.c"

//

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
    while ((volatile_read_uint32(0x24000FFF308) & 0x100) == 0) {}

    volatile_write_uint32(0x24000FFF31C, c);
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
FUNC_DEF uint32_t crc32c(uint32_t crc, const uint8_t* buf, uint64_t len)
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
        dead();

    //

    static const uint64_t a_send_packet_counter_cell = 0x2400008DFF0;

    static const uint64_t a_send_packet_data = 0x2400008D000;
    static const uint64_t a_send_packet_kick = 0x2400008E100;

    //

    static const uint64_t a_recieve_packet_counter_sc = 0x2400008CFF0;
    static const uint64_t a_recieve_packet_counter_cell = 0x2400008DFF4;

    static const uint64_t a_recieve_packet_test = 0x2400008E000;

    static const uint64_t a_recieve_packet_data = 0x2400008C000;
    const volatile uint32_t* p_recieve_packet_data = (const volatile uint32_t*)a_recieve_packet_data;

    //

    {
        uint16_t tag = (uint16_t)GetTimeInMs();

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
                const uint32_t* p = (const uint32_t*)buf;

                for (uint32_t i = 0; i < (512 / 4); ++i)
                    volatile_write_uint32((a_send_packet_data + (i * 4)), p[i]);
            }
        }

        {
            uint32_t value = volatile_read_uint32(a_send_packet_counter_cell);

            value = value + 1;
            value &= 0xffff;
            value = (value << 16) | value;

            volatile_write_uint32(a_send_packet_counter_cell, value);
        }

        const uint32_t old_recieve_packet_counter_sc = volatile_read_uint32(a_recieve_packet_counter_sc);

        volatile_write_uint32(a_send_packet_kick, 0x1);

        {
            uint32_t v = volatile_read_uint32(a_recieve_packet_test);
            v |= 0xFFFFFFFD;

            volatile_write_uint32(a_recieve_packet_test, v);
        }

        while (1)
        {
            if (volatile_read_uint32(a_recieve_packet_counter_sc) != old_recieve_packet_counter_sc)
                break;
        }

        {
            uint32_t value = volatile_read_uint32(a_recieve_packet_counter_cell);

            value = value + 1;
            value &= 0xffff;
            value = (value << 16) | value;

            volatile_write_uint32(a_recieve_packet_counter_cell, value);
        }

        struct sc_real_packet_header_s out_real_pkt_hdr;

        {
            uint32_t *p = (uint32_t *)&out_real_pkt_hdr;

            for (uint32_t i = 0; i < 4; i++)
                p[i] = p_recieve_packet_data[i];

            eieio();
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
                    p[i] = p_recieve_packet_data[(i + 4)];

                eieio();
            }
        }
    }

    //
}

FUNC_DEF void sc_puts(const char *str)
{
    uint64_t len = strlen(str);

    if (len == 0)
        return;

    if (len > 199)
        dead();

    struct sc_packet_s pkt;

    pkt.service_id = 0x20;
    pkt.communication_tag = 1;

    pkt.payload_size = 1;
    pkt.data[0] = 0x00;

    memcpy(&pkt.data[pkt.payload_size], str, (len + 1));
    pkt.payload_size += (len + 1);

    sc_send_packet(&pkt, NULL);
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

    sc_send_packet(&pkt, NULL);
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

FUNC_DEF uint8_t sc_read_flash_type()
{
    // block id (0x3000)
    // offset (0x3007)
    return sc_read_eeprom8(0x20, 0x7);
}

FUNC_DEF uint32_t sc_read_ros0_crc32()
{
    uint32_t v;
    uint8_t* v2 = (uint8_t*)&v;

    // block id (0x3000)
    // offset (0x3008 - 0x300b)
    for (uint32_t i = 0; i < 4; ++i)
        v2[i] = sc_read_eeprom8(0x20, (0x8 + i));

    return v;
}

FUNC_DEF uint32_t sc_read_ros1_crc32()
{
    uint32_t v;
    uint8_t* v2 = (uint8_t*)&v;

    // block id (0x3000)
    // offset (0x300c - 0x300f)
    for (uint32_t i = 0; i < 4; ++i)
        v2[i] = sc_read_eeprom8(0x20, (0xc + i));

    return v;
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

FUNC_DEF void sc_shutdown()
{
    struct sc_packet_s pkt;

    pkt.service_id = 0x13;
    pkt.communication_tag = 1;

    pkt.payload_size = 4;

    pkt.data[0] = 0x11;
    pkt.data[1] = 0x00;
    pkt.data[2] = 0x00;
    pkt.data[3] = 0x00;

    sc_send_packet(&pkt, NULL);
    dead();
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
    dead();
}

#include "Stagexldr_emmc_critical_dead_beep.c"

#include "Stagexldr_emmc_critical.c"

//

static const uint64_t stagex_max_size = (60 * 1024);

FUNC_DEF void Stagex_Relocate(const volatile void* stagex_data, uint64_t old_stagex_addr, uint64_t new_stagex_addr)
{
    puts("Stagex_Relocate,  ");

    print_hex(old_stagex_addr);
    puts(" -> ");
    print_hex(new_stagex_addr);

    puts("\n");

    static const uint64_t stagex_size = stagex_max_size;

    volatile_memcpy((void*)new_stagex_addr, stagex_data, stagex_size);

    const uint64_t* signature = (const uint64_t*)(new_stagex_addr + 0x908);

    if (*signature != 0x5446072c5516c2c6)
    {
        puts("bad signature!\n");
        dead_beep();
    }

    uint64_t* toc1_addr = (uint64_t*)(new_stagex_addr + 0x910);
    const uint64_t* toc1_size = (const uint64_t*)(new_stagex_addr + 0x918);

    if (*toc1_size > 0)
    {
        uint64_t* toc = (uint64_t*)(new_stagex_addr + 0x900);
        *toc -= old_stagex_addr;
        *toc += new_stagex_addr;

        *toc1_addr -= old_stagex_addr;
        *toc1_addr += new_stagex_addr;

        uint64_t* toc1 = (uint64_t*)(*toc1_addr);

        for (uint64_t i = 0; i < (*toc1_size / 8); ++i)
        {
            toc1[i] -= old_stagex_addr;
            toc1[i] += new_stagex_addr;
        }
    }
}

FUNC_DEF void RelocateStagexAndJumpToStage1(const volatile void* stagex_data)
{
    static const uint64_t new_stagex_addr = 0x1010000;
    Stagex_Relocate(stagex_data, 0x2401FF21000, new_stagex_addr);

    puts("Jumping to Stage1...\n");

    ASM("mtctr %0" ::"r"(new_stagex_addr):);
    ASM("bctr");

    dead_beep();
}

FUNC_DEF void badwdsd_ok()
{
    sc_puts("BadWDSD ok!\n");
}

FUNC_DEF void Stagexldr()
{
    puts("\nStagexldr by Kafuu(aomsin2526)" " (Build Date: " __DATE__ " " __TIME__ ")\n");

    {
        uint32_t payload_size = *((const uint32_t*)0xff8);
        uint32_t payload_crc32 = *((const uint32_t*)0xffc);

        puts("payload_size = ");
        print_decimal(payload_size);
        puts("\n");

        puts("payload_crc32 = ");
        print_hex(payload_crc32);
        puts("\n");

        if ((payload_size == 0) || (payload_size > (1 * 1024 * 1024)))
        {
            puts("bad!!!\n");
            dead_beep();
        }

        uint32_t crc32 = crc32c(0, (const uint8_t*)0x1000, payload_size);

        if (crc32 != payload_crc32)
        {
            puts("bad crc!!!\n");
            dead_beep();
        }

        puts("crc32 check ok!\n");
    }

    //

    is_emmc = FetchIsEmmc();

    //

    if (!is_emmc)
    {
        puts("Flash is NOR\n");

        RelocateStagexAndJumpToStage1((const volatile void*)0x2401FF21000);
        dead_beep();
    }

    //

    puts("Flash is eMMC\n");

    //

#if 0
    // print lv0ldr region
    {
        // inform modchip that we are good...
        badwdsd_ok();

        static const uint64_t dumpSize = (256 * 1024);

        uint8_t buf[dumpSize];
        uint8_t buf2[dumpSize];

        emmc_read(0, buf, dumpSize);
        emmc_read(0, buf2, dumpSize);

        uint32_t c1 = crc32c(0, buf, dumpSize);
        uint32_t c2 = crc32c(0, buf2, dumpSize);

        puts("c1 = ");
        print_hex(c1);
        puts("\n");

        puts("c2 = ");
        print_hex(c2);
        puts("\n");

        if (c1 != c2)
        {
            puts("crc fail!\n");
            dead_beep();
        }

        puts("start\n");
        hexdump(buf, dumpSize);
        puts("\n");
        puts("end\n");

        dead_beep();
    }
#endif

    //

    {
        uint8_t request_os_bank_indicator = sc_read_request_os_bank_indicator();

        puts("request_os_bank_indicator = ");
        print_hex(request_os_bank_indicator);
        puts("\n");

        if (request_os_bank_indicator != 0xff)
        {
            // inform modchip that we are good...
            badwdsd_ok();
            WaitInMs(10000);
        }

        sc_write_request_os_bank_indicator(0xff);

        if (request_os_bank_indicator == 0x1)
            switch_ros(0xff);
        else if (request_os_bank_indicator == 0x2)
            switch_ros(0x00);
    }

    //

    {
        uint8_t real_os_bank_indicator = get_os_bank_indicator_from_ros();

        puts("real_os_bank_indicator = ");
        print_hex(real_os_bank_indicator);
        puts("\n");

        uint8_t shadow_os_bank_indicator = ((real_os_bank_indicator == 0xFF) ? 0x1 : 0x2); // ros0 or ros1
        sc_write_shadow_os_bank_indicator(shadow_os_bank_indicator);

        puts("shadow_os_bank_indicator = ");
        print_hex(shadow_os_bank_indicator);
        puts("\n");
    }

    //

    {
        static const uint64_t stagex_addr = 0x1010000;

        emmc_read(0xA1000, (void*)stagex_addr, stagex_max_size);
        RelocateStagexAndJumpToStage1((const void*)stagex_addr);
    }

    //

    dead_beep();
}

__attribute__((section("main"))) void stagexldr_main()
{
    memset((void*)0x0, 0x0, 0xA00);

    Stagexldr();
    dead_beep();
}

__attribute__((noreturn, section("entry"))) void stagexldr_entry()
{
    // .toc
    ASM("li %r2, 0x1100");

    // += 0x8000
    ASM("addi %r2, %r2, 0x7fff");
    ASM("addi %r2, %r2, 1");

    // set sp to 0x1FFFF00
    ASM("lis %r1, 0x200");
    ASM("addi %r1, %r1, -0x100");

    // init ppu
    ASM("bl HW_Init");

    // push stack
    ASM("addi %r1, %r1, -128");

    // jump to stagexldr_main
    ASM("b stagexldr_main");

    //
    __builtin_unreachable();
}

void stagexldr_link_entry()
{
    ASM("bl stagexldr_entry");
}