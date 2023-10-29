/*
 * Copyright(C) 2023 Vinicius G. Santos <viniciusgsantos@protonmail.com>
 *
 * This file is part of Nanvix.
 *
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

/* Must come first. */
#define _ASM_FILE_

#include <bthread.h>

.globl loadctx
.globl savectx
.globl set_stack

.type loadctx,@function 
.type set_stack,@function
.type savectx,@function


/*----------------------------------------------------------------------------*
 *                                  savectx()                                 *
 *----------------------------------------------------------------------------*/

 /*
 * Saves thread context, either from yield() or from stack after alarm().
 */
savectx:
    movl 0(%esp), %ecx      /* save return eip */
    movl 4(%esp), %eax      /* context buffer */
    movl 8(%esp), %edx      /* mode */
    test %edx, %edx
    jnz savectx.from_yield

savectx.from_stack:
    /* reset ebp and esp */
    movl 28(%esp), %ebp
    addl $36, %esp

    /* saving thread context */
    popl BTHREAD_CTXBUF_EDI(%eax)
    popl BTHREAD_CTXBUF_ESI(%eax)
    popl BTHREAD_CTXBUF_EBP(%eax)
    popl BTHREAD_CTXBUF_EBX(%eax)
    popl BTHREAD_CTXBUF_EDX(%eax)
    popl BTHREAD_CTXBUF_ECX(%eax)
    popl BTHREAD_CTXBUF_EAX(%eax)
    popl BTHREAD_CTXBUF_EFLAGS(%eax)
    popl BTHREAD_CTXBUF_EIP(%eax)
    movl %esp, BTHREAD_CTXBUF_ESP(%eax)
    
    /* return to caller */
    push %ecx
    ret

savectx.from_yield:
    /* saving callee saved registers */
    movl %edi, BTHREAD_CTXBUF_EDI(%eax)
    movl %esi, BTHREAD_CTXBUF_ESI(%eax)
    movl 28(%esp), %ebp
    movl %ebp, BTHREAD_CTXBUF_EBP(%eax)
    movl %ebx, BTHREAD_CTXBUF_EBX(%eax)
    pushf
    popl BTHREAD_CTXBUF_EFLAGS(%eax)
    movl 32(%esp), %ecx
    movl %ecx, BTHREAD_CTXBUF_EIP(%eax)
    lea 36(%esp), %edx
    movl %edx, BTHREAD_CTXBUF_ESP(%eax)

    /* return to yield */
    ret

/*----------------------------------------------------------------------------*
 *                                 loadctx()                                  *
 *----------------------------------------------------------------------------*/

 /*
 * Loads thread context from buffer.
 */
loadctx:
    movl 4(%esp), %eax
    addl $8, %esp
    movl BTHREAD_CTXBUF_EDI(%eax), %edi  
    movl BTHREAD_CTXBUF_ESI(%eax), %esi   
    movl BTHREAD_CTXBUF_EBP(%eax), %ebp   
    movl BTHREAD_CTXBUF_EBX(%eax), %ebx   
    movl BTHREAD_CTXBUF_EDX(%eax), %edx   
    movl BTHREAD_CTXBUF_EIP(%eax), %ecx
    movl BTHREAD_CTXBUF_ESP(%eax), %esp
    pushl %ecx
    movl BTHREAD_CTXBUF_EFLAGS(%eax), %ecx
    pushl %ecx
    movl BTHREAD_CTXBUF_EAX(%eax), %ecx
    pushl %ecx
    movl BTHREAD_CTXBUF_ECX(%eax), %ecx
    popl %eax
    popf
    ret

/*----------------------------------------------------------------------------*
 *                                 set_stack()                                *
 *----------------------------------------------------------------------------*/

 /*
 * Sets a new stack to a non returning routine, then calls it.
 */
set_stack:
    movl 4(%esp), %eax      /* new stack */
    movl 8(%esp), %edx      /* non returning routine */

    /* set stack for the routine */
    lea BTHREAD_STACK_SIZE - 4(%eax), %esp
    lea BTHREAD_STACK_SIZE - 4(%eax), %ebp

    /* calling the non returning routine */
    jmp *%edx


