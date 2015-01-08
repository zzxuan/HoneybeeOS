#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo, mousefifo;

#define MEMMAN_FREES		4090
#define MEMMAN_ADDR			0x003c0000

struct FREEINFO{
	unsigned int addr,size;
};

struct MEMMAN{//内存管理
	int frees,maxfrees,lostsize,losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start,unsigned int end);//内存检测
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);//空余内存大小
unsigned int memman_alloc(struct MEMMAN *man,unsigned int size);//分配内存
int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size);//释放内存

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256],keybuf[32],mousebuf[128];
	int mx, my,i;

	init_gdtidt();
	init_pic();
	io_sti(); 
	/********键盘与鼠标设置***************************/
	fifo8_init(&keyfifo,32,keybuf);//
	fifo8_init(&mousefifo, 128, mousebuf);//嬫
	io_out8(PIC0_IMR, 0xf9); /* PIC设置键盘与鼠标*/
	io_out8(PIC1_IMR, 0xef); 
	init_keyboard();//初始化键盘

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;//鼠标坐标
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	struct MOUSE_DEC mdec;//鼠标缓冲区
	enable_mouse(&mdec);//初始化鼠标
	
	//******************内存**************************

	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	memtotal = memtest(0x00400000,0xbfffffff);
	memman_init(memman);
	memman_free(memman,0x00001000,0x0009e000);
	memman_free(memman,0x00400000,memtotal - 0x00400000);
	sprintf(s,"memory %dMB   free : %dKB",memtotal/(1024*1024),memman_total(memman)/1024);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	
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
				if(mouse_decode(&mdec,i)==1)//接收完鼠标数据
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
					
					//堏?憀?
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
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);//显示坐标
					
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);//绘制鼠标
				}
				
			}
		}
	}
}

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start,unsigned int end)//内存检测
{
	char flg486=0;
	unsigned int eflg,cr0,i;
	//判断CPU是386还是486以上
	eflg = io_load_eflags();
	eflg |=EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	
	eflg = io_load_eflags();
	
	if((eflg&EFLAGS_AC_BIT)!=0){
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT;
	
	io_store_eflags(eflg);
	
	if(flg486!=0){
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; //禁止高速缓存
		store_cr0(cr0);
	}
	
	i= memtest_sub(start,end);
	
	if(flg486!=0){
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; //允许高速缓存
		store_cr0(cr0);
	}
	
	return i;
}



void memman_init(struct MEMMAN *man)
{
	man->frees = 0;//可用信息数目
	man->maxfrees = 0;
	man->lostsize=0;//释放失败的内存大小总和
	man->losts=0;//释放失败次数
	return;
}

unsigned int memman_total(struct MEMMAN *man)//空余内存大小
{
	unsigned int i,t=0;
	for(i=0;i < man->frees;i++){
		t+=man->free[i].size;
	}	
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man,unsigned int size)//分配内存
{
	unsigned int i,a;
	for(i=0;i< man->frees;i++){
		if(man->free[i].size >= size){//找到内存
			a=man->free[i].addr;
			man->free[i].addr += size;
			man -> free[i].size -= size;
			if(man->free[i].size == 0){//去除一条信息
				man->frees--;
				for(;i<man->frees;i++){
					man -> free[i] = man ->free[i+1];//移一步
				}
			}
			return a;
		}
	}
	return 0;//没有可用内存
}

int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size)//释放内存
{
	int i,j;
	for(i=0;i<man->frees;i++){
		if(man->free[i].addr>addr){
			break;
		}
	}
	
	if(i>0){
		if(man->free[i-1].addr + man -> free[i-1].size==addr){
			//可以放在前面
			man->free[i-1].size += size;
			if(i< man->frees){
				if(addr + size ==man -> free[i].addr){
					man->free[i-1].size += man->free[i].size;
					man -> frees--;
					for(;i<man->frees;i++){
						man -> free[i] = man ->free[i+1];//移一步
					}
				}
			}
			return 0;
		}
	}
	if(i< man->frees){
		//后面还有
		if(addr + size == man-> free[i].addr){
			man->free[i].addr =addr;
			man->free[i].size += size;
			return 0;
		}
	}
	
	if(man->frees< MEMMAN_FREES){
		for(j=man->frees;j>i;j--){
			man->free[j] = man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees<man->frees){
			man -> maxfrees = man->frees;
		}
		man->free[i].addr =addr;
		man-> free[i].size = size;
		return 0;
	}
	
	//失败
	man->losts++;
	man->lostsize += size;
	return -1;
	
	
	
}





























