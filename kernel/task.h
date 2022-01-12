#ifndef __TASK_H__

#define __TASK_H__

#include "memory.h"
#include "cpu.h"
#include "lib.h"
#include "ptrace.h"

#define KERNEL_CS 	(0x08)
#define	KERNEL_DS 	(0x10)

#define	USER_CS		(0x28)
#define USER_DS		(0x30)

#define CLONE_FS	(1 << 0)
#define CLONE_FILES	(1 << 1)
#define CLONE_SIGNAL	(1 << 2)

// stack size 32K
#define STACK_SIZE 32768

extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;

extern void ret_from_intr();

// 进程状态
#define TASK_RUNNING		(1 << 0)
#define TASK_INTERRUPTIBLE	(1 << 1)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)
#define	TASK_ZOMBIE		(1 << 3)	
#define	TASK_STOPPED		(1 << 4)



struct mm_struct
{
	pml4t_t *pgd;	                            // 内存页表指针
	
	unsigned long start_code,end_code;          // 代码段空间
	unsigned long start_data,end_data;          // 数据段空间
	unsigned long start_rodata,end_rodata;      // 只读数据段空间
	unsigned long start_brk,end_brk;            // 动态内存分配区(堆区域)
	unsigned long start_stack;	                // 应用层栈基地址
};



struct thread_struct
{
	unsigned long rsp0;	                // 内核层栈基地址

	unsigned long rip;                  // 内核层代码指针
	unsigned long rsp;	                // 内核层当前栈指针

	unsigned long fs;                   // FS段寄存器
	unsigned long gs;                   // GS段寄存器

	unsigned long cr2;                  // CR2控制寄存器
	unsigned long trap_nr;              // 产生异常的异常号
	unsigned long error_code;           // 异常的错误码
};


#define PF_KTHREAD	(1 << 0)

struct task_struct
{
	struct List list;               // 双向链表,用于连接各个进程控制结构体
	volatile long state;            // 进程状态
	unsigned long flags;            // 进程标志

	struct mm_struct *mm;           // 内存空间分布结构体,记录内存页表和程序段信息
	struct thread_struct *thread;   // 进程切换时保留的状态信息
                                    
                                    // 进程地址空间范围
    unsigned long addr_limit;	    /*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff user*/
					                /*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff kernel*/

	long pid;                       // 进程ID号

	long counter;                   // 进程可用时间片

	long signal;                    // 进程持有的信号

	long priority;                  // 进程优先级
};

union task_union
{
	struct task_struct task;
	unsigned long stack[STACK_SIZE / sizeof(unsigned long)];
}__attribute__((aligned (8)));  // 8B 对齐

struct mm_struct init_mm;
struct thread_struct init_thread;


// 将联合体 union task_union 实例化成全局变量 init_task_union
// 其中 tsk 是什么似乎无所谓,编译后该段信息直接位于指定地址  
#define INIT_TASK(tsk)	\
{			\
	.state = TASK_UNINTERRUPTIBLE,		\
	.flags = PF_KTHREAD,		\
	.mm = &init_mm,			\
	.thread = &init_thread,		\
	.addr_limit = 0xffff800000000000,	\
	.pid = 0,			\
	.counter = 1,		\
	.signal = 0,		\
	.priority = 0		\
}

// 将该全局变量链接到指定程序段内,在链接脚本kernel.lds中指定了程序段的地址空间
union task_union init_task_union __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};

struct task_struct *init_task[NR_CPUS] = {&init_task_union.task,0};

struct mm_struct init_mm = {0};

struct thread_struct init_thread = 
{
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
	.rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),
	.fs = KERNEL_DS,
	.gs = KERNEL_DS,
	.cr2 = 0,
	.trap_nr = 0,
	.error_code = 0
};

/*

*/


// TSS 结构. 该结构体是一个紧凑结构,编译器不会对此结构体内的成员变量进行字节对齐
struct tss_struct
{
	unsigned int  reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomapbaseaddr;
}__attribute__((packed));

// 初始化 TSS 信息
#define INIT_TSS \
{	.reserved0 = 0,	 \
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	 \
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0,	\
	.reserved3 = 0,	\
	.iomapbaseaddr = 0	\
}

struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };

/*

*/

// 在当前栈指针寄存器RSP的基础上,按 32 KB下边界对齐实现
inline	struct task_struct * get_current()
{
	struct task_struct * current = NULL;
	__asm__ __volatile__ ("andq %%rsp,%0	\n\t":"=r"(current):"0"(~32767UL));
	return current;
}

#define current get_current()

#define GET_CURRENT			\
	"movq	%rsp,	%rbx	\n\t"	\
	"andq	$-32768,%rbx	\n\t"




/*
 * 应用程序在进入内核层时已将进程的执行现场(所有通用寄存器值)保存起来,所以进程切换过
 * 程并不涉及保存/还原通用寄存器,仅需对RIP寄存器和RSP寄存器进行设置
 */
#define switch_to(prev,next)			\
do{							\
	__asm__ __volatile__ (	"pushq	%%rbp	\n\t"	\
				"pushq	%%rax	\n\t"	\
				"movq	%%rsp,	%0	\n\t"	\
				"movq	%2,	%%rsp	\n\t"	\
				"leaq	1f(%%rip),	%%rax	\n\t"	\
				"movq	%%rax,	%1	\n\t"	\
				"pushq	%3		\n\t"	\
				"jmp	__switch_to	\n\t"	\
				"1:	\n\t"	\
				"popq	%%rax	\n\t"	\
				"popq	%%rbp	\n\t"	\
				:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)		\
				:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
				:"memory"		\
				);			\
}while(0)

/*

*/

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);
void task_init();

#endif
