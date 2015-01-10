#include "bootpack.h"
#include <stdio.h>


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

unsigned int memman_alloc_4k(struct MEMMAN *man,unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a= memman_alloc(man,size);
	return a;
}

int memman_free_4k(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man,addr,size);
	return i;
}




