MEMORY
{
	USER_RAM (rwx) : ORIGIN = 0x20100000, LENGTH = 0x00800000
}

SECTIONS
{
	.text :
	{
		KEEP(*(TOP))
		*(.text)
	} >USER_RAM
	.rodata :
	{
		*(.rodata)
	} >USER_RAM
	.bss :
	{
		*(.bss)
	} >USER_RAM
}
