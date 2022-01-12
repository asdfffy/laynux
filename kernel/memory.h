
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

//	8Bytes per cell
#define PTRS_PER_PAGE	512                                     // 页表项个数

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)         // 内核层的起始线性地址

// 各项对应长度的偏移值
#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

// 
#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

// 向下取整
#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

// 向上取整
#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))


// 页对应位属性

//	bit 63	Execution Disable:
#define PAGE_XD		(unsigned long)0x1000000000000000

//	bit 12	Page Attribute Table
#define	PAGE_PAT	(unsigned long)0x1000

//	bit 8	Global Page:1,global;0,part
#define	PAGE_Global	(unsigned long)0x0100

//	bit 7	Page Size:1,big page;0,small page;
#define	PAGE_PS		(unsigned long)0x0080

//	bit 6	Dirty:1,dirty;0,clean;
#define	PAGE_Dirty	(unsigned long)0x0040

//	bit 5	Accessed:1,visited;0,unvisited;
#define	PAGE_Accessed	(unsigned long)0x0020

//	bit 4	Page Level Cache Disable
#define PAGE_PCD	(unsigned long)0x0010

//	bit 3	Page Level Write Through
#define PAGE_PWT	(unsigned long)0x0008

//	bit 2	User Supervisor:1,user and supervisor;0,supervisor;
#define	PAGE_U_S	(unsigned long)0x0004

//	bit 1	Read Write:1,read and write;0,read;
#define	PAGE_R_W	(unsigned long)0x0002

//	bit 0	Present:1,present;0,no present;
#define	PAGE_Present	(unsigned long)0x0001

//1,0
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

//1,0
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

//7,1,0
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)

//2,1,0
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//7,2,1,0
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

/*

*/

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))


typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))


typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))


typedef struct {unsigned long pt;} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))


unsigned long * Global_CR3 = NULL;


// 内存信息结构体
struct E820
{
	unsigned long address;
	unsigned long length;
	unsigned int	type;
}__attribute__((packed)); // 通知编译器采用紧凑结构,不生成对齐空间


/*

*/

struct Global_Memory_Descriptor
{
	struct E820 	e820[32];               // 物理内存段结构数组
	unsigned long 	e820_length;            // 物理内存段结构数组长度

	unsigned long * bits_map;               // 物理地址空间页映射位图
	unsigned long 	bits_size;              // 物理地址空间页数量
	unsigned long   bits_length;            // 物理地址空间页映射位图长度

	struct Page *	pages_struct;           // 指向全局 struct page 结构体数组的指针
	unsigned long	pages_size;             // struct page 结构体总数
	unsigned long 	pages_length;           // struct page 结构体数组长度

	struct Zone * 	zones_struct;           // 指向全局 struct zone 结构体数组的指针
	unsigned long	zones_size;             // struct zone 结构体数量
	unsigned long 	zones_length;           // struct zone 结构体数组长度

	unsigned long 	start_code , end_code , end_data , end_brk; // 内核对应段起始结束地址

	unsigned long	end_of_struct;          // 内存页管理结构的结尾地址
};

////alloc_pages zone_select

//
#define ZONE_DMA	(1 << 0)

//
#define ZONE_NORMAL	(1 << 1)

//
#define ZONE_UNMAPED	(1 << 2)

////struct page attribute (alloc_pages flags)

//
#define PG_PTable_Maped	(1 << 0)

//
#define PG_Kernel_Init	(1 << 1)

//
#define PG_Referenced	(1 << 2)

//
#define PG_Dirty	(1 << 3)

//
#define PG_Active	(1 << 4)

//
#define PG_Up_To_Date	(1 << 5)

//
#define PG_Device	(1 << 6)

//
#define PG_Kernel	(1 << 7)

//
#define PG_K_Share_To_U	(1 << 8)

//
#define PG_Slab		(1 << 9)

struct Page
{
	struct Zone *	zone_struct;            // 指向本页所属的区域结构体
	unsigned long	PHY_address;            // 页的物理地址
	unsigned long	attribute;              // 页的属性

	unsigned long	reference_count;        // 该页的引用次数

	unsigned long	age;                    // 该页的创建时间
};


//// each zone index

int ZONE_DMA_INDEX	= 0;
int ZONE_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
int ZONE_UNMAPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable

#define MAX_NR_ZONES	10	//max zone

/*

*/

struct Zone
{
	struct Page * 	pages_group;                    // struct page 结构体数组指针
	unsigned long	pages_length;                   // 本区域包含的 struct page 结构体数量

	unsigned long	zone_start_address;             // 本区域的起始页对齐地址
	unsigned long	zone_end_address;               // 本区域的结束页对齐地址
	unsigned long	zone_length;                    // 本区域经过页对齐后的地址长度
	unsigned long	attribute;                      // 本区域空间的属性

	struct Global_Memory_Descriptor * GMD_struct;   // 指向全局结构体 struct Global_Memory_Descriptor

	unsigned long	page_using_count;               // 本区域已使用物理内存页数量
	unsigned long	page_free_count;                // 本区域空闲物理内存页数量

	unsigned long	total_pages_link;               // 本区域物理页被引用次数
};


extern struct Global_Memory_Descriptor memory_management_struct;

unsigned long page_init(struct Page * page,unsigned long flags);

unsigned long page_clean(struct Page * page);

void init_memory();

struct Page * alloc_pages(int zone_select,int number,unsigned long page_flags);

/*

*/

#define	flush_tlb_one(addr)	\
	__asm__ __volatile__	("invlpg	(%0)	\n\t"::"r"(addr):"memory")
/*

*/

// 重新赋值 CR3 使 TLB 刷新,以更新页表项
#define flush_tlb()						\
do								\
{								\
	unsigned long	tmpreg;					\
	__asm__ __volatile__ 	(				\
				"movq	%%cr3,	%0	\n\t"	\
				"movq	%0,	%%cr3	\n\t"	\
				:"=r"(tmpreg)			\
				:				\
				:"memory"			\
				);				\
}while(0)

/*

*/

// 获取 GDT 地址
inline unsigned long * Get_gdt()
{
	unsigned long * tmp;
	__asm__ __volatile__	(
					"movq	%%cr3,	%0	\n\t"
					:"=r"(tmp)
					:
					:"memory"
				);
	return tmp;
}


#endif
