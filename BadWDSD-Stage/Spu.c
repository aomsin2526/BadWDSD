FUNC_DEF uint64_t SPU_CalcMMIOAddress(uint64_t spu_id, uint64_t offset)
{
    return 0x20000000000 + (0x80000 * spu_id) + offset;
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
    return 0x20000400000 + (0x2000 * spu_id) + offset;
}

FUNC_DEF uint64_t SPU_LS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_LS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset));
}

FUNC_DEF void SPU_LS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_LS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_PS_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_PS_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset));
}

FUNC_DEF void SPU_PS_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_PS(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_P2_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_P2_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset));
}

FUNC_DEF void SPU_P2_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P2(spu_id, offset)) = value;
}

FUNC_DEF uint64_t SPU_P1_Read64(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write64(uint64_t spu_id, uint64_t offset, uint64_t value)
{
    *((volatile uint64_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
}

FUNC_DEF uint32_t SPU_P1_Read32(uint64_t spu_id, uint64_t offset)
{
    return *((volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset));
}

FUNC_DEF void SPU_P1_Write32(uint64_t spu_id, uint64_t offset, uint32_t value)
{
    *((volatile uint32_t *)SPU_CalcMMIOAddress_P1(spu_id, offset)) = value;
}

FUNC_DEF void LoadElfSpu(uint64_t elfFileAddress, uint64_t spu_id)
{
    puts("LoadElfSpu()\n");

    struct ElfHeader32_s *elfHdr = (struct ElfHeader32_s *)elfFileAddress;

    if (*((uint32_t *)elfHdr->e_ident) != 0x7F454C46)
    {
        puts("LoadElf e_ident check failed!\n");
        dead();
    }

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

    uint64_t curPhdrAddress = (elfFileAddress + elfHdr->e_phoff);

    for (uint16_t i = 0; i < elfHdr->e_phnum; ++i)
    {
        struct ElfPhdr32_s *phdr = (struct ElfPhdr32_s *)curPhdrAddress;

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

        for (uint64_t i = 0; i < phdr->p_memsz; i += 8)
            SPU_LS_Write64(spu_id, (phdr->p_vaddr + i), 0);

        for (uint64_t i = 0; i < phdr->p_filesz; i += 8)
        {
            uint64_t v = *((uint64_t *)(elfFileAddress + phdr->p_offset + i));
            SPU_LS_Write64(spu_id, (phdr->p_vaddr + i), v);
        }

        curPhdrAddress += elfHdr->e_phentsize;
    }

    // SPU_NPC[0:29] = entry (LS)
    SPU_PS_Write32(spu_id, 0x04034, elfHdr->e_entry);

    eieio();

    puts("LoadElfSpu() done.\n");
}