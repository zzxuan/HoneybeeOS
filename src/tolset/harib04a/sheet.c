//图层

#include "bootpack.h"

#define SHEET_USE		1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	ctl-> map = (unsigned char *)memman_alloc_4k(memman, xsize*ysize);
	if(ctl-> map==0){
		memman_free_4k(memman,(int)ctl,sizeof (struct SHTCTL));
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; //没有??
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; //没使用??
		ctl->sheets0[i].ctl=ctl;
	}
err:
	return ctl;
}

//生成图层
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE; //该图层已使用
			sht->height = -1; //高度为-1 隐藏
			return sht;
		}
	}
	return 0;	//没有可用图层
}
//设置图层数据
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;//透明色色号
	return;
}
//设置图层高度
void sheet_updown( struct SHEET *sht, int height)
{
	struct SHTCTL *ctl=sht->ctl;
	int h, old = sht->height; //设定前高度

	//范围判断
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height; //?置高度

	//???排序
	if (old > height) {	//比以前低的情况
		if (height >= 0) {
			
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;//?置
			
			sheet_refreshmap(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,height+1); //刷新
			sheet_refreshsub(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,height+1,old); //刷新
		} else {//高度?-1?藏
			if (ctl->top > old) {
				
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; 
			sheet_refreshmap(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,0); //刷新
			sheet_refreshsub(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,0,old-1); //刷新
		}

	} else if (old < height) {	//比以前高
		if (old >= 0) {
			
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;//?置
		} else {	//以前?藏?
			
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;//?置
			ctl->top++; //增加一?
		}
		sheet_refreshmap(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,height); //刷新
		sheet_refreshsub(ctl,sht->vx0,sht->vy0,sht->vx0+sht->bxsize,sht->vy0+sht->bysize,height,height); //刷新
	}
	return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,int h0)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	
	if(vx0<0){vx0=0;}
	if(vy0<0){vy0=0;}
	if(vx1>ctl->xsize){vx1=ctl->xsize;}
	if(vy1>ctl->ysize){vy1=ctl->ysize;}
	
	for (h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		sid=sht - ctl->sheets0;
		buf = sht->buf;
		//根据vx0~vy1,算出bx0~by1
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {//从bx0~by1循环减少时间
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {//从bx0~by1循环减少时间
				vx = sht->vx0 + bx;
				if (buf[by * sht->bxsize + bx] != sht->col_inv) {
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}

//指定范围刷新 h0指基础图层高度 在此图层上
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1,int h0,int h1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram,*map = ctl->map,sid;
	struct SHEET *sht;
	
	if(vx0<0){vx0=0;}
	if(vy0<0){vy0=0;}
	if(vx1>ctl->xsize){vx1=ctl->xsize;}
	if(vy1>ctl->ysize){vy1=ctl->ysize;}
	
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid=sht - ctl->sheets0;
		//根据vx0~vy1,算出bx0~by1
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {//从bx0~by1循环减少时间
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {//从bx0~by1循环减少时间
				vx = sht->vx0 + bx;
				if (map[vy * ctl->xsize + vx] == sid) {
					vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
				}
			}
		}
	}
	return;
}

void sheet_refresh(struct SHEET *sht,int bx0,int by0,int bx1,int by1)
{

	if(sht->height>=0){
		sheet_refreshsub(sht->ctl,sht->vx0 + bx0,sht->vy0+by0,sht->vx0+bx1,sht->vy0+by1,sht->height,sht->height);
	}
	return;
}

//移动
void sheet_slide( struct SHEET *sht, int vx0, int vy0)
{
	int old_vx0=sht->vx0,old_vy0=sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { 
		sheet_refreshmap(sht->ctl,old_vx0,old_vy0,old_vx0 + sht->bxsize,old_vy0 + sht->bysize,0);
		sheet_refreshmap(sht->ctl,vx0,vy0,vx0 + sht->bxsize,vy0 + sht->bysize,sht->height);
		sheet_refreshsub(sht->ctl,old_vx0,old_vy0,old_vx0 + sht->bxsize,old_vy0 + sht->bysize,0,sht->height-1);
		sheet_refreshsub(sht->ctl,vx0,vy0,vx0 + sht->bxsize,vy0 + sht->bysize,sht->height,sht->height);
	}
	return;
}
//释放
void sheet_free( struct SHEET *sht)
{
	if (sht->height >= 0) {
		sheet_updown(sht, -1); 
	}
	sht->flags = 0; 
	return;
}
