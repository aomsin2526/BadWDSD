#define ASM(...) asm volatile(__VA_ARGS__)

#define eieio()                \
    {                          \
        ASM("eieio");          \
        ASM("sync");           \
    }

#define sync()                 \
    {                          \
        ASM("eieio");          \
        ASM("isync");          \
        ASM("sync");           \
    }
