struct LoadLv2KernelType_e
{
public:
    static const uint32_t Self = 0;
    static const uint32_t Fself = 1;

    static const uint32_t OtherOS_Fself = 2;
    static const uint32_t OtherOS_ZFself = 3;
};

extern void LoadLv2Kernel(const char* fileName, uint32_t type);