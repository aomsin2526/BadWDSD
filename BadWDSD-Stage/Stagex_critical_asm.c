#pragma GCC optimize("align-functions=8")
#pragma GCC diagnostic ignored "-Wunused-function"

#define FUNC_DECL __attribute__((section("code")))
#define FUNC_DEF FUNC_DECL

// branch-able code
#define FUNC_DECL_BCODE __attribute__((section("bcode")))
#define FUNC_DEF_BCODE FUNC_DECL_BCODE

#define ASM(...) asm volatile(__VA_ARGS__)

#define eieio()                \
    {                          \
        ASM("eieio");          \
        ASM("sync");           \
    }

#define dcbf(___in_addr_register)                                                  \
    {                                                                              \
        ASM("dcbf %0, %1" ::"r"(stage_zero), "r"(___in_addr_register) :);          \
        ASM("sync");                                                               \
    }

#define sync()                 \
    {                          \
        ASM("eieio");          \
        ASM("isync");          \
        ASM("sync");           \
    }
