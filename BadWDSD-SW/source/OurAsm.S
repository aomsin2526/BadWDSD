.global our_lv1_peek_do

// in: r3 = addr
// out: r3 = value

our_lv1_peek_do:
	ld %r3, 0(%r3)
	blr

.global our_lv1_poke_do

// in: r3 = addr, r4 = value
// out: r3 = 0

our_lv1_poke_do:
	std %r4, 0(%r3)
	li %r3, 0
	blr

// in: r3-r8 = args, r9 = addr

.global our_lv1_exec_do

our_lv1_exec_do:

	// push
	addi %r1, %r1, -16

	mflr %r0
	std %r0, 0(%r1)

	// push
	addi %r1, %r1, -32

	mtctr %r9
	bctrl

	// pop
	addi %r1, %r1, 32

	ld %r0, 0(%r1)
	mtlr %r0

	// pop
	addi %r1, %r1, 16

	blr

.global our_lv1_exec_do_size

our_lv1_exec_do_size:
	.quad (our_lv1_exec_do_size - our_lv1_exec_do)

// test
// in: r9 = start addr of this function

.global our_lv1_exec_test_do

our_lv1_exec_test_do:
	li %r3, 0x39
	blr

.global our_lv1_exec_test_do_size

our_lv1_exec_test_do_size:
	.quad (our_lv1_exec_test_do_size - our_lv1_exec_test_do)

// test puts
// in: r9 = start addr of this function

// puts = 0x2BA460

.global our_lv1_test_puts_do

our_lv1_test_puts_do:

	// push
	addi %r1, %r1, -512

	mflr %r0
	std %r0, 0(%r1)

	ld %r0, (our_lv1_test_puts_do_puts_addr - our_lv1_test_puts_do)(%r9)
	mtctr %r0

	mr %r3, %r9
	addi %r3, %r3, (our_lv1_test_puts_do_test_str - our_lv1_test_puts_do)

	bctrl

	ld %r0, 0(%r1)
	mtlr %r0

	// pop
	addi %r1, %r1, 512

	blr

our_lv1_test_puts_do_test_str:
	.asciz "Hello from lv1_puts\n"

.align 4

our_lv1_test_puts_do_puts_addr:
	.quad 0x2BA460

.global our_lv1_test_puts_do_size

our_lv1_test_puts_do_size:
	.quad (our_lv1_test_puts_do_size - our_lv1_test_puts_do)

// in: r3 = laid, r4 = dest, r5 = src
// out: r3 = 0 on success

// tmp: r6 = readSize, r7 = curReadSize, r8 = curSrcAddr, r9 = curDestAddr, r10 = tmp

// lv2_kernel code starts from file offset ((file offset to data) + 0x10000)
// src + 0x10 = file offset to data
// src + 0x18 = data size

.global our_lv1_auth_lv2_hook_fself_do

our_lv1_auth_lv2_hook_fself_do:

	// load offsetToData
	ld %r10, 0x10(%r5)
	
	// load readSize
	ld %r6, 0x18(%r5)

	// readSize -= 0x10000
	addi %r6, %r6, -32768
	addi %r6, %r6, -32768

	// readSize &= 0xFFFFFFFFFFFFF000
	// use r8 and r7 as tmp

	// 0xFFF
	//li %r8, 0xFFF

	// 0xFFFFFFFFFFFFFFFF
	//li %r7, 0
	//addi %r7, %r7, -1

	// 0xFFFFFFFFFFFFF000
	//xor %r8, %r8, %r7

	// readSize &= 0xFFFFFFFFFFFFF000
	//and %r6, %r6, %r8

	// curSrcAddr = src
	mr %r8, %r5
	
	// curSrcAddr += offsetToData
	add %r8, %r8, %r10

	// curSrcAddr += 0x10000
	addi %r8, %r8, 32767
	addi %r8, %r8, 32767
	addi %r8, %r8, 2

	// curDestAddr = dest
	mr %r9, %r4

	// curReadSize = 0
	li %r7, 0

loop:
	// load data from curSrcAddr to r10
	lbz %r10, 0(%r8)

	// store data from r10 to curDestAddr
	stb %r10, 0(%r9)

	// curReadSize += 1
	addi %r7, %r7, 1

	// curSrcAddr += 1
	addi %r8, %r8, 1

	// curDestAddr += 1
	addi %r9, %r9, 1

	// end loop if curReadSize == readSize
	cmp 0, 0, %r7, %r6
	beq end

	// otherwise, loop again
	b loop

end:
	li %r3, 0
	blr

.global our_lv1_auth_lv2_hook_fself_do_size

our_lv1_auth_lv2_hook_fself_do_size:
	.quad (our_lv1_auth_lv2_hook_fself_do_size - our_lv1_auth_lv2_hook_fself_do)

// our junk

.global our_junk

our_junk:
	addi %r28, %r3, 0x1000

// in: r3 = core_mul, r4 = mem_mul, r9 = addr of this function
// out: r3 = 0

.global our_lv1_apply_rsx_clock_do

our_lv1_apply_rsx_clock_do:

	eieio
	sync

	// add delay

	ld %r5, (our_lv1_apply_rsx_clock_do_loop_count - our_lv1_apply_rsx_clock_do)(%r9)
	li %r6, 0

looppp1:
	addi %r6, %r6, 1

	cmp 0, 0, %r6, %r5
	beq write_clock

	b looppp1

write_clock:

	// write clock

	eieio
	sync

	// core

	ld %r5, (our_lv1_apply_rsx_clock_do_core_clock_addr - our_lv1_apply_rsx_clock_do)(%r9)
	ld %r6, 0(%r5)

	ld %r5, (our_lv1_apply_rsx_clock_do_clock_mask - our_lv1_apply_rsx_clock_do)(%r9)
	and %r6, %r6, %r5

	sli %r3, %r3, 8
	or %r6, %r6, %r3

	ld %r5, (our_lv1_apply_rsx_clock_do_core_clock_addr - our_lv1_apply_rsx_clock_do)(%r9)
	std %r6, 0(%r5)

	// mem

	ld %r5, (our_lv1_apply_rsx_clock_do_mem_clock_addr - our_lv1_apply_rsx_clock_do)(%r9)
	ld %r6, 0(%r5)

	ld %r5, (our_lv1_apply_rsx_clock_do_clock_mask - our_lv1_apply_rsx_clock_do)(%r9)
	and %r6, %r6, %r5

	sli %r4, %r4, 8
	or %r6, %r6, %r4

	ld %r5, (our_lv1_apply_rsx_clock_do_mem_clock_addr - our_lv1_apply_rsx_clock_do)(%r9)
	std %r6, 0(%r5)

	//

	eieio
	sync

	// add delay

	ld %r5, (our_lv1_apply_rsx_clock_do_loop_count - our_lv1_apply_rsx_clock_do)(%r9)
	li %r6, 0

looppp2:
	addi %r6, %r6, 1

	cmp 0, 0, %r6, %r5
	beq endd

	b looppp2

endd:
	eieio
	sync

    li %r3, 0
    blr

our_lv1_apply_rsx_clock_do_loop_count:
	.quad 200000000

our_lv1_apply_rsx_clock_do_core_clock_addr:
	.quad 0x28000004028

our_lv1_apply_rsx_clock_do_mem_clock_addr:
	.quad 0x28000004010

our_lv1_apply_rsx_clock_do_clock_mask:
	.quad 0xFFFFFFFFFFFF00FF

.global our_lv1_apply_rsx_clock_do_size

our_lv1_apply_rsx_clock_do_size:
	.quad (our_lv1_apply_rsx_clock_do_size - our_lv1_apply_rsx_clock_do)