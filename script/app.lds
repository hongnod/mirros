ENTRY(main)
SECTIONS
{
	. = 0x00004000;
	.text : 
	{
		*(.text) 
		*(.rodata)
	}


	. = ALIGN(4);

	.data : {*(.data)}

	. = ALIGN(4);

	_bss_start = .;
	.bss : {*(.bss)}
	_bss_end = .;

}
