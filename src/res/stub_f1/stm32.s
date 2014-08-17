.global  _start
.section .text
.thumb

_start:
	ldr r0, _flashbase
	ldr r1, _addr
	mov r2, pc
	add r2, #(_data - . - 2)
	ldr r3, _size
	mov r5, #1
_next:
	cmp r3, #0
	beq _done
	@ Write PG command to FLASH_CR
	str r5, [r0, #0x10]
	@ Write data to flash (half-word)
	ldrh r4, [r2]
	strh r4, [r1]

_wait:	@ Wait for BSY bit to clear
	ldr r4, [r0, #0x0C]
	mov r6, #1
	tst r4, r6
	bne _wait

	sub r3, #2
	add r1, #2
	add r2, #2
	b _next
_done:
	bkpt

.align 2
_flashbase:
	.word 0x40022000
/* tohle se natahuje az nasledne */
_addr:
	.word 0x08000000
_size:
	.word 0x400
/* skutecna delka je posledni slovo */
_data:
	.word _addr - _start

