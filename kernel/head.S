#include "linkage.h"

.section .text
.globl _start

//  64位环境下地址空间布局
//      2^48 256T
// ---------------------
// 0xFFFF FFFF FFFF FFFF
//       内核空间
// 0xFFFF 8000 0000 0000
// ---------------------
//       中间为空
// ---------------------
// 0x0000 7FFF FFFF FFFF
//       用户空间
// 0x0000 0000 0000 0000
// ---------------------

_start:

	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%ss
	mov	$0x7E00,	%esp

// 加载 GDTR

	lgdt	GDT_POINTER(%rip)

// 加载 IDTR

	lidt	IDT_POINTER(%rip)

	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%gs
	mov	%ax,	%ss

	movq	$0x7E00,	%rsp

//  加载 CR3 寄存器

	movq	$0x101000,	%rax
	movq	%rax,		%cr3


	movq	switch_seg(%rip),	%rax    // 0008:000000000010003e
	pushq	$0x08
	pushq	%rax
	lretq

//  64位模式代码
switch_seg:
	.quad	entry64

entry64:
	movq	$0x10,	%rax        // 0008:ffff800000100052 
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%gs
	movq	%rax,	%ss
	movq	$0xffff800000007E00,	%rsp		/* rsp address */

// 初始化 IDT 
setup_IDT:
    leaq    ignore_int(%rip),   %rdx
    movq    $(0x08 << 16),      %rax
    movw    %dx,                %ax
    movq    $(0x8E00 << 32),    %rcx
    addq    %rcx,               %rax
    movl    %edx,               %ecx
    shrl    $16,                %ecx
    shlq    $48,                %rcx
    addq    %rcx,               %rax
    shrq    $32,                %rdx
    leaq    IDT_Table(%rip),    %rdi
    mov     $256,               %rcx
rp_sidt:
    movq    %rax,   (%rdi)
    movq    %rdx,   8(%rdi)
    addq    $0x10,  %rdi
    dec     %rcx
    jne rp_sidt

//  初始化 TSS
setup_TSS64:
    leaq    TSS64_Table(%rip),  %rdx
    xorq    %rax,               %rax
    xorq    %rcx,               %rcx
    movq    $0x89,              %rax
    shlq    $40,        %rax
    movl    %edx,       %ecx
    shrl    $24,        %ecx
    shlq    $56,        %rcx
    addq    %rcx,       %rax
    xorq    %rcx,       %rcx
    movl    %edx,       %ecx
    andl    $0xffffff,  %ecx
    shlq    $16,        %rcx
    addq    %rcx,       %rax
    addq    $103,       %rax
    leaq    GDT_Table(%rip),  %rdi
    movq    %rax,       64(%rdi)
    shrq    $32,        %rdx
    movq    %rdx,       72(%rdi)

    // movw    $0x40,      %ax
    // ltr     %ax

	movq	go_to_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq

//  跳转至 C 内核代码执行
go_to_kernel:
	.quad	Start_Kernel


// 中断和异常处理模块
ignore_int:
	cld

    // 保存各寄存器值
	pushq	%rax
	pushq	%rbx
	pushq	%rcx
	pushq	%rdx
	pushq	%rbp
	pushq	%rdi
	pushq	%rsi

	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	movq	%es,	%rax
	pushq	%rax
	movq	%ds,	%rax
	pushq	%rax

    // DS和ES段寄存器设置成内核数据段
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es

    // 为 color_printk 函数准备参数,并采用寄存器传递方式向其传递参数
	leaq	int_msg(%rip),	%rax			// 此处使用 IA-32e 特有的 rip 相对寻址
	pushq	%rax
	movq	%rax,	%rdx
	movq	$0x00000000,	%rsi
	movq	$0x00ff0000,	%rdi
	movq	$0,	%rax
	callq	color_printk                    // 打印相关信息
	addq	$0x8,	%rsp

    /*
     *  在x64模式下,大部分编译器采用寄存器传递参数,参数按照
     *  从左向右的顺序依次是RDI、RSI、RDX、RCX、R8、R9,
     *  剩余参数使用内存传递方式,RAX放置函数的返回值,调用者负责平衡栈。
     */
     
Loop:
	jmp	Loop
    // 恢复寄存器信息
	popq	%rax
	movq	%rax,	%ds
	popq	%rax
	movq	%rax,	%es

	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8

	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	%rdx
	popq	%rcx
	popq	%rbx
	popq	%rax
	iretq

int_msg:
	.asciz "Unknown interrupt or fault at RIP\n"

ENTRY(_stack_start)
	.quad	init_task_union + 32768

// 初始化页目录及页表
.align 8                    // 每个页表项为 8B

.org	0x1000              




/*
 *  各级页表位图及详细介绍: https://bbs.pediy.com/thread-203391.htm
 *  Canonical型地址介绍: https://v1ckydxp.github.io/2019/08/30/2019-08-30-%E3%80%8A%E6%B7%B1%E5%85%A5%E7%90%86%E8%A7%A3linux%E5%86%85%E6%A0%B8%E3%80%8B%E9%98%85%E8%AF%BB%E7%AC%94%E8%AE%B0/
 *  四级页表寻址机制: https://zhuanlan.zhihu.com/p/50824798
 */

/*
 *  以下结构根据书上说法将前 10M 地址映射至用户区起始处和内核区起始处
 *  将 0xE000 0000 开始的 16M 帧缓冲区映射到 0xA0000 处
 *  为了实现滚屏效果本人将 0x66000083 开始的 20M 地址映射至 0xFD0 0000
 */

/*
 *  采用三级页表的寻址方式. 参考p235页表位图默认情况下为四级页表寻址
 *  此时对应页面大小为 4K 即页内偏移表示为 11 位 => 2^11 = 4K
 *  故采用 2M 物理页时,页内偏移为 21 位 => 2^21 = 2M 即三级页表
 */

/*
 *  一个 PML4E 表项对应 512G 的空间大小. 由地址映射位图可知三级页表时
 *  PML4=>9bit PDT=>9bit PDT=>9bit 页面大小 2M
 *  故一个 PML4E 表项对应空间为 2^9 * 2^9 * 2M = 512G
 */

__PML4E:                    // 单个表项对应 512G 的地址空间
	.quad	0x102007        // 256*512G = 2^8 * 2^9 * 2^30 = 2^47
	.fill	255,8,0         // 0x0000 8000 0000 0000 由于CPU遵循 AMD 的Canonical form
	.quad	0x102007        // 任何虚拟地址的48位至63位必须与47位一致(sign extension)
	.fill	255,8,0         // 故 0x8000 0000 0000 => 0xFFFF 8000 0000 0000

.org	0x2000
__PDPTE:
	.quad	0x103003        // 单个表项对应空间 2^9 * 2^21 = 1G 
	.fill	511,8,0         

.org	0x3000


/*
 *  对于 2M 的物理页面, PDE位图中 51-21 位表示物理基地址,以 0xe0000083 为例
 *  0x0xe0000083 => 1110 0000 0000 0000 0000 0000 1000 0011
 *                  |  物理基地址 |    
 *  MAXPHYADDR 原则低21位补0
 */

__PDE:                      // 单个表项对应 2M 空间

#include "PDE_entrys"

//	.quad	0x000083        
//	.quad	0x200083
//	.quad	0x400083
//	.quad	0x600083
//	.quad	0x800083
//
//    // 1110 0000 0000 0000 0000 0000 1000 0011
//    // 具体映射位置需根据不同显示芯片进行调整
//
//	.quad	0xfd000083	    /*0xa0 0000*/   // 显卡内存空间 
//	.quad	0xfd200083
//	.quad	0xfd400083
//	.quad	0xfd600083		/*0x1000000*/
//	.quad	0xfd800083
//
//	.quad	0xfda00083
//	.quad	0xfdc00083
//	.quad	0xfde00083
//    .quad	0x66000083		/*0x1a0 0000*/ // 打印函数的中转空间
//	.quad	0x66200083
//
//	.quad	0x66400083
//	.quad	0x66600083
//	.quad	0x66800083
//	.quad	0x66a00083
//	.quad	0x66c00083
//
//	.quad	0x66e00083
//    .quad	0x67000083      /* 缺少以下两项后打印失败 */
//	.quad	0x67200083      /* 补全 printk 的相关文档 */
//
//	.fill	489,8,0

//=======	GDT_Table

.section .data                          

.globl GDT_Table                        // 可被外部访问,即C/C++中 extern
GDT_Table:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	    00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
	.quad	0x0000920000000000			/*2	KERNEL	Data	64-bit	Segment	10*/
	.quad	0x0020f80000000000			/*3	USER	Code	64-bit	Segment	18*/
	.quad	0x0000f20000000000			/*4	USER	Data	64-bit	Segment	20*/
	.quad	0x00cf9a000000ffff			/*5	KERNEL	Code	32-bit	Segment	28*/
	.quad	0x00cf92000000ffff			/*6	KERNEL	Data	32-bit	Segment	30*/
	.fill	10,8,0					    /*8 ~ 9	TSS (jmp one segment <7>) in long-mode 128-bit 40*/
GDT_END:

GDT_POINTER:                            // 以下各 POINTER 指向信息即各描述符表寄存器加载的信息
GDT_LIMIT:	.word	GDT_END - GDT_Table - 1
GDT_BASE:	.quad	GDT_Table


.globl IDT_Table
IDT_Table:
	.fill  512,8,0
IDT_END:

IDT_POINTER:
IDT_LIMIT:	.word	IDT_END - IDT_Table - 1
IDT_BASE:	.quad	IDT_Table


.globl	TSS64_Table
TSS64_Table:
	.fill  13,8,0
TSS64_END:


TSS64_POINTER:
TSS64_LIMIT:	.word	TSS64_END - TSS64_Table - 1
TSS64_BASE:	.quad	TSS64_Table
