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
