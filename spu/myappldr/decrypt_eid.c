typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

uint32_t start(void* junk, uint32_t* outTargetID, uint64_t* outIDPS)
{
    uint64_t* targetID = (uint64_t*)0x39060;
    *outTargetID = (uint32_t)*targetID;

    uint64_t* idps = (uint64_t*)0x39050;
    outIDPS[0] = idps[0];
    outIDPS[1] = idps[1];

    return 0;
}