#ifndef __GATE_H__
#define __GATE_H__


struct desc_struct
{
	unsigned char x[8];
};

struct gate_struct
{
	unsigned char x[16];
};

extern struct desc_struct GDT_Table[];
extern struct gate_struct IDT_Table[];
extern unsigned int TSS64_Table[26];

// 此处根据门描述符结构,初始化中断描述符表内的门描述符
//
// 中断描述符结构: 64bit  IST: 中断栈表
//
// 127---------------------------------96 95---------------------------------64
//  |               保留               |  |           段内偏移 63:32          |
//                                                       |
//  63------------48 47 46-45 44 43--40 39-37 36 35 34-32|31------16 15-------0
//  |段内偏移 31:16| P   DPL  0  |Type| | 0 | 0  0  |IST | 段选择子| |段内偏移|
//


// 初始化中断描述符表内的门描述符,对应以上位结构
#define _set_gate(gate_selector_addr, attr, ist, code_addr)\
do 	\
{	unsigned long __d0, __d1;	\
	__asm__ __volatile__ (			\
		"movw	%%dx,	%%ax	;"	\
		"andq	$0x7,	%%rcx 	;"	\
		"addw 	%4,		%%cx	;"	\
		"shlq	$32,	%%rcx 	;"	\
		"addq 	%%rcx,	%%rax 	;"	\
		"xorq 	%%rcx,	%%rcx 	;"	\
		"movq 	%%rdx,	%%rcx	;"	\
		"shrq	$16,	%%rcx 	;"	\
		"shlq 	$48,	%%rcx 	;"	\
		"addq 	%%rcx,	%%rax 	;"	\
		"shrq 	$32,	%%rdx 	;"	\
		"movq 	%%rax,	%0		;"	\
		"movq 	%%rdx,	%1 		;"	\
		:"=m"(*((unsigned long *)(gate_selector_addr))),	\
		"=m"(*(1 + (unsigned long *)(gate_selector_addr))),	\
		"=&a"(__d0), "=&d"(__d1)	\
		:"i"(attr << 8), "c"(ist), "2"(0x8<<16), "3"((unsigned long *)(code_addr))	\
		:"memory" 	\
	);				\
}while(0);




#define load_TR(n) 							\
do{											\
	__asm__ __volatile__(	"ltr	%%ax"	\
				:							\
				:"a"(n << 3)				\
				:"memory");					\
}while(0)


// #define _set_gate(gate_selector_addr,attr,ist,code_addr)	\
// do								\
// {	unsigned long __d0,__d1;				\
// 	__asm__ __volatile__	(	"movw	%%dx,	%%ax	\n\t"	\
// 					"andq	$0x7,	%%rcx	\n\t"	\
// 					"addq	%4,		%%rcx	\n\t"	\
// 					"shlq	$32,	%%rcx	\n\t"	\
// 					"addq	%%rcx,	%%rax	\n\t"	\
// 					"xorq	%%rcx,	%%rcx	\n\t"	\
// 					"movl	%%edx,	%%ecx	\n\t"	\
// 					"shrq	$16,	%%rcx	\n\t"	\
// 					"shlq	$48,	%%rcx	\n\t"	\
// 					"addq	%%rcx,	%%rax	\n\t"	\
// 					"movq	%%rax,	%0	\n\t"	\
// 					"shrq	$32,	%%rdx	\n\t"	\
// 					"movq	%%rdx,	%1	\n\t"	\
// 					:"=m"(*((unsigned long *)(gate_selector_addr)))	,					\
// 					 "=m"(*(1 + (unsigned long *)(gate_selector_addr))),"=&a"(__d0),"=&d"(__d1)		\
// 					:"i"(attr << 8),									\
// 					 "3"((unsigned long *)(code_addr)),"2"(0x8 << 16),"c"(ist)				\
// 					:"memory"		\
// 				);				\
// }while(0)


/*

*/


/*

*/

inline void set_intr_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0x8E , ist , addr);	//P,DPL=0,TYPE=E
}

/*

*/

inline void set_trap_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0x8F , ist , addr);	//P,DPL=0,TYPE=F
}

/*

*/

inline void set_system_gate(unsigned int n,unsigned char ist,void * addr)
{
	_set_gate(IDT_Table + n , 0xEF , ist , addr);	//P,DPL=3,TYPE=F
}

/*

*/

inline void set_system_intr_gate(unsigned int n,unsigned char ist,void * addr)	//int3
{
	_set_gate(IDT_Table + n , 0xEE , ist , addr);	//P,DPL=3,TYPE=E
}


/*

*/

void set_tss64(unsigned long rsp0,unsigned long rsp1,unsigned long rsp2,unsigned long ist1,unsigned long ist2,unsigned long ist3,
unsigned long ist4,unsigned long ist5,unsigned long ist6,unsigned long ist7)
{
	*(unsigned long *)(TSS64_Table+1) = rsp0;
	*(unsigned long *)(TSS64_Table+3) = rsp1;
	*(unsigned long *)(TSS64_Table+5) = rsp2;

	*(unsigned long *)(TSS64_Table+9) = ist1;
	*(unsigned long *)(TSS64_Table+11) = ist2;
	*(unsigned long *)(TSS64_Table+13) = ist3;
	*(unsigned long *)(TSS64_Table+15) = ist4;
	*(unsigned long *)(TSS64_Table+17) = ist5;
	*(unsigned long *)(TSS64_Table+19) = ist6;
	*(unsigned long *)(TSS64_Table+21) = ist7;
}


#endif



