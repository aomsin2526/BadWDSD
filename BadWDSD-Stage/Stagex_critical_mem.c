FUNC_DEF void memset(void *buf, uint8_t v, uint64_t count)
{
    if ((((uint64_t)buf % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t vv = 0;

        if (v != 0)
        {
            uint64_t v64 = v;

            vv |= (v64 << 56);
            vv |= (v64 << 48);
            vv |= (v64 << 40);
            vv |= (v64 << 32);
            vv |= (v64 << 24);
            vv |= (v64 << 16);
            vv |= (v64 << 8);
            vv |= v64;
        }

        uint64_t *buff = (uint64_t *)buf;

        for (uint64_t i = 0; i < (count / 8); ++i)
            buff[i] = vv;
    }
    else if ((((uint64_t)buf % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t vv = 0;

        if (v != 0)
        {
            uint32_t v32 = v;

            vv |= (v32 << 24);
            vv |= (v32 << 16);
            vv |= (v32 << 8);
            vv |= v32;
        }

        uint32_t *buff = (uint32_t *)buf;

        for (uint64_t i = 0; i < (count / 4); ++i)
            buff[i] = vv;
    }
    else if ((((uint64_t)buf % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t vv = 0;

        if (v != 0)
        {
            uint16_t v16 = v;

            vv |= (v16 << 8);
            vv |= v16;
        }

        uint16_t *buff = (uint16_t *)buf;

        for (uint64_t i = 0; i < (count / 2); ++i)
            buff[i] = vv;
    }
    else
    {
        uint8_t *buff = (uint8_t *)buf;

        for (uint64_t i = 0; i < count; ++i)
            buff[i] = v;
    }
}

FUNC_DEF void memcpy(void *dest, const void *src, uint64_t count)
{
    if ((((uint64_t)dest % 8) == 0) && (((uint64_t)src % 8) == 0) && ((count % 8) == 0))
    {
        uint64_t *destt = (uint64_t *)dest;
        const uint64_t *srcc = (const uint64_t *)src;

        for (uint64_t i = 0; i < (count / 8); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint64_t)dest % 4) == 0) && (((uint64_t)src % 4) == 0) && ((count % 4) == 0))
    {
        uint32_t *destt = (uint32_t *)dest;
        const uint32_t *srcc = (const uint32_t *)src;

        for (uint64_t i = 0; i < (count / 4); ++i)
            destt[i] = srcc[i];
    }
    else if ((((uint64_t)dest % 2) == 0) && (((uint64_t)src % 2) == 0) && ((count % 2) == 0))
    {
        uint16_t *destt = (uint16_t *)dest;
        const uint16_t *srcc = (const uint16_t *)src;

        for (uint64_t i = 0; i < (count / 2); ++i)
            destt[i] = srcc[i];
    }
    else
    {
        uint8_t *destt = (uint8_t *)dest;
        const uint8_t *srcc = (const uint8_t *)src;

        for (uint64_t i = 0; i < count; ++i)
            destt[i] = srcc[i];
    }
}

FUNC_DEF void memcpy8(void *dest, const void *src, uint64_t count)
{
    uint8_t *destt = (uint8_t *)dest;
    const uint8_t *srcc = (const uint8_t *)src;

    for (uint64_t i = 0; i < count; ++i)
        destt[i] = srcc[i];
}

FUNC_DEF void memcpy32(void *dest, const void *src, uint64_t count)
{
    uint32_t *destt = (uint32_t *)dest;
    const uint32_t *srcc = (const uint32_t *)src;

    for (uint64_t i = 0; i < (count / 4); ++i)
        destt[i] = srcc[i];
}

FUNC_DEF uint8_t memcmp(const void *p1, const void *p2, uint64_t count)
{
    const uint8_t *pp1 = (const uint8_t *)p1;
    const uint8_t *pp2 = (const uint8_t *)p2;

    for (uint64_t i = 0; i < count; ++i)
    {
        if (pp1[i] != pp2[i])
            return 1;
    }

    return 0;
}

FUNC_DEF uint8_t memcmp32(const void *p1, const void *p2, uint64_t count)
{
    const uint32_t *pp1 = (const uint32_t *)p1;
    const uint32_t *pp2 = (const uint32_t *)p2;

    for (uint64_t i = 0; i < (count / 4); ++i)
    {
        if (pp1[i] != pp2[i])
            return 1;
    }

    return 0;
}

FUNC_DEF uint64_t strlen(const char *str)
{
    uint64_t len = 0;

    while (1)
    {
        if (str[len] == 0)
            break;

        ++len;
    }

    return len;
}

FUNC_DEF uint8_t strcmp(const char *str1, const char *str2)
{
    uint64_t str1_len = strlen(str1);
    uint64_t str2_len = strlen(str2);

    if (str1_len != str2_len)
        return 1;

    for (uint64_t i = 0; i < str2_len; ++i)
    {
        if (str1[i] != str2[i])
            return 1;
    }

    return 0;
}

FUNC_DEF uint8_t SearchAndReplace(void *in_data, uint64_t dataSize, const void *in_searchData, uint64_t searchDataSize, const void *in_replaceData, uint64_t replaceDataSize)
{
    uint8_t *data = (uint8_t *)in_data;

    const uint8_t *searchData = (const uint8_t *)in_searchData;
    const uint8_t *replaceData = (const uint8_t *)in_replaceData;

    for (uint64_t i = 0; i < dataSize; ++i)
    {
        if (!memcmp(&data[i], searchData, searchDataSize))
        {
            memcpy(&data[i], replaceData, replaceDataSize);
            return 1;
        }
    }

    return 0;
}

FUNC_DEF uint8_t SearchMemory(const void *in_data, uint64_t dataSize, const void *in_searchData, uint64_t searchDataSize, uint64_t *outFoundAddr)
{
    const uint8_t *data = (const uint8_t *)in_data;

    const uint8_t *searchData = (const uint8_t *)in_searchData;

    for (uint64_t i = 0; i < dataSize; ++i)
    {
        if (!memcmp(&data[i], searchData, searchDataSize))
        {
            if (outFoundAddr != NULL)
                *outFoundAddr = (uint64_t)&data[i];

            // puts("foundAddr: ");
            // print_hex((uint64_t)&data[i]);
            // puts("\n");

            return 1;
        }
    }

    return 0;
}