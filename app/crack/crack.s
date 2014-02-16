.globl OnSenMain

.text

OnSenMain:
#    movw $1004*8, %ax
#    movw %ax, %ds
#    movl %ds:(0), %ecx
    movw $1132*8, %ax
    movw %ax, %ds
loop:
#    subl $1, %ecx
#    movb $65, %ds:(%ecx)
    movb $65, (0x0400)
#    cmpl $0, %ecx
#    jne  loop

fin:
    jmp fin
