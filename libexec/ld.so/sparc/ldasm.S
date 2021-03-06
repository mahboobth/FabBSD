/*	$OpenBSD: ldasm.S,v 1.15 2006/05/03 16:10:52 drahn Exp $	*/

/*
 * Copyright (c) 2001 Jason L. Wright (jason@thought.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2000 Eduardo Horvath.
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas and Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/syscall.h>
#include <machine/trap.h>
#include <machine/asm.h>

#define DL_DATA_SIZE	(16*4)
#define ARGC		(16*4)

	.section	".text"
	.align	4
	.global	_dl_start
	.type	_dl_start,@function
_dl_start:
	sub	%g0, %g0, %fp		! clear frame
	mov	%g1, %l1		! save ps_strings
	sub	%sp, DL_DATA_SIZE, %sp	! make room for dl_data
	add	%sp, ARGC, %l3

	add	%l3, DL_DATA_SIZE, %o0
	mov	%o0, %l0

	/*
	 * need to figure out where _DYNAMIC is located newer binutils
	 * does not fill in GOT to read _DYNAMIC before relocation.
	 */
	call	0f
	 nop
	call	_DYNAMIC+8		! not executed (no delay needed)
0:	ld	[%o7+8], %o2		! load stub call instruction
	sll	%o2, 2, %o2		! extract PC offset
	sra	%o2, 0, %o2		! sign-extend

	add	%o2, %o7, %o2		! real &_DYNAMIC

	call	_dl_boot_bind		! _dl_boot_bind(sp,dl_data,dynamicp)
	 mov	%l3, %o1

	mov	%l3, %o3
	ld	[%l0], %l3		! argc = *sp
	sll	%l3, 2, %l3		! argc *= sizeof(long)
	add	%l0, 4, %o0		! argv = [sp + argc]
	add	%l0, 8, %o1		! envp = sp + 8 +
	add	%o1, %l3, %o1		!	+ argc

	add	%o3, (7*4), %l2
	ld	[%l2], %o2		! loff = dl_data[AUX_base];

	call	_dl_boot		! _dl_boot(argv,envp,loff,dl_data)
	 nop

	add	%sp, DL_DATA_SIZE, %sp	! restore stack

	jmp	%o0
	 mov	%l1, %g1		! restore ps_strings


	.section	".text"
	.align	4
	.global	_dl_bind_start
	.type	_dl_bind_start,@function
_dl_bind_start:	# (obj, reloff)
	save	%sp, -96, %sp		/* setup standard stack frame */
	ld	[%i7 + 8], %o0		/* obj id is in second PLT slot */
	srl	%g1, 10, %o1		/* offset is in high 22 bits */
	call	_dl_bind		/* Call _rtld_bind(obj, offset) */
	 sub	%o1, 12*4, %o1		/* first 4 `pltrel' entries missing! */

	mov	%o0, %g1		/* return value == function address */
	restore				/* get rid of our context */
	 jmp	%g1			/* and the jmpslot context, then go. */
	restore

	.section ".text"
	.align 4
	.global _dl_close
	.type _dl_close,@function
_dl_close:
	mov SYS_close | SYSCALL_G2RFLAG, %g1	! call sys_close
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_exit
	.type _dl_exit,@function
_dl_exit:
	mov SYS_exit | SYSCALL_G2RFLAG, %g1	! call sys_exit
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_issetugid
	.type _dl_issetugid,@function
_dl_issetugid:
	mov SYS_issetugid | SYSCALL_G2RFLAG, %g1	! call sys_issetugid
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl__syscall
	.type _dl__syscall,@function
_dl__syscall:
	mov SYS___syscall | SYSCALL_G2RFLAG, %g1	! call sys_syscall
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_munmap
	.type _dl_munmap,@function
_dl_munmap:
	mov SYS_munmap | SYSCALL_G2RFLAG, %g1	! calling sys_munmap
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_mprotect
	.type _dl_mprotect,@function
_dl_mprotect:
	mov SYS_mprotect | SYSCALL_G2RFLAG, %g1	! calling sys_mprotect
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_open
	.type _dl_open,@function
_dl_open:
	mov SYS_open | SYSCALL_G2RFLAG, %g1	! calling sys_open
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_read
	.type _dl_read,@function
_dl_read:
	mov SYS_read | SYSCALL_G2RFLAG, %g1	! calling sys_read
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_write
	.type _dl_write,@function
_dl_write:
	mov SYS_write | SYSCALL_G2RFLAG, %g1	! calling sys_write
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.global _dl_stat
	.type _dl_stat,@function
_dl_stat:
	mov SYS_stat | SYSCALL_G2RFLAG, %g1	! call sys_stat
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.globl _dl_fstat
	.type _dl_fstat,@function
_dl_fstat:
	mov  SYS_fstat | SYSCALL_G2RFLAG, %g1	! call sys_fstat
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.globl _dl_fcntl
	.type _dl_fcntl,@function
_dl_fcntl:
	mov  SYS_fcntl | SYSCALL_G2RFLAG, %g1	! call sys_fcntl
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno


	.section ".text"
	.align 4
	.globl _dl_getdirentries
	.type _dl_getdirentries,@function
_dl_getdirentries:
	mov  SYS_getdirentries | SYSCALL_G2RFLAG, %g1	! call sys_getdirentries
	add %o7, 8, %g2					! just return on success
	t ST_SYSCALL					! off to wonderland
	retl
	 sub %g0, %o0, %o0				! error: result = -errno


	.section ".text"
	.align 4
	.globl _dl_sysctl
	.type _dl_sysctl,@function
_dl_sysctl:
	mov  SYS___sysctl | SYSCALL_G2RFLAG, %g1	! call sys___sysctl
	add %o7, 8, %g2					! just return on success
	t ST_SYSCALL					! off to wonderland
	retl
	 sub %g0, %o0, %o0				! error: result = -errno


	/* _dl_sigprocmask does not support NULL new mask */
	.section ".text"
	.align 4
	.globl _dl_sigprocmask
	.type _dl_sigprocmask,@function
_dl_sigprocmask:
	ld	[%o1], %o1
	mov	SYS_sigprocmask, %g1			! call sys___sigprocmask
	t	ST_SYSCALL				! off to wonderland

	cmp	%o2, 0
	bne,a	1f
	 st	%o0, [%o2]
1:
	retl
	 clr	%o0


	.section ".text"
	.align 4
	.global _dl_gettimeofday
	.type _dl_gettimeofday,@function
_dl_gettimeofday:
	mov SYS_gettimeofday | SYSCALL_G2RFLAG, %g1	! calling sys_gettimeofday
	add %o7, 8, %g2				! just return on success
	t ST_SYSCALL				! off to wonderland
	retl
	 sub %g0, %o0, %o0			! error: result = -errno

/*
 * V8 sparc .{,u}{mul,div,rem} replacements.
 * We try to mimic them 100%.  Full 64 bit sources or outputs, and
 * these routines are required to update the condition codes.
 */
.globl _C_LABEL(_mulreplace), _C_LABEL(_mulreplace_end)
_C_LABEL(_mulreplace):
	smulcc	%o0, %o1, %o0
	retl
	 rd	%y, %o1
_C_LABEL(_mulreplace_end):

.globl _C_LABEL(_umulreplace), _C_LABEL(_umulreplace_end)
_C_LABEL(_umulreplace):
	umulcc	%o0, %o1, %o0
	retl
	 rd	%y, %o1
_C_LABEL(_umulreplace_end):

.globl _C_LABEL(_divreplace), _C_LABEL(_divreplace_end)
_C_LABEL(_divreplace):
	sra	%o0, 31, %g1
	wr	%g1, 0, %y
	nop
	nop
	nop
	retl
	 sdivcc	%o0, %o1, %o0
_C_LABEL(_divreplace_end):

.globl _C_LABEL(_udivreplace), _C_LABEL(_udivreplace_end)
_C_LABEL(_udivreplace):
	wr	%g0, 0, %y
	nop
	nop
	nop
	retl
	 udivcc	%o0, %o1, %o0
_C_LABEL(_udivreplace_end):

.globl _C_LABEL(_remreplace), _C_LABEL(_remreplace_end)
_C_LABEL(_remreplace):
	sra	%o0, 31, %g1
	wr	%g1, 0, %y
	nop
	nop
	nop
	sdiv	%o0, %o1, %o2
	smul	%o1, %o2, %o2
	retl
	 subcc	%o0, %o2, %o0
_C_LABEL(_remreplace_end):

.globl _C_LABEL(_uremreplace), _C_LABEL(_uremreplace_end)
_C_LABEL(_uremreplace):
	wr	%g0, 0, %y
	nop
	nop
	nop
	udiv	%o0, %o1, %o2
	umul	%o1, %o2, %o2
	retl
	 subcc	%o0, %o2, %o0
_C_LABEL(_uremreplace_end):
