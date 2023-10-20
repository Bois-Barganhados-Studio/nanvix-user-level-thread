.globl start_routine

.type start_routine,@function

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

    
    