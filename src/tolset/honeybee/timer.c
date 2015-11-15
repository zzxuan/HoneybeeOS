#include "bootpack.h"
#include <stdio.h>

#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040

#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2

struct TIMERCTL timerctl;

extern struct TIMER *mt_timer;//任务切换


void init_pit(void)
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++) {
		timerctl.timers0[i].flags = 0; //没有使用
	}
	t = timer_alloc(); //给最后插入一个
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0;
	timerctl.t0 = t; 
	timerctl.next = 0xffffffff; 
	return;
}

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0; //没有了
}

void timer_free(struct TIMER *timer)
{
	timer->flags = 0; /* 未使用 */
	return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		//最前面 给链表最前面插入
		timerctl.t0 = timer;
		timer->next = t; 
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	/* 查找插入位置 */
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			//给链表中间插入
			s->next = timer; 
			timer->next = t; 
			io_store_eflags(e);
			return;
		}
	}
}

/*
*
*这里产生pit中断
*
*/
void inthandler20(int *esp)//pit中断
{
	char ts = 0;//任务管理标志
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	//通知PIC
	timerctl.count++;
	if (timerctl.next > timerctl.count) {
		return;
	}
	timer = timerctl.t0; 
	for (;;) {
		if (timer->timeout > timerctl.count) {
			break;
		}

		timer->flags = TIMER_FLAGS_ALLOC;

		if(timer != mt_timer){
			fifo32_put(timer->fifo, timer->data);
		} else {
			ts = 1;
		}
		
		timer = timer->next; 
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;

	//任务切换
	if(ts != 0){
		mt_taskswitch();
	}
	
	return;
}



