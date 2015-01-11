#include "bootpack.h"
//#include <stdio.h>
#define FLAGS_OVERRUN       0x0001

void fifo8_init(struct FIFO8 *fifo,int size,unsigned char *buf)//初始化缓冲区
{
	fifo->size=size;
	fifo->buf=buf;
	fifo->free=size;
	fifo->flags=0;
	fifo->p=0;
	fifo->q=0;
	return;
}

int fifo8_put(struct FIFO8 *fifo,unsigned char data)//给缓冲区写数据
{
	if(fifo->free==0){
		fifo->flags |= FLAGS_OVERRUN;//溢出
		return -1;
	}
	fifo->buf[fifo->p]=data;
	fifo->p++;
	if(fifo->p==fifo->size){
		fifo->p=0;//转一圈
	}
	fifo->free--;
	return 0;
}

int fifo8_get(struct FIFO8 *fifo)//从缓冲区读数据
{
	int data;
	if(fifo->free==fifo->size){
		return -1;//缓冲区为空
	}
	data =fifo->buf[fifo->q];
	fifo->q++;
	if(fifo->q==fifo->size){
		fifo->q=0;
	}
	fifo->free++;
	return data;
	
}

int fifo8_status(struct FIFO8 *fifo)//缓冲区剩余数据
{
	return fifo->size - fifo->free;
}


































