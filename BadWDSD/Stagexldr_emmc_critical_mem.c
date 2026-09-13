FUNC_DEF void memset(void *buf, uint8_t v, uint64_t count)
{
    uint8_t *buff = (uint8_t *)buf;

    for (uint64_t i = 0; i < count; ++i)
        buff[i] = v;
}

FUNC_DEF void memcpy(void *dest, const void *src, uint64_t count)
{
    uint8_t *destt = (uint8_t *)dest;
    const uint8_t *srcc = (const uint8_t *)src;

    for (uint64_t i = 0; i < count; ++i)
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