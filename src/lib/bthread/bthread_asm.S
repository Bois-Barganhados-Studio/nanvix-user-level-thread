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


.globl savectx
.globl loadctx
.globl start_routine
.globl bt_do_restore

.type start_routine,@function
.type savectx,@function
.type loadctx,@function 
.type bt_do_restore,@function

bt_do_restore:
    addl $4, %esp           /* clear saved eip */
    popl %eax
    popl %edx
    addl $4, %esp           /* ? */
    popl %ebp               /* undo caller new frame */
    addl $4, %esp           /* clear caller ret eip */
    
    /* save thread context */
    popl 0(%eax)
    popl 4(%eax)
    popl 8(%eax)
    popl 12(%eax)
    popl 16(%eax)
    popl 20(%eax)
    popl 24(%eax)
    popl 28(%eax)
    popl 32(%eax)
    leal 0(%esp), %ecx
    movl %ecx, 36(%eax)
    
    /* back to scheduler */
    pushl $1
    pushl %edx
    call longjmp
    /* no return */

savectx:
    pushl %eax 
    pushl %edx 
    movl 12(%esp), %eax
    movl 16(%esp), %edx
    movl %edi, BTHREAD_CTXBUF_EDI(%eax)
    movl %esi, BTHREAD_CTXBUF_ESI(%eax)
    movl %ebp, BTHREAD_CTXBUF_EBP(%eax)
    movl %ebx, BTHREAD_CTXBUF_EBX(%eax)
    popl BTHREAD_CTXBUF_EDX(%eax)
    movl %ecx, BTHREAD_CTXBUF_ECX(%eax)
    popl BTHREAD_CTXBUF_EAX(%eax)
    pushf
    popl BTHREAD_CTXBUF_EFLAGS(%eax)
    movl 0(%esp), %ecx
    movl %ecx, BTHREAD_CTXBUF_EIP(%eax)
    movl %esp, BTHREAD_CTXBUF_ESP(%eax)
    push $1
    push %edx
    push $0
    call alarm
    add $4, %esp
    call longjmp
    /* no return */

loadctx:
    movl 4(%esp), %eax
    addl $8, %esp
    movl 0(%eax), %edi  
    movl 4(%eax), %esi   
    movl 8(%eax), %ebp   
    movl 12(%eax), %ebx   
    movl 16(%eax), %edx   
    movl 32(%eax), %ecx
    movl 36(%eax), %esp
    pushl %ecx
    movl 28(%eax), %ecx
    pushl %ecx
    movl 24(%eax), %ecx
    pushl %ecx
    movl 20(%eax), %ecx
    popl %eax
    popf
    ret

start_routine:
    movl 16(%esp), %eax         /* tcb */
    movl 8(%esp), %edx          /* sched_ctx */

    /* saving sched sp */
    lea 0(%esp), %edx

    /* building stack for the thread */
    movl 52(%eax), %ecx
    lea 1020(%ecx), %esp
    lea 1020(%ecx), %ebp

    /* saving vars to call thread routine */
    push %eax
    push 12(%edx)
    push 8(%edx)
    
    call *4(%edx)
    movl 8(%esp), %eax

    /* calling thread routine */
    push 60(%eax)
    call *56(%eax)     

    /* saving routine return, then calling `turnoff_alarm` */
    movl %eax, 0(%esp)
    movl 8(%esp), %ecx
    call *%ecx

    /* restoring vars before routine call, then back to scheduler */
    movl 12(%esp), %eax
    pop 64(%eax)
    movl $2, 4(%esp) 
    call longjmp
