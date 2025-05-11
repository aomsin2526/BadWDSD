.global main

main:
    ilhu 2, 0x1337
    iohl 2, 0xBAAD
    wrch 28, 2
loop:
    br loop