/* Runtime ABI for the ARM Cortex-M0  
 * lmul.S: 64 bit multiplication
 *
 * Copyright (c) 2012 Jörg Mische <bobbl@gmx.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */



	.syntax unified
	.text
	.thumb
	.cpu cortex-m0



@ long long __muldi3(long long, long long)
@
@ libgcc wrapper: just an alias for __aeabi_lmul()
@
	.thumb_func
        .global __muldi3
__muldi3:



@ long long __aeabi_lmul(long long r1:r0, long long r3:r2)
@
@ Multiply r1:r0 and r3:r2 and return the product in r1:r0
@ Can also be used for unsigned long product
@
	.thumb_func
        .global __aeabi_lmul
__aeabi_lmul:
	
	muls	r1, r2
	muls	r3, r0
	adds	r1, r3
	lsrs	r3, r0, #16
	muls	r3, r2
	lsrs	r3, #16
	adds	r1, r3
	lsrs	r3, r2, #16
	muls	r3, r0
	lsrs	r3, #16
	adds	r1, r3
	muls	r0, r2
	bx	lr
