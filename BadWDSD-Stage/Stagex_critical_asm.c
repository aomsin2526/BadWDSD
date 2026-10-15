#pragma GCC optimize("align-functions=8") // this is needed to execute directly on nor, DO NOT REMOVE!!!!!!
#pragma GCC diagnostic ignored "-Wunused-function"

// gc sections wont work
//#define FUNC_DECL __attribute__((section("code")))

#define FUNC_DECL
#define FUNC_DEF FUNC_DECL

// branch-able code
#define FUNC_DECL_BCODE __attribute__((section("bcode")))
#define FUNC_DEF_BCODE FUNC_DECL_BCODE

#define ASM(...) asm volatile(__VA_ARGS__ ::: "memory")
#define ASMX(...) asm volatile(__VA_ARGS__ "memory")

#define eieio()                \
    {                          \
        ASM("eieio");          \
        ASM("sync");           \
    }

#define dcbf(___in_addr_register)                                                  \
    {                                                                              \
        ASMX("dcbf %0, %1" ::"r"(stage_zero), "r"(___in_addr_register) :);         \
        ASM("sync");                                                               \
    }
