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
    // r5 = out

    register uint64_t r8 asm("r8");

    // out[0] = 0x00000000 0x00007859
    // out[1] = 0x00000000 0x00007a4d
    // out[2] = 0x00000000 0x00000000
    // out[3] = 0x00000000 0x00000001

    r8 = 0x00000000;
    asm volatile("stw %0, 0(5)" ::"r"(r8):);

    r8 = 0x00007859;
    asm volatile("stw %0, 4(5)" ::"r"(r8):);

    r8 = 0x00000000;
    asm volatile("stw %0, 8(5)" ::"r"(r8):);

    r8 = 0x00007a4d;
    asm volatile("stw %0, 12(5)" ::"r"(r8):);

    r8 = 0x00000000;
    asm volatile("stw %0, 16(5)" ::"r"(r8):);

    r8 = 0x00000000;
    asm volatile("stw %0, 20(5)" ::"r"(r8):);

    r8 = 0x00000000;
    asm volatile("stw %0, 24(5)" ::"r"(r8):);

    r8 = 0x00000001;
    asm volatile("stw %0, 28(5)" ::"r"(r8):);

    //

    asm volatile("li 3, 0");

    asm volatile("blr");
    __builtin_unreachable();
}