ENTRY(_start)

SECTIONS
{
    . = 0xffffffff80182000;
    __start = .;
    
    __text_start = .;
    .text : {
        *(.text)
    }
    __text_end = .;

    .reginfo : {
        *(.reginfo)
    }
    
    __rodata_start = .;
    .rodata :
    {
        *(.rodata)
    }
    __rodata_end = .;
    
    __data_start = .;
    .data :
    {
        *(.data)
    }
    __data_end = .;

    __bss_start = .;
    .bss  : {
        *(.bss)
    }
    __bss_end = .;
    
    __end = .;
}
