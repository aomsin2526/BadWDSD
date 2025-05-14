#pragma GCC optimize("align-functions=8")

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long int64_t;
typedef unsigned long uint64_t;

typedef uint64_t size_t;

__attribute__((noreturn)) void patches()
{
    //bl        memcpy(uchar *,uchar *,uint) # r3 = dest ptr
    //ld        r0, 0x80(r31)
    //std       r0, 0(r28)

    //4B FF 45 D1 E8 1F 00 80 F8 1C 00 00 38 9D 00 08

    register uint64_t r0 asm("r0");
    //0x4008400000000
    
    //r0 = 0x3006000000000;

    asm volatile("li %0, 0x3006" :"=r"(r0)::);
    asm volatile("sldi %0, %1, 36" :"=r"(r0):"r"(r0):);

    asm volatile("std %0, 0(28)" ::"r"(r0):);

    //38 00 30 06 78 00 26 C6 F8 1C 00 00 38 9D 00 08

    // 0x4000000000000

    //lis       r0, 4
    //sldi      r0, r0, 32
    //std       r0, 0(r28)

    //3C 00 00 04 78 00 07 C6 F8 1C 00 00 38 9D 00 08

    __builtin_unreachable();
}
