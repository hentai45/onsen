SECTIONS
{
    . = 0x100000;

    .text : {
        bin/boot.o (.text)
        * (.text)
    }

    .data : {
        * (.data)
        * (.rodata*)
        * (.bss)
    }

    .eh_frame : { * (.eh_frame) }

}

