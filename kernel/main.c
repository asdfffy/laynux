// #include "lib.h"

#include "printk.h"
#include "trap.h"
#include "gate.h"
#include "lib.h"
#include "memory.h"
#include "task.h"

extern char _text;
extern char _etext;
extern char _edata;
extern char _end;



struct Global_Memory_Descriptor memory_management_struct;

// Loader 加载程序 设置显示模式 0x180 分辨率: 1440x900
void Start_Kernel(void)
{


	// struct Page * page = NULL;
    // // 内核链接时 将线性地址 0xffff800000000000 映射至物理地址 0 处

	// int *addr = (int *)0xffff800000a00000;
	// int i;

    // Pos.XResolution = 1440;
    // Pos.YResolution = 900;

    // Pos.XPosition = 0;
    // Pos.YPosition = 0;

    // Pos.XCharSize = 8;
    // Pos.YCharSize = 16;


    // Pos.FB_addr = (int *)0xffff800000a00000;                // ==> 0xe0000000 帧缓冲区
	// Pos.FB_length = Pos.XResolution * Pos.YResolution * 4;


    // int a = 0;
    // int b = 0;
    // int c = 0;

	// for(i = 0 ;i<1440*16;i++)
    // {
	// 	*((char *)addr+0)=(char)a;
	// 	*((char *)addr+1)=(char)b;
	// 	*((char *)addr+2)=(char)c;
	// 	*((char *)addr+3)=(char)0x00;
    //     a += 4;
    //     b += 8;
    //     c += 16;
	// 	addr +=1;
	// }

struct Page * page = NULL;
    // 内核链接时 将线性地址 0xffff800000000000 映射至物理地址 0 处

	int *addr = (int *)0xffff800000a00000;
	int * fb_addr = frame_buffer;

	int i;
    int test;

    Pos.XResolution = 1440;
    Pos.YResolution = 900;

    Pos.XPosition = 0;
    Pos.YPosition = 0;

    Pos.XCharSize = 8;
    Pos.YCharSize = 16;

    Pos.start = 0;

    Pos.FB_addr = (int *)0xffff800000a00000;                // ==> 0xe0000000 帧缓冲区
	Pos.FB_length = Pos.XResolution * Pos.YResolution * 4;

    int a = 0;
    int b = 0;
    int c = 0;

	for(i = 0 ;i<1440*16;i++)
    {
		*((char *)fb_addr+0)=(char)a;
		*((char *)fb_addr+1)=(char)b;
		*((char *)fb_addr+2)=(char)c;
		*((char *)fb_addr+3)=(char)0x00;
        a += 4;
        b += 8;
        c += 16;
		fb_addr +=1;
	}


    load_TR(8);

	set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();

	memory_management_struct.start_code = (unsigned long)& _text;
	memory_management_struct.end_code   = (unsigned long)& _etext;
	memory_management_struct.end_data   = (unsigned long)& _edata;
	memory_management_struct.end_brk    = (unsigned long)& _end;


    color_printk(RED,BLACK,"+++++++++++++ \n");


    color_printk(RED,BLACK,"+++++++++++++ \n");
    color_printk(RED,BLACK,"+++++++++++++ \n");
    color_printk(RED,BLACK,"+++++++++++++ \n");
    long (*ptr)(void) = task_init;
    color_printk(RED,BLACK,"ptr = %lx \n", ptr);
    color_printk(RED,BLACK,"+++++++++++++ \n");
    color_printk(RED,BLACK,"+++++++++++++ \n");
    color_printk(RED,BLACK,"+++++++++++++ \n");
    color_printk(RED,BLACK,"+++++++++++++ \n");


	color_printk(RED,BLACK,"memory init \n");
	init_memory();

    // while(1);

	color_printk(RED,BLACK,"interrupt init \n");
	init_interrupt();

    while(1);

	color_printk(RED,BLACK,"task_init \n");
	task_init(); // 0xffff80000010b27b

    // i = *(int *)0xffff80000aa00000;

	while(1)
		;


}

















// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0xff;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0xff;
// 		*((char *)fb_addr+2)=0xff;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0xff;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
//
// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0x8f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0xff;
// 		*((char *)fb_addr+2)=0x8f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0x80;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0x4f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0xff;
// 		*((char *)fb_addr+2)=0x4f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0x40;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
//
// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x2f;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x00;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0x20;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
//
// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x10;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x10;
// 		*((char *)fb_addr+1)=0x2f;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
//     {
// 		*((char *)fb_addr+0)=0x10;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0x20;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
//
//
// for (a = 0; a < 3; a++){
// 	for(i = 0 ;i<1440*16;i++)
// 	{
// 		*((char *)fb_addr+0)=0x20;
// 		*((char *)fb_addr+1)=0x80;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
// 	{
// 		*((char *)fb_addr+0)=0x20;
// 		*((char *)fb_addr+1)=0x2f;
// 		*((char *)fb_addr+2)=0x2f;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// 	for(i = 0 ;i<1440*16;i++)
// 	{
// 		*((char *)fb_addr+0)=0x20;
// 		*((char *)fb_addr+1)=0x00;
// 		*((char *)fb_addr+2)=0x20;
// 		*((char *)fb_addr+3)=(char)0x00;
// 		fb_addr +=1;
// 	}
// }
