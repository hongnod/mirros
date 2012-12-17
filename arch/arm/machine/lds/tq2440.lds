ENTRY(_start)
SECTIONS
{
	. = 0x80006000;
	.text : 
	{
		*(.text) 
		*(.rodata)
	}


	. = ALIGN(4);

	__platform_start = .;
	.platform_info : {
		*(.platform_info)
	}
	__platform_end = .;

	.data : {*(.data)}

	__init_call_start = .;
	.arch_init : {
		*(.arch_init)
	}
	.platform_init : {
		*(.platform_init)
	}
	__init_call_end = .;

	_bss_start = .;
	.bss : {*(.bss)}
	_bss_end = .;


        /* Stabs debugging sections.    */
        .stab 0 : { *(.stab) }
        .stabstr 0 : { *(.stabstr) }
        .stab.excl 0 : { *(.stab.excl) }
        .stab.exclstr 0 : { *(.stab.exclstr) }
        .stab.index 0 : { *(.stab.index) }
        .stab.indexstr 0 : { *(.stab.indexstr) }
        .comment 0 : { *(.comment) }
        .debug_abbrev 0 : { *(.debug_abbrev) }
        .debug_info 0 : { *(.debug_info) }
        .debug_line 0 : { *(.debug_line) }
        .debug_pubnames 0 : { *(.debug_pubnames) }
        .debug_aranges 0 : { *(.debug_aranges) }
}
