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

	.data : {*(.data)}

	. = ALIGN(4);

	__platform_start = .;
	.platform_info : {
		*(.platform_info)
	}
	__platform_end = .;

	__init_call_start = .;
	._init : {
		*(.arch_init)
		*(.platform_init)
	}
	__init_call_end = .;

	. = ALIGN(4);

	__syscall_table_start = .;
	.syscall_table : {
		*(.__syscall)
	}
	__syscall_table_end = .;

	. = ALIGN(4);

	_bss_start = .;
	.bss : {*(.bss)}
	_bss_end = .;
}
