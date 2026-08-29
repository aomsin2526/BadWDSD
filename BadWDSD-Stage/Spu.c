static const uint32_t SPU_STATUS_RUN_MASK = (1 << 0);
static const uint32_t SPU_STATUS_ISOLATED_MASK = (1 << 7);

static const uint8_t SPU_MFC_PUT_CMD = 0x20;
static const uint8_t SPU_MFC_GET_CMD = 0x40;

FUNC_DEF uint64_t SPU_CalcMMIOAddress(uint64_t spu_id, uint64_t offset)
{
    return (0x20000000000 + (0x80000 * spu_id) + offset);
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_LS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset);
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_PS(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x40000;
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_P2(uint64_t spu_id, uint64_t offset)
{
    return SPU_CalcMMIOAddress(spu_id, offset) + 0x60000;
}

FUNC_DEF uint64_t SPU_CalcMMIOAddress_P1(uint64_t spu_id, uint64_t offset)
{
    return (0x20000400000 + (0x2000 * spu_id) + offset);
}

FUNC_DEF uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_LS_Read8(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint8_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write8(uint64_t spu_id, uint64_t offset, uint8_t value)
{
    *((volatile uint8_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint64_t SPU_P1_Read64(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_P1_Read32(uint64_t spu_id, uint64_t offset)
{
    eieio();
    return *((const volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
    eieio();
}

FUNC_DEF uint32_t SPU_Read_SPU_STATUS(uint64_t spu_id)
{
    return SPU_PS_Read32(spu_id, 0x04024);
}

FUNC_DEF uint32_t SPU_Read_SPU_NPC(uint64_t spu_id)
{
    return SPU_PS_Read32(spu_id, 0x04034);
}

FUNC_DEF void SPU_Write_SPU_NPC(uint64_t spu_id, uint32_t val)
{
    SPU_PS_Write32(spu_id, 0x04034, val);
}

FUNC_DEF uint32_t SPU_Read_SPU_RUNCNTL(uint64_t spu_id)
{
    return SPU_PS_Read32(spu_id, 0x0401C);
}

FUNC_DEF void SPU_Write_SPU_RUNCNTL(uint64_t spu_id, uint32_t val)
{
    SPU_PS_Write32(spu_id, 0x0401C, val);
}

FUNC_DEF uint32_t SPU_Read_SPU_MBOX_STAT(uint64_t spu_id)
{
    return SPU_PS_Read32(spu_id, 0x04014);
}

FUNC_DEF uint32_t SPU_Read_SPU_OUT_MBOX(uint64_t spu_id)
{
    return SPU_PS_Read32(spu_id, 0x04004);
}

FUNC_DEF uint64_t SPU_Read_INT_STAT_CLASS0(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x140);
}

FUNC_DEF uint64_t SPU_Read_INT_STAT_CLASS2(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x150);
}

FUNC_DEF uint64_t SPU_Read_INT_STAT_CLASS1(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x148);
}

FUNC_DEF void SPU_Write_INT_STAT_CLASS1(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x148, val);
}

FUNC_DEF uint8_t SPU_IsMF_INT_STAT_CLASS1(uint64_t spu_id)
{
    return ((SPU_Read_INT_STAT_CLASS1(spu_id) & 0x2) != 0) ? 1 : 0;
}

FUNC_DEF uint8_t SPU_IsSF_INT_STAT_CLASS1(uint64_t spu_id)
{
    return ((SPU_Read_INT_STAT_CLASS1(spu_id) & 0x1) != 0) ? 1 : 0;
}

FUNC_DEF uint8_t SPU_IsLSCmp_INT_STAT_CLASS1(uint64_t spu_id)
{
    return ((SPU_Read_INT_STAT_CLASS1(spu_id) & 0xc) != 0) ? 1 : 0;
}

FUNC_DEF void SPU_Clear_INT_STAT_CLASS1(uint64_t spu_id)
{
    SPU_Write_INT_STAT_CLASS1(spu_id, ~0);
}

FUNC_DEF void SPU_ClearSF_INT_STAT_CLASS1(uint64_t spu_id)
{
    SPU_Write_INT_STAT_CLASS1(spu_id, 0x1);
}

FUNC_DEF void SPU_ClearMF_INT_STAT_CLASS1(uint64_t spu_id)
{
    SPU_Write_INT_STAT_CLASS1(spu_id, 0x2);
}

FUNC_DEF void SPU_ClearLSCmp_INT_STAT_CLASS1(uint64_t spu_id)
{
    SPU_Write_INT_STAT_CLASS1(spu_id, 0xc);
}

FUNC_DEF uint64_t SPU_Read_MFC_SR1(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x0);
}

FUNC_DEF void SPU_Write_MFC_SR1(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x0, val);
}

FUNC_DEF void SPU_EnableRelocate(uint64_t spu_id)
{
    uint64_t val = SPU_Read_MFC_SR1(spu_id);
    val |= (1ULL << 4);
    SPU_Write_MFC_SR1(spu_id, val);
}

FUNC_DEF void SPU_DisableRelocate(uint64_t spu_id)
{
    uint64_t val = SPU_Read_MFC_SR1(spu_id);
    val &= ~(1ULL << 4);
    SPU_Write_MFC_SR1(spu_id, val);
}

FUNC_DEF void SPU_EnableSoftwareTLB(uint64_t spu_id)
{
    uint64_t val = SPU_Read_MFC_SR1(spu_id);
    val |= (1ULL << 6);
    SPU_Write_MFC_SR1(spu_id, val);
}

FUNC_DEF void SPU_DisableSoftwareTLB(uint64_t spu_id)
{
    uint64_t val = SPU_Read_MFC_SR1(spu_id);
    val &= ~(1ULL << 6);
    SPU_Write_MFC_SR1(spu_id, val);
}

FUNC_DEF uint64_t SPU_Read_MFC_LSACR(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x808);
}

FUNC_DEF void SPU_Write_MFC_LSACR(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x808, val);
}

FUNC_DEF uint64_t SPU_Read_MFC_LSCRR(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x810);
}

FUNC_DEF void SPU_Write_MFC_LSCRR(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x810, val);
}

FUNC_DEF uint64_t SPU_Read_MFC_ACCR(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x600);
}

FUNC_DEF void SPU_Write_MFC_ACCR(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x600, val);
}

FUNC_DEF void SPU_EnableLSCmp(uint64_t spu_id)
{
    SPU_Write_MFC_LSCRR(spu_id, 0);

    uint64_t value = SPU_Read_MFC_ACCR(spu_id);
    value |= 0b10000;
    value |= 0b01000;

    SPU_Write_MFC_ACCR(spu_id, value);
}

FUNC_DEF void SPU_DisableLSCmp(uint64_t spu_id)
{
    uint64_t value = SPU_Read_MFC_ACCR(spu_id);

    uint64_t mask = 0;
    mask |= 0b10000;
    mask |= 0b01000;

    value &= ~mask;

    SPU_Write_MFC_ACCR(spu_id, value);
}

FUNC_DEF void SPU_EnableEACmp(uint64_t spu_id)
{
    SPU_Write_MFC_LSCRR(spu_id, 0);

    uint64_t value = SPU_Read_MFC_ACCR(spu_id);
    value |= 0b00010;
    value |= 0b00001;

    SPU_Write_MFC_ACCR(spu_id, value);
}

FUNC_DEF void SPU_DisableEACmp(uint64_t spu_id)
{
    uint64_t value = SPU_Read_MFC_ACCR(spu_id);

    uint64_t mask = 0;
    mask |= 0b00010;
    mask |= 0b00001;

    value &= ~mask;

    SPU_Write_MFC_ACCR(spu_id, value);
}

FUNC_DEF uint64_t SPU_Read_MFC_DSISR(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x610);
}

FUNC_DEF void SPU_Write_MFC_DSISR(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x610, val);
}

FUNC_DEF uint64_t SPU_Read_MFC_DAR(uint64_t spu_id)
{
    return SPU_P1_Read64(spu_id, 0x620);
}

FUNC_DEF void SPU_Write_MFC_DAR(uint64_t spu_id, uint64_t val)
{
    SPU_P1_Write64(spu_id, 0x620, val);
}

FUNC_DEF uint64_t SPU_Read_MFC_CNTL(uint64_t spu_id)
{
    return SPU_P2_Read64(spu_id, 0x3000);
}

FUNC_DEF void SPU_Write_MFC_CNTL(uint64_t spu_id, uint64_t val)
{
    SPU_P2_Write64(spu_id, 0x3000, val);
}

FUNC_DEF void SPU_MFC_ResumeLSCmp(uint64_t spu_id)
{
    SPU_Write_MFC_CNTL(spu_id, 0x0);
}

FUNC_DEF void SPU_MFC_ResumeSegmentFault(uint64_t spu_id)
{
    SPU_Write_MFC_CNTL(spu_id, 0x100000000);
}

FUNC_DEF void SPU_MFC_ResumeMappingFault(uint64_t spu_id)
{
    SPU_MFC_ResumeSegmentFault(spu_id);
}

FUNC_DEF void SPU_MFC_PurgeAllCommands(uint64_t spu_id)
{
    SPU_Write_MFC_CNTL(spu_id, 0x8000);
}

FUNC_DEF uint64_t SPU_Read_MFC_RMAB(uint64_t spu_id)
{
    return SPU_P2_Read64(spu_id, 0x900);
}

FUNC_DEF void SPU_Write_MFC_RMAB(uint64_t spu_id, uint64_t val)
{
    SPU_P2_Write64(spu_id, 0x900, val);
}

static const uint64_t SPU_SLB_EA_MASK = 0xFFFFFFFFF0000000ULL;
static const uint64_t SPU_SLB_VA_MASK = SPU_SLB_EA_MASK;

FUNC_DEF void SPU_SetSLB(uint64_t spu_id, uint64_t ea, uint64_t va)
{
    // lower 28 bits of ea and va must not be set
    
    if ((ea & ~SPU_SLB_EA_MASK) != 0)
    {
        puts("Bad slb ea!!\n");
        dead_beep();
    }

    if ((va & ~SPU_SLB_VA_MASK) != 0)
    {
        puts("Bad slb va!!\n");
        dead_beep();
    }

    uint64_t esid = ea;
    esid &= SPU_SLB_EA_MASK;
    esid |= 0x8000000; // V

    uint64_t vsid = 0;
    vsid |= (va >> 28);
    vsid <<= 12;

    // SLB_Index
    SPU_P2_Write64(spu_id, 0x1108, 0);

    // Order is important!

    // SLB_VSID
    SPU_P2_Write64(spu_id, 0x1118, vsid);

    // SLB_ESID
    SPU_P2_Write64(spu_id, 0x1110, esid);
}

FUNC_DEF void SPU_InvalidateTLB(uint64_t spu_id, uint64_t tlbIndex)
{
    // TLB_Index
    SPU_P1_Write64(spu_id, 0x508, tlbIndex);

    // Order is important!

    // TLB_RPN
    SPU_P1_Write64(spu_id, 0x518, 0x0);

    // TLB_VPN
    SPU_P1_Write64(spu_id, 0x510, 0x0);
}

static const uint64_t SPU_TLB_VA_MASK = 0xFFFFFFFFFFFFF000ULL;
static const uint64_t SPU_TLB_RA_MASK = 0xFFFFFFFFFFFFF000ULL;

FUNC_DEF uint64_t SPU_SetTLB(uint64_t spu_id, uint64_t va, uint64_t ra)
{
    // lower 12 bits of va must not be set

    if ((va & ~SPU_TLB_VA_MASK) != 0)
    {
        puts("Bad tlb va!!\n");
        dead_beep();
    }

    if ((ra & ~SPU_TLB_RA_MASK) != 0)
    {
        puts("Bad tlb ra!!\n");
        dead_beep();
    }

    uint64_t vpn = va;
    vpn >>= 12;

    uint64_t avpn = vpn;
    avpn >>= 11;

    uint64_t lvpn = vpn;
    lvpn &= 0x7FF;

    uint64_t rpn = ra;
    rpn >>= 12;

    uint64_t vpn_val = avpn;
    vpn_val <<= 7;
    vpn_val |= 1; // V

    uint64_t rpn_val = rpn;
    rpn_val <<= 12;
    rpn_val |= 0x200; // AC
    rpn_val |= 0x180; // R C
    rpn_val |= 0x28; // I G

    uint64_t tlbIndexHint = SPU_P1_Read64(spu_id, 0x500);

    uint64_t tlbIndex = lvpn;
    tlbIndex <<= 37;
    tlbIndex |= tlbIndexHint;

    // TLB_Index
    SPU_P1_Write64(spu_id, 0x508, tlbIndex);

    // Order is important!

    // TLB_RPN
    SPU_P1_Write64(spu_id, 0x518, rpn_val);

    // TLB_VPN
    SPU_P1_Write64(spu_id, 0x510, vpn_val);

    return tlbIndex;
}

FUNC_DEF void SPU_StopRequest(uint64_t spu_id)
{
    uint32_t status = SPU_Read_SPU_STATUS(spu_id);

    if ((status & SPU_STATUS_RUN_MASK) != 0)
    {
        // stop request
        SPU_Write_SPU_RUNCNTL(spu_id, 0x0);

        while ((status & SPU_STATUS_RUN_MASK) != 0)
            status = SPU_Read_SPU_STATUS(spu_id);
    }
}

FUNC_DEF void SPU_StartRequest(uint64_t spu_id)
{
    SPU_Write_SPU_RUNCNTL(spu_id, 0x1);
}

FUNC_DEF void SPU_DoIsoExitRequest(uint64_t spu_id)
{
    SPU_Write_SPU_RUNCNTL(spu_id, 0x2);
}

FUNC_DEF void SPU_IsoExitRequest(uint64_t spu_id)
{
    uint32_t status = SPU_Read_SPU_STATUS(spu_id);
    
    if ((status & SPU_STATUS_ISOLATED_MASK) != 0)
    {
        // iso exit request
        SPU_DoIsoExitRequest(spu_id);

        while ((status & SPU_STATUS_ISOLATED_MASK) != 0)
            status = SPU_Read_SPU_STATUS(spu_id);
    }
}

FUNC_DEF void SPU_IsoLoadRequest(uint64_t spu_id)
{
    SPU_Write_SPU_RUNCNTL(spu_id, 0x3);
}

FUNC_DEF void LoadElfSpu(uint64_t elfFileAddress, uint64_t spu_id, uint8_t quiet)
{
    if (!quiet)
        puts("LoadElfSpu()\n");

    const struct ElfHeader32_s *elfHdr = (const struct ElfHeader32_s *)elfFileAddress;

    if (*((const uint32_t *)elfHdr->e_ident) != 0x7F454C46)
    {
        puts("LoadElfSpu e_ident check failed!\n");
        dead_beep();
    }

    if (!quiet)
    {
        puts("spu_id = ");
        print_decimal(spu_id);
        puts("\n");

        puts("e_entry = ");
        print_hex(elfHdr->e_entry);
        puts("\n");

        puts("e_phoff = ");
        print_hex(elfHdr->e_phoff);
        puts("\n");

        puts("e_phentsize = ");
        print_decimal(elfHdr->e_phentsize);
        puts("\n");

        puts("e_phnum = ");
        print_hex(elfHdr->e_phnum);
        puts("\n");
    }

    uint64_t curPhdrAddress = (elfFileAddress + elfHdr->e_phoff);

    for (uint16_t i = 0; i < elfHdr->e_phnum; ++i)
    {
        const struct ElfPhdr32_s *phdr = (const struct ElfPhdr32_s *)curPhdrAddress;

        if (!quiet)
        {
            puts("p_offset = ");
            print_hex(phdr->p_offset);

            puts(", p_vaddr = ");
            print_hex(phdr->p_vaddr);

            puts(", p_paddr = ");
            print_hex(phdr->p_paddr);

            puts(", p_filesz = ");
            print_hex(phdr->p_filesz);

            puts(", p_memsz = ");
            print_hex(phdr->p_memsz);

            puts("\n");
        }

        {
            uint64_t clearSize = (phdr->p_memsz - phdr->p_filesz);

            for (uint64_t i = 0; i < clearSize; i += 8)
                SPU_LS_Write64(spu_id, ((phdr->p_vaddr + phdr->p_filesz) + i), 0);
        }

        for (uint64_t i = 0; i < phdr->p_filesz; i += 8)
        {
            uint64_t v = *((uint64_t *)(elfFileAddress + phdr->p_offset + i));
            SPU_LS_Write64(spu_id, (phdr->p_vaddr + i), v);
        }

        curPhdrAddress += elfHdr->e_phentsize;
    }

    // SPU_NPC[0:29] = entry (LS)
    SPU_Write_SPU_NPC(spu_id, elfHdr->e_entry);

    eieio();

    if (!quiet)
        puts("LoadElfSpu() done.\n");
}

FUNC_DEF void HW_Init_SPU()
{
    puts("HW_Init_SPU()\n");

    {
        uint64_t mfc_sr1_value = 0x21;

        {
            uint32_t spu_avail = read_spu_avail();

            for (uint32_t i = 0; i < 8; ++i)
            {
                uint32_t mask = (1U << i);

                if ((spu_avail & mask) != 0)
                {
                    uint32_t spu_id = (7 - i);

                    puts("Enabling spu ");
                    print_decimal(spu_id);
                    puts("...\n");

                    SPU_P1_Write64(spu_id, 0x0, mfc_sr1_value);
                }
            }
        }
    }

    {
        uint64_t eib_cfg_or_value = 0x0018000000000000;

        volatile uint64_t* p = (volatile uint64_t*)0x20000511870;
        *p |= eib_cfg_or_value;

        eieio();
    }

    puts("HW_Init_SPU() done.\n");
}

struct SPU_MFCContextEntry_s
{
    uint64_t ea;
    uint64_t ls;
    uint64_t size;
    uint64_t command;
    uint64_t qw;
    uint64_t realSize;
};

FUNC_DEF void SPU_ReadMFCContextEntry(uint64_t spu_id, uint32_t entryIdx, struct SPU_MFCContextEntry_s* outEntry)
{
    if (outEntry == NULL)
        dead_beep();

    if (entryIdx >= 16)
        dead_beep();

    uint64_t value0, value1, value2;
    //uint64_t value3;
    uint64_t spuAddress = (0x2000 + (0x20*entryIdx)); // P2

    value0 = SPU_P2_Read64(spu_id, spuAddress + 0x00);
    value1 = SPU_P2_Read64(spu_id, spuAddress + 0x08);
    value2 = SPU_P2_Read64(spu_id, spuAddress + 0x10);
    //value3 = SPU_P2_Read64(spu_id, spuAddress + 0x18);

    outEntry->ea = ((value1 >> (63 - 51)) << 12) | ((value2 >> (63 - 36)) & 0xFFF);
    outEntry->ls = ((value2 >> (63 - 13)) << 4) | (outEntry->ea & 0xF);
    outEntry->size = (value2 >> (63 - 24)) & 0x7FF;
    outEntry->command = (value0 >> (63 - 34)) & 0xFF;
    outEntry->qw = (value2 >> (63 - 38)) & 0x1;
    outEntry->realSize = (outEntry->qw)?16*outEntry->size:outEntry->size;
}

#if 0

// treat EA as 42 bits?
FUNC_DEF void SPU_ModifyMFCContextEntry_EA(uint64_t spu_id, uint32_t entryIdx, uint64_t ea)
{
    uint64_t spuAddress = (0x2000 + (0x20 * entryIdx)); // P2

    // value1 = high 30 bits
    uint64_t value1Mask = 0x3FFFFFFFULL;
    uint64_t value1MaskShiftCount = 12;

    // value2 = low 12 bits
    uint64_t value2Mask = 0xFFFULL;
    uint64_t value2MaskShiftCount = 27;

    uint64_t value1 = SPU_P2_Read64(spu_id, spuAddress + 0x08);
    uint64_t value2 = SPU_P2_Read64(spu_id, spuAddress + 0x10);

    value1 &= ~(value1Mask << value1MaskShiftCount);
    value2 &= ~(value2Mask << value2MaskShiftCount);

    value1 |= ((ea >> value1MaskShiftCount) << value1MaskShiftCount);
    value2 |= ((ea & value2Mask) << value2MaskShiftCount);

    SPU_P2_Write64(spu_id, (spuAddress + 0x08), value1);
    SPU_P2_Write64(spu_id, (spuAddress + 0x10), value2);
}

// LS is 18 bits long
// only high 14 bits stored in value2, low 4 bits shared with ea
// low 4 bits will be ignored
FUNC_DEF void SPU_ModifyMFCContextEntry_LS(uint64_t spu_id, uint32_t entryIdx, uint32_t ls)
{
    if (ls >= 0x40000)
        dead_beep();

    uint64_t spuAddress = (0x2000 + (0x20 * entryIdx)); // P2

    uint64_t value2 = SPU_P2_Read64(spu_id, spuAddress + 0x10);

    // value2 = high 14 bits
    uint64_t value2Mask = 0x3FFFULL;
    uint64_t value2MaskShiftCount = 50;

    value2 &= ~(value2Mask << value2MaskShiftCount);
    value2 |= ((((uint64_t)ls) >> 4) << value2MaskShiftCount);

    SPU_P2_Write64(spu_id, (spuAddress + 0x10), value2);
}

FUNC_DEF uint8_t SPU_FindMFCContextEntryByEA(uint64_t spu_id, uint64_t ea, uint32_t* outEntryIdx)
{
    if (outEntryIdx == NULL)
        dead_beep();

    struct SPU_MFCContextEntry_s entry;

    for (uint32_t i = 0; i < 16; ++i)
    {
        SPU_ReadMFCContextEntry(spu_id, i, &entry);

        if (entry.ea == ea)
        {
            *outEntryIdx = i;
            return 1;
        }
    }

    return 0;
}

FUNC_DEF uint8_t SPU_FindMFCContextEntryByLS(uint64_t spu_id, uint64_t ls, uint32_t* outEntryIdx)
{
    if (outEntryIdx == NULL)
        dead_beep();

    struct SPU_MFCContextEntry_s entry;

    for (uint32_t i = 0; i < 16; ++i)
    {
        SPU_ReadMFCContextEntry(spu_id, i, &entry);

        if (entry.ls == ls)
        {
            *outEntryIdx = i;
            return 1;
        }
    }

    return 0;
}

#endif

#if 0

FUNC_DEF void SPU_PrintMFCContextEntry(uint64_t spu_id, uint32_t entryIdx)
{
    struct SPU_MFCContextEntry_s entry;
    SPU_ReadMFCContextEntry(spu_id, entryIdx, &entry);

    puts("entryIdx = ");
    print_decimal(entryIdx);
    puts(", ls = ");
    print_hex(entry.ls);
    puts(", ea = ");
    print_hex(entry.ea);
    puts(", command = ");
    print_hex(entry.command);
    puts(", realSize = ");
    print_hex(entry.realSize);
    puts("\n");
}

FUNC_DEF void SPU_PrintAllMFCContextEntry(uint64_t spu_id)
{
    for (uint32_t i = 0; i < 16; ++i)
        SPU_PrintMFCContextEntry(spu_id, i);
}

#endif