#include "memory.h"
#include "lib.h"

//初始化目标物理页的 struct page 结构体,并更新目标物理页所在区域空间结构 struct zone 内的统计信息
unsigned long page_init(struct Page* page, unsigned long flags)
{
    // 对于未初始化的页面,初始化页面结构
	if(!page->attribute)
	{
		*(memory_management_struct.bits_map +((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->attribute = flags;
		page->reference_count++;

		page->zone_struct->page_using_count++;
		page->zone_struct->page_free_count--;
		page->zone_struct->total_pages_link++;
	}

    // 如果页面结构属性或参数 flags 中含有引用属性( PG_Referenced )或共享属性( PG_K_Share_To_U )
    // 就只增加 struct page 结构体的引用计数和 struct zone 结构体的页面被引用计数
	else if((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
	{
		page->attribute |= flags;
		page->reference_count++;
		page->zone_struct->total_pages_link++;
	}

    // 否则只添加页表属性,并置位 bit 映射位图的相应位
	else
	{
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->attribute |= flags;
	}
	return 0;
}


// unsigned long page_init(struct Page * page,unsigned long flags)
// {
// 	if(!page->attribute)
// 	{
// 		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
// 		page->attribute = flags;
// 		page->reference_count++;
// 		page->zone_struct->page_using_count++;
// 		page->zone_struct->page_free_count--;
// 		page->zone_struct->total_pages_link++;
// 	}
// 	else if((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
// 	{
// 		page->attribute |= flags;
// 		page->reference_count++;
// 		page->zone_struct->total_pages_link++;
// 	}
// 	else
// 	{
// 		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
// 		page->attribute |= flags;
// 	}
// 	return 0;
// }


unsigned long page_clean(struct Page * page)
{
	if(!page->attribute)
	{
		page->attribute = 0;
	}
	else if((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U))
	{
		page->reference_count--;
		page->zone_struct->total_pages_link--;
		if(!page->reference_count)
		{
			page->attribute = 0;
			page->zone_struct->page_using_count--;
			page->zone_struct->page_free_count++;
		}
	}
	else
	{
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);

		page->attribute = 0;
		page->reference_count = 0;
		page->zone_struct->page_using_count--;
		page->zone_struct->page_free_count++;
		page->zone_struct->total_pages_link--;
	}
	return 0;
}


void init_memory()
{
	int i,j;
	unsigned long TotalMem = 0 ;
	struct E820 *p = NULL;

	color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");

	p = (struct E820*)0xffff800000007e00;

    // 逐条显示内存地址空间的分布信息并初始化 E820 结构体
	for(i = 0; i < 32; i++)
	{
		color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
		if(p->type == 1)
			TotalMem += p->length;

		memory_management_struct.e820[i].address = p->address;
		memory_management_struct.e820[i].length = p->length;
		memory_management_struct.e820[i].type = p->type;
		memory_management_struct.e820_length = i;

		p++;

        // 其 type 值不会大于 4 ,如果出现则是遇见了程序运行时产生的脏数据, 直接跳出该循环
		if(p->type > 4 || p->type < 1 || p->length <= 0)
			break;
	}

	color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);

	TotalMem = 0;

    // 计算系统可用内存
	for(i = 0; i <= memory_management_struct.e820_length; i++)
	{
		unsigned long start, end;
		if(memory_management_struct.e820[i].type != 1) continue;

		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end   = (memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) & PAGE_2M_MASK;

		if(end <= start) continue;
		TotalMem += (end - start) >> PAGE_2M_SHIFT;
	}
	color_printk(ORANGE,BLACK,"OS Can Used Total 2M PAGEs:%#010x=%010d\n",TotalMem,TotalMem);



    // 可用内存总数
	TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address + memory_management_struct.e820[memory_management_struct.e820_length].length;

    // 位图初始化
	memory_management_struct.bits_map = (unsigned long *)(PAGE_4K_ALIGN(memory_management_struct.end_brk));
	memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;
	memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & (~ (sizeof(long) - 1)); // 向上取整并对齐 下同
	memset(memory_management_struct.bits_map, 0xff, memory_management_struct.bits_length);


    // 页表结构初始化
	memory_management_struct.pages_struct = (struct Page *)(PAGE_4K_ALIGN(memory_management_struct.bits_map + memory_management_struct.bits_length));
	memory_management_struct.pages_size = TotalMem >> PAGE_2M_SHIFT;
	memory_management_struct.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) + sizeof(long) - 1) & (~ (sizeof(long) - 1));
	memset(memory_management_struct.pages_struct, 0x00, memory_management_struct.pages_length);

    // zone 结构体数组初始化
	memory_management_struct.zones_struct = (struct Zone *)(PAGE_4K_ALIGN((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_length));
	memory_management_struct.zones_size = 0;
	memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));
	memset(memory_management_struct.zones_struct, 0x00, memory_management_struct.zones_length);




    /*
     * 遍历全部物理内存段信息以初始可用物理内存段。代码首先过滤掉非物理内存段,再将剩下的可用物理内存段进行页对齐
     * 如果本段物理内存有可用物理页,则把该段内存空间视为一个可用的 structzone 区域空间,并对其进行初始化。
     * 同时,还将对这段内存空间中的 struct page 结构体和 bit 位图映射位进行初始化
     */

	for(i = 0;i <= memory_management_struct.e820_length;i++)
	{
		unsigned long start,end;
		struct Zone * z;
		struct Page * p;
		unsigned long * b;
	
		if(memory_management_struct.e820[i].type != 1)
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if(end <= start)
			continue;
	
		//zone 配置
		z = memory_management_struct.zones_struct + memory_management_struct.zones_size;
		memory_management_struct.zones_size++;
	
		z->zone_start_address = start;
		z->zone_end_address = end;
		z->zone_length = end - start;
	
		z->page_using_count = 0;
		z->page_free_count = (end - start) >> PAGE_2M_SHIFT;
	
		z->total_pages_link = 0;
	
		z->attribute = 0;
		z->GMD_struct = &memory_management_struct;
	
		z->pages_length = (end - start) >> PAGE_2M_SHIFT;
		z->pages_group =  (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));
	
		//page 配置
		p = z->pages_group;
		for(j = 0;j < z->pages_length; j++ , p++)
		{
			p->zone_struct = z;
			p->PHY_address = start + PAGE_2M_SIZE * j;
			p->attribute = 0;	
			p->reference_count = 0;
			p->age = 0;

            // 位图配置
			*(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
	
		}
	
	}

	/////////////init address 0 to page struct 0; because the memory_management_struct.e820[0].type != 1

	memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;

	memory_management_struct.pages_struct->PHY_address = 0UL;
	memory_management_struct.pages_struct->attribute = 0;
	memory_management_struct.pages_struct->reference_count = 0;
	memory_management_struct.pages_struct->age = 0;

	/////////////

	memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));

	color_printk(ORANGE,BLACK,"bits_map:%#018lx, bits_size:%#018lx, bits_length:%#018lx\n",memory_management_struct.bits_map,memory_management_struct.bits_size,memory_management_struct.bits_length);

	color_printk(ORANGE,BLACK,"pages_struct:%#018lx, pages_size:%#018lx, pages_length:%#018lx\n",memory_management_struct.pages_struct,memory_management_struct.pages_size,memory_management_struct.pages_length);

	color_printk(ORANGE,BLACK,"zones_struct:%#018lx, zones_size:%#018lx, zones_length:%#018lx\n",memory_management_struct.zones_struct,memory_management_struct.zones_size,memory_management_struct.zones_length);

	ZONE_DMA_INDEX = 0;	//need rewrite in the future
	ZONE_NORMAL_INDEX = 0;	//need rewrite in the future


    // 遍历显示各个区域空间结构体 struct zone 的详细统计信息
	for(i = 0;i < memory_management_struct.zones_size;i++)	//need rewrite in the future
	{
		struct Zone * z = memory_management_struct.zones_struct + i;
		color_printk(ORANGE,BLACK,"zone_start_address:%#018lx, zone_end_address:%#018lx, zone_length:%#018lx, pages_group:%#018lx, pages_length:%#018lx\n",z->zone_start_address,z->zone_end_address,z->zone_length,z->pages_group,z->pages_length);
        
        // 如果条件成立,该区域空间开始的物理内存页未曾经过页表映射
		if(z->zone_start_address == 0x100000000)
			ZONE_UNMAPED_INDEX = i;
	}

    // 在结束地址处预留空间,防止越界
	memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_length + sizeof(long) * 32) & ( ~ (sizeof(long) - 1));	////need a blank to separate memory_management_struct

	color_printk(ORANGE,BLACK,"start_code:%#018lx, end_code:%#018lx, end_data:%#018lx, end_brk:%#018lx, end_of_struct:%#018lx\n",memory_management_struct.start_code,memory_management_struct.end_code,memory_management_struct.end_data,memory_management_struct.end_brk, memory_management_struct.end_of_struct);


	i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;

	for(j = 0;j <= i;j++)
	{   
        // 初始化页表属性 PG_PTable_Maped (经过页表映射的页) | PG_Kernel_Init (内核初始化程序) | PG_Active(使用中的页) | PG_Kernel (内核层页)
		page_init(memory_management_struct.pages_struct + j,PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
	}


	Global_CR3 = Get_gdt();

    // 打印三级页表地址
	color_printk(INDIGO,BLACK,"Global_CR3\t:%#018lx\n",Global_CR3);
	color_printk(INDIGO,BLACK,"*Global_CR3\t:%#018lx\n",*Phy_To_Virt(Global_CR3) & (~0xff));
	color_printk(PURPLE,BLACK,"**Global_CR3\t:%#018lx\n",*Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));


	for(i = 0;i < 10;i++)
		*(Phy_To_Virt(Global_CR3)  + i) = 0UL;

	flush_tlb();
}

/*

	number: number < 64

	zone_select: zone select from dma , mapped in  pagetable , unmapped in pagetable

	page_flags: struct Page flages

*/


struct Page * alloc_pages(int zone_select,int number,unsigned long page_flags)
{
	int i;
	unsigned long page = 0;

	int zone_start = 0;
	int zone_end = 0;

    // Bochs 虚拟机只能开辟出 2GB 的物理内存空间,以至于虚拟平台仅有一个可用物理内存段
    // 故以下三个 case 均代表通一内存空间
	switch(zone_select)
	{
		case ZONE_DMA:
				zone_start = 0;
				zone_end = ZONE_DMA_INDEX;
			break;

		case ZONE_NORMAL:
				zone_start = ZONE_DMA_INDEX;
				zone_end = ZONE_NORMAL_INDEX;
			break;

		case ZONE_UNMAPED:
				zone_start = ZONE_UNMAPED_INDEX;
				zone_end = memory_management_struct.zones_size - 1;
			break;

		default:
			color_printk(RED,BLACK,"alloc_pages error zone_select index\n");
			return NULL;
			break;
	}

    // 目标内存区域空间的起始内存页结构开始逐一遍历
	for(i = zone_start;i <= zone_end; i++)
	{
		struct Zone * z;
		unsigned long j;
		unsigned long start,end,length;
		unsigned long tmp;

		if((memory_management_struct.zones_struct + i)->page_free_count < number)
			continue;

        // 起始内存页结构对应的BIT映射位图往往位于非对齐(unsigned long 类型)位置处
        // 每次按 UNSIGNED LONG 类型作为遍历的步进长度
		z = memory_management_struct.zones_struct + i;
		start = z->zone_start_address >> PAGE_2M_SHIFT;
		end = z->zone_end_address >> PAGE_2M_SHIFT;
		length = z->zone_length >> PAGE_2M_SHIFT;

		tmp = 64 - start % 64;

        
		for(j = start;j <= end;j += j % 64 ? tmp : 64)  // 将索引变量 j 调整到对齐位置处
		{
			unsigned long * p = memory_management_struct.bits_map + (j >> 6);
			unsigned long shift = j % 64;
			unsigned long k;
			for(k = shift;k < 64 - shift;k++)
			{   
                // 为了 alloc_pages 函数可检索出64个连续的物理页
                // if 中语句将后一个 UNSIGNED LONG 变量的低位部分补齐到正在检索的变量中
				if( !(((*p >> k) | (*(p + 1) << (64 - k))) & (number == 64 ? 0xffffffffffffffffUL : ((1UL << number) - 1))) )
				{
					unsigned long	l;
					page = j + k - 1;
					for(l = 0;l < number;l++)
					{
						struct Page * x = memory_management_struct.pages_struct + page + l;
                        
                        // 将BIT映射位图对应的内存页结构 struct page 初始化并返回第一个内存页结构的地址
						page_init(x,page_flags);
					}
					goto find_free_pages;
				}
			}
		}
	}

	return NULL;

find_free_pages:

	return (struct Page *)(memory_management_struct.pages_struct + page);
}
