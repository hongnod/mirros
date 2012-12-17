ENTRY(main)
SECTIONS
{
	. = 0xc0018000;
	.text : 
	{
		*(.text) 
		*(.rodata)
	}


	. = ALIGN(4);

	.data : {*(.data)}

	_bss_start = .;
	.bss : {*(.bss)}
	_bss_end = .;

}
