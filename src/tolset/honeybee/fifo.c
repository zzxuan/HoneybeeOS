#include "bootpack.h"
//#include <stdio.h>
#define FLAGS_OVERRUN       0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf)
//��ʼ��������
{
	fifo->size=size;
	fifo->buf=buf;
	fifo->free=size;
	fifo->flags=0;
	fifo->p=0;
	fifo->q=0;
	return;
}

int fifo32_put(struct FIFO32 *fifo, int data)//��������д����
{
	if(fifo->free==0){
		fifo->flags |= FLAGS_OVERRUN;//���
		return -1;
	}
	fifo->buf[fifo->p]=data;
	fifo->p++;
	if(fifo->p==fifo->size){
		fifo->p=0;//תһȦ
	}
	fifo->free--;
	return 0;
}

int fifo32_get(struct FIFO32 *fifo)//�ӻ�����������
{
	int data;
	if(fifo->free==fifo->size){
		return -1;//������Ϊ��
	}
	data =fifo->buf[fifo->q];
	fifo->q++;
	if(fifo->q==fifo->size){
		fifo->q=0;
	}
	fifo->free++;
	return data;
	
}

int fifo32_status(struct FIFO32 *fifo)//������ʣ������
{
	return fifo->size - fifo->free;
}


































