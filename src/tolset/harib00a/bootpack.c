void io_hlt(void);
//void write_mem8(int addr, int data);

void HariMain(void)
{
	int i; 
	char *p;
	for (i = 0xa0000; i <= 0xaffff; i++) {
		//write_mem8(i, i&0x0f); /* MOV BYTE [i],15 */
		p=i;
		*p=0xf;
	}

	for (;;) {
		io_hlt();
	}
}
