#    .globl reload_segments

areload_segments:
    ljmpl   $0x10, $reloadcs
reloadcs:
    movw   $0x08, %ax
    movw   %ax, %ds
    movw   %ax, %es
    movw   %ax, %fs
    movw   %ax, %gs
    movw   %ax, %ss
    ret

