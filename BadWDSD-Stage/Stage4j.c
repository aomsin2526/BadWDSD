// stage4j max size = 644

#pragma GCC optimize("align-functions=8")
#pragma GCC optimize("O1")

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long int64_t;
typedef unsigned long uint64_t;

typedef uint64_t size_t;

#define SC()              \
    asm volatile("sync"); \
    asm volatile("sc" :"=r"(r3), "=r"(r4), "=r"(r5), "=r"(r6), "=r"(r7):"r"(r11), "r"(r3), "r"(r4), "r"(r5), "r"(r6), "r"(r7):)

#define dead() \
    while (1)  \
    {          \
    }

// in: r3 = laid, r4 = dest, r5 = src
// out: r3 = 0 on success

__attribute__((noreturn)) void stage4j_entry()
{
    register uint64_t r3 asm("r3");

    register uint64_t r4 asm("r4");
    register uint64_t r5 asm("r5");
    register uint64_t r6 asm("r6");
    register uint64_t r7 asm("r7");

    // local use
    register uint64_t r0 asm("r0");
    register uint64_t r8 asm("r8");
    register uint64_t r9 asm("r9");
    register uint64_t r10 asm("r10");

    register uint64_t r11 asm("r11");

    // push stack
    asm volatile("addi 1, 1, -128");

    // store lr
    asm volatile("mflr 0");
    asm volatile("std 0, 0(1)");

    // store laid
    asm volatile("std 3, 8(1)");

    // store dest
    asm volatile("std 4, 16(1)");

    // store src
    asm volatile("std 5, 24(1)");

    {
        // copy src starts

        r11 = 0x10003;

        r4 = 0x6996;
        r5 = 0x20;

        r6 = 0x1111;

        SC();

        // copy src to ra 0xC000000

        {
            // r0 = temp
            // r3 = src
            // r8 = curOffset
            // r9 = left
            // r10 = temp

            // r3 = src
            asm volatile("ld %0, 24(1)" : "=r"(r3)::); // example: 0xa0008000

            // r8 = 0
            asm volatile("li %0, 0" : "=r"(r8)::);

            // r9 = 0
            asm volatile("li %0, 0" : "=r"(r9)::);

            // r10 = file offset
            asm volatile("ld %0, 0x10(%1)" : "=r"(r10) : "r"(r3) :);

            // r9 += file offset
            asm volatile("add %0, %1, %2" : "=r"(r9) : "r"(r9), "r"(r10) :);

            // r10 = file size
            asm volatile("ld %0, 0x18(%1)" : "=r"(r10) : "r"(r3) :);

            // r9 += file size
            asm volatile("add %0, %1, %2" : "=r"(r9) : "r"(r9), "r"(r10) :);

            //
            asm volatile("src_copy_loop:");

            r11 = 0x10003;

            r4 = 0x6996;

            r6 = 0xC000000;
            r6 += r8;

            r10 = r3;
            r10 += r8;

            if (r9 < 8)
            {
                r5 = 0x8; // poke8
                asm volatile("lbz %0, 0(%1)" : "=r"(r7) : "r"(r10) :);
                r10 = 1;
            }
            else
            {
                r5 = 0x2; // poke64
                asm volatile("ld %0, 0(%1)" : "=r"(r7) : "r"(r10) :);
                r10 = 8;
            }

            // store register to stack
            asm volatile("std %0, 32(1)" ::"r"(r0) :);
            asm volatile("std %0, 40(1)" ::"r"(r8) :);
            asm volatile("std %0, 48(1)" ::"r"(r9) :);
            asm volatile("std %0, 56(1)" ::"r"(r10) :);
            asm volatile("std %0, 64(1)" ::"r"(r3) :);

            SC();

            // restore register from stack
            asm volatile("ld %0, 64(1)" : "=r"(r3)::);
            asm volatile("ld %0, 56(1)" : "=r"(r10)::);
            asm volatile("ld %0, 48(1)" : "=r"(r9)::);
            asm volatile("ld %0, 40(1)" : "=r"(r8)::);
            asm volatile("ld %0, 32(1)" : "=r"(r0)::);

            // curOffset += r10
            asm volatile("add %0, %1, %2" : "=r"(r8) : "r"(r8), "r"(r10) :);

            // left -= r10
            asm volatile("sub %0, %1, %2" : "=r"(r9) : "r"(r9), "r"(r10) :);

            // break if left == 0
            asm volatile("li %0, 0" : "=r"(r0)::);
            asm volatile("cmp 0, 0, %0, %1" ::"r"(r9), "r"(r0) :);
            asm volatile("beq src_copy_done");

            asm volatile("b src_copy_loop");
        }

        asm volatile("src_copy_done:");

        // copy src done

        r11 = 0x10003;

        r4 = 0x6996;
        r5 = 0x20;

        r6 = 0x2222;

        SC();
    }

    // 4C4F4144 4D455858 58584C4F 41444D45
    // write loadme into dest

    {
        // r10 = dest
        asm volatile("ld %0, 16(1)" : "=r"(r10)::);

        r3 = 0x4C4F4144;
        asm volatile("stw %0, 0(%1)" ::"r"(r3), "r"(r10) :);
        r10 += 4;

        r3 = 0x4D455858;
        asm volatile("stw %0, 0(%1)" ::"r"(r3), "r"(r10) :);
        r10 += 4;

        r3 = 0x58584C4F;
        asm volatile("stw %0, 0(%1)" ::"r"(r3), "r"(r10) :);
        r10 += 4;

        r3 = 0x41444D45;
        asm volatile("stw %0, 0(%1)" ::"r"(r3), "r"(r10) :);
        r10 += 4;
    }

    // Call stage4

    r11 = 0x10003;

    r4 = 0x6996;
    r5 = 0x21;

    SC();

    // everything done

    r11 = 0x10003;

    r4 = 0x6996;
    r5 = 0x20;

    r6 = 0x3333;

    SC();

    //

    // restore lr
    asm volatile("ld 0, 0(1)");
    asm volatile("mtlr 0");

    // pop stack
    asm volatile("addi 1, 1, 128");

    // set r3
    asm volatile("li 3, 0");

    // return
    asm volatile("blr");

    __builtin_unreachable();
}