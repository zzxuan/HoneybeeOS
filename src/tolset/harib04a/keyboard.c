#include "bootpack.h"
#include <stdio.h>

#define PORT_KEYDAT		0x0060

struct FIFO8 keyfifo;//键盘缓冲区

void inthandler21(int *esp)//??中断?理
{
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	/* ?始接受中断 */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo,data);
	return;
}

void wait_KBC_sendready(void)//等待键盘准备
{
	while(1)
	{
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY)==0){
			break;
		}
	}
	return;
}

void init_keyboard(void){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD,KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}