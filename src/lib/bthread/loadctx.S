.globl loadctx

.type loadctx,@function

loadctx:
    movl 4(%esp), %eax
    addl $8, %esp
    movl 0(%eax), %edi  
    movl 4(%eax), %esi   
    movl 8(%eax), %ebp   
    movl 12(%eax), %ebx   
    movl 16(%eax), %edx   
    movl 32(%eax), %ecx
    movl 8(%eax), %esp
    pushl %ecx
    movl 28(%eax), %ecx
    pushl %ecx
    movl 24(%eax), %ecx
    pushl %ecx
    movl 20(%eax), %ecx
    popl %eax
    popf
    ret
    