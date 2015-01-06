#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo;

struct MOUSE_DEC{
	unsigned char buf[3],phase;
	int x,y,btn;
};


void enable_mouse(struct MOUSE_DEC *mdec);

int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat);

void init_keyboard(void);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256],keybuf[32],mousebuf[128];
	int mx, my,i;

	init_gdtidt();
	init_pic();
	io_sti(); 

	fifo8_init(&keyfifo,32,keybuf);//n»???tæ
	fifo8_init(&mousefifo, 128, mousebuf);//n»l??tæ
	io_out8(PIC0_IMR, 0xf9); /* PIC ???Âi11111001j?Â*/
	io_out8(PIC1_IMR, 0xef); /* PICl??Â(11101111) */
	init_keyboard();//n»??

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; /* æÊ¿??Z */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	struct MOUSE_DEC mdec;//l?
	enable_mouse(&mdec);//l?
	
	for (;;) {
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if(mouse_decode(&mdec,i)==1)//Ú¾®l?
				{
					//sprintf(s,"%02X %02X %02X",mdec.buf[0],mdec.buf[1],mdec.buf[2]);
					sprintf(s,"[lcr %4d %4d]",mdec.x,mdec.y);
					if((mdec.btn&0x01)!=0){
						s[1]='L';
					}
					if((mdec.btn&0x02)!=0){
						s[1]='R';
					}
					if((mdec.btn&0x04)!=0){
						s[1]='C';
					}
					
					
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  32, 16, 32+15*8-1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					
					//Ú?l?
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  mx, my, mx+15, my+15);
					mx+=mdec.x;
					my+=mdec.y;
					if(mx<0){
						mx=0;
					}
					if(my<0){
						my=0;
					}
					if(mx>binfo->scrnx-16){
						mx=binfo->scrnx-16;
					}
					if(my>binfo->scrny-16){
						my=binfo->scrny-16;
					}
					
					sprintf(s,"(%3d %3d)",mx,my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 0, 79, 15);
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);//?¦¿??
					
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
				
			}
		}
	}
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)//Ò??T§?Hy?®?
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
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec)//l?
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	
	mdec->phase=0;
	return; 
}

int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat)
{
	if(mdec->phase==0){
		if(dat==0xfa){
			mdec->phase=1;
		}
	} else if(mdec->phase==1){
		if((dat&0xc8)==0x08){//»fæê¢³?
			mdec->buf[0]=dat;
			mdec->phase=2;
		}
	}else if(mdec->phase==2){
		mdec->buf[1]=dat;
		mdec->phase=3;
	}else if(mdec->phase==3){
		mdec->buf[2]=dat;
		mdec->phase=1;
		
		mdec->btn=mdec->buf[0]&0x07;
		mdec->x=mdec->buf[1];
		mdec->y=mdec->buf[2];
		
		if((mdec->buf[0]&0x10)!=0){
			mdec->x|=0xffffff00;
		}
		if((mdec->buf[0]&0x20)!=0){
			mdec->y|=0xffffff00;
		}
		mdec->y=-mdec->y;
		return 1;
	}else{
		return -1;//?í
	}
	return 0;
}

























