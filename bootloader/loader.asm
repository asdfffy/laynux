org	10000h
	jmp	Label_Start

%include	"fat12.inc"

BaseOfKernelFile	equ	0x00
OffsetOfKernelFile	equ	0x100000

BaseTmpOfKernelAddr	equ	0x00
OffsetTmpOfKernelFile	equ	0x7E00

MemoryStructBufferAddr	equ	0x7E00

[SECTION gdt]                                               ; 32位GDT表结构

LABEL_GDT:		dd	0,0                                     ; 第一项必须为空
LABEL_DESC_CODE32:	dd	0x0000FFFF,0x00CF9A00               ;
LABEL_DESC_DATA32:	dd	0x0000FFFF,0x00CF9200               ;

GdtLen	equ	$ - LABEL_GDT
GdtPtr	dw	GdtLen - 1
	    dd	LABEL_GDT	                                    ;be carefull the address(after use org)!!!!!!

SelectorCode32	equ	LABEL_DESC_CODE32 - LABEL_GDT
SelectorData32	equ	LABEL_DESC_DATA32 - LABEL_GDT

[SECTION gdt64]                                             ; 64位GDT表结构

LABEL_GDT64:		dq	0x0000000000000000
LABEL_DESC_CODE64:	dq	0x0020980000000000
LABEL_DESC_DATA64:	dq	0x0000920000000000

GdtLen64	equ	$ - LABEL_GDT64
GdtPtr64	dw	GdtLen64 - 1
		dd	LABEL_GDT64

SelectorCode64	equ	LABEL_DESC_CODE64 - LABEL_GDT64
SelectorData64	equ	LABEL_DESC_DATA64 - LABEL_GDT64

[SECTION .s16]
[BITS 16]                                                   ; 通知编译器以下代码运行在16位环境

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

;   显示信息
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0200h		;row 2
	mov	cx,	12
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage
	int	10h



;   有多种开启A20地址线方法
;   1. 操作键盘控制器
;   2. A20快速门(Fast Gate A20), 置位I/O端口 0x92 第一位实现开启
;   3. BIOS中断服务程序 INT 15H,AX 2401开启,2400禁止,2403查询
;   4. 读 0xEE 端口开启, 写 0xEE 禁止

;   开启A20地址线,以访问1M以上地址空间
	push	ax
	in	al,	92h
	or	al,	00000010b                   ; 置位 0x92 第一位开启A20地址线
	out	92h,	al
	pop	ax

	cli

	lgdt	[GdtPtr]                    ; 加载GDTR

	mov	eax,	cr0                     ;
	or	eax,	1
	mov	cr0,	eax

	mov	ax,	SelectorData32
	mov	fs,	ax
	mov	eax,	cr0
	and	al,	11111110b
	mov	cr0,	eax

	sti

;   复位软盘驱动器
	xor	ah,	ah
	xor	dl,	dl
	int	13h

; 搜索 kernel.bin 文件, 与boot中搜索 loader.bin 的结构相似
; ==============================================================
	mov	word	[SectorNo],	SectorNumOfRootDirStart

Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]
	mov	ax,	00h
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1
	call	Func_ReadOneSector
	mov	si,	KernelFileName
	mov	di,	8000h
	cld
	mov	dx,	10h

Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11

Label_Cmp_FileName:

	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb
	cmp	al,	byte	[es:di]
	jz	Label_Go_On
	jmp	Label_Different

Label_Go_On:

	inc	di
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0FFE0h
	add	di,	20h
	mov	si,	KernelFileName
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:

	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin

Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0300h		            ;row 3
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

Label_FileName_Found:
	mov	ax,	RootDirSectors
	and	di,	0FFE0h
	add	di,	01Ah
	mov	cx,	word	[es:di]
	push	cx
	add	cx,	ax
	add	cx,	SectorBalance
	mov	eax,	BaseTmpOfKernelAddr ;BaseOfKernelFile
	mov	es,	eax
	mov	bx,	OffsetTmpOfKernelFile   ;OffsetOfKernelFile
	mov	ax,	cx

Label_Go_On_Loading_File:                                   
	push	ax
	push	bx
	mov	ah,	0Eh
	mov	al,	'.'
	mov	bl,	0Fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
;================================= 以上结构与 boot 中的相似,与 boot 中略有不同的是此处将读取的数据储存至中转区后再拷贝至内核区
	push	cx
	push	eax
	push	fs
	push	edi
	push	ds
	push	esi

	mov	cx,	200h
	mov	ax,	BaseOfKernelFile                                ; 内核地址基址
	mov	fs,	ax                          ; <<========== 此操作在物理环境下会使 fs 寻址范围重新回到1M
	mov	edi,	dword	[OffsetOfKernelFileCount]           ; 内核地址偏移,即OffsetOfKernelFile        

	mov	ax,	BaseTmpOfKernelAddr                             ; 内核中转地址基址
	mov	ds,	ax
	mov	esi,	OffsetTmpOfKernelFile                       ; 内核中转地址偏移


Label_Mov_Kernel:	

	mov	al,	byte	[ds:esi]                                ; 0:7E00
	mov	byte	[fs:edi],	al                              ; 0:100000

	inc	esi
	inc	edi

	loop	Label_Mov_Kernel                                ; 循环拷贝 0x200 字节数据

	mov	eax,	0x1000
	mov	ds,	eax

	mov	dword	[OffsetOfKernelFileCount],	edi

	pop	esi
	pop	ds
	pop	edi
	pop	fs
	pop	eax
	pop	cx

	call	Func_GetFATEntry                                ; 内核过长会涉及到的文件簇处理
	cmp	ax,	0FFFh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
;	add	bx,	[BPB_BytesPerSec]

	jmp	Label_Go_On_Loading_File

Label_File_Loaded:                                          ; 内核加载完成

	mov	ax, 0B800h
	mov	gs, ax
	mov	ah, 0Fh				                                ; 0000: 黑底    1111: 白字
	mov	al, 'G'
	mov	[gs:((80 * 0 + 39) * 2)], ax	                    ; 屏幕第 0 行, 第 39 列。

KillMotor:                                                  ; 关闭软驱马达

	push	dx
	mov	dx,	03F2h
	mov	al,	0
	out	dx,	al
	pop	dx

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0400h		;row 4
	mov	cx,	44
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetMemStructMessage
	int	10h

	mov	ebx,	0
	mov	ax,	0x00
	mov	es,	ax
	mov	di,	MemoryStructBufferAddr

;   使用 INT 15, E820 中断服务程序获取内存信息
Label_Get_Mem_Struct:

	mov	eax,	0x0E820
	mov	ecx,	20
	mov	edx,	0x534D4150
	int	15h
	jc	Label_Get_Mem_Fail
	add	di,	20
	inc	dword	[MemStructNumber]

	cmp	ebx,	0
	jne	Label_Get_Mem_Struct
	jmp	Label_Get_Mem_OK

Label_Get_Mem_Fail:

	mov	dword	[MemStructNumber],	0

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0500h		;row 5
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructErrMessage
	int	10h

Label_Get_Mem_OK:

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0600h		;row 6
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetMemStructOKMessage
	int	10h

;   以下进行SVGA显示模式处理
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0800h		;row 8
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAVBEInfoMessage
	int	10h

	mov	ax,	0x00
	mov	es,	ax
	mov	di,	0x8000
	mov	ax,	4F00h

	int	10h

	cmp	ax,	004Fh

	jz	.KO

;   错误信息
	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0900h		;row 9
	mov	cx,	23
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoErrMessage
	int	10h

	jmp	$

.KO:

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0A00h		;row 10
	mov	cx,	29
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAVBEInfoOKMessage
	int	10h

;   获取SVGA信息
	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0C00h		;row 12
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartGetSVGAModeInfoMessage
	int	10h


	mov	ax,	0x00
	mov	es,	ax
	mov	si,	0x800e

	mov	esi,	dword	[es:si]
	mov	edi,	0x8200

Label_SVGA_Mode_Info_Get:

	mov	cx,	word	[es:esi]

;   打印信息
	push	ax

	mov	ax,	00h
	mov	al,	ch
	call	Label_DispAL

	mov	ax,	00h
	mov	al,	cl
	call	Label_DispAL

	pop	ax

	cmp	cx,	0FFFFh
	jz	Label_SVGA_Mode_Info_Finish

	mov	ax,	4F01h
	int	10h

	cmp	ax,	004Fh

	jnz	Label_SVGA_Mode_Info_FAIL

	inc	dword		[SVGAModeCounter]
	add	esi,	2
	add	edi,	0x100

	jmp	Label_SVGA_Mode_Info_Get

Label_SVGA_Mode_Info_FAIL:

	mov	ax,	1301h
	mov	bx,	008Ch
	mov	dx,	0D00h		;row 13
	mov	cx,	24
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoErrMessage
	int	10h

Label_SET_SVGA_Mode_VESA_VBE_FAIL:

	jmp	$

Label_SVGA_Mode_Info_Finish:

	mov	ax,	1301h
	mov	bx,	000Fh
	mov	dx,	0E00h		;row 14
	mov	cx,	30
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	GetSVGAModeInfoOKMessage
	int	10h

;   设置 SVGA 显示模式

	mov	ax,	4F02h
	mov	bx,	4180h	                ; 显示模式号, 1440*900 分辨率
	int 	10h

	cmp	ax,	004Fh
	jnz	Label_SET_SVGA_Mode_VESA_VBE_FAIL







;==================================================================
;                            保护模式切换
;   1. 创建一个拥有代码段描述符和数据段描述符的GDT表项,并将其加载至 GDTR 寄存器.
;      若使用到 LDT,TSS 等结构必须在GDT中创建相应表项,
;   2. 创建 IDT 表,初始化相应表项,中断门描述符和陷阱门描述符只需指向相应的处理程序.
;      任务门描述符需要准备相应的 TSS 段描述符,额外的代码,数据和任务段描述符了.
;      如果处理器允许接受外部请求则需要为其建立对应描述符.
;      切换前需将 IDT 表加载至 IDTR 寄存器.
;      如果执行过程中不会触发中断,则以上过程可以省略.
;   3. 分页机制由 CR0 中 PG 标志位控制,若启用分页机制,则需创建至少一个页目录项
;      和页表项,目录项和页表项不能使用同一物理页, 4M页表可以之创建一个页目录项.
;      将目录页物理地址加载至 CR3(PDBR) 寄存器,至此可以同时置位CR0中PE,PG标志位 
;
;   PS:如果需要使用多任务机制或允许改变特权级,则需在首次任务切换前建立至少一个
;      任务状态段 TSS 和 TSS段描述符 在使用 TSS 段结构前必须使用 LTR 将其加载
;      至 TR 寄存器,此行为必须在进入保护模式后执行,此表也必须保存在 GDT 中.
;===================================================================  

	cli			                                    ; 关中断,故暂时无需完整初始化IDT

    db	    66h                                     ; 修饰当前指令操作数是32位宽,开始少加了这个但也没出错
                                                    ; 不知道是编译器做了什么还是给自己埋了个坑,小心为妙

	lgdt	[GdtPtr]                                ; 加载 GDT 表至 GDTR

;   db	    66h
;	lidt	[IDT_POINTER]                           ; 加载 IDT 表至 IDTR

	mov	eax,	cr0                                 ; 置位 PE 标志位开启保护模式
	or	eax,	1
	mov	cr0,	eax

	jmp	dword SelectorCode32:GO_TO_TMP_Protect

[SECTION .s32]
[BITS 32]                                           ; 通知编译器以下代码运行在32位环境

GO_TO_TMP_Protect:

;====================================================================
;                           长模式切换
;   1. 类似保护模式的初始化过程.
;   2. 重新加载各描述符表寄存器
;   3. 重新加载 IDTR 保证不能触发中断,否则系统会把32位描述符表当64位表解析
; 
;   切换流程:
;   1. 保护模式下复位 CR0 PG标志位关闭分页机制.
;   2. 置位 CR4 PAE 标志位,开启物理地址扩展
;   3. 加载页目录(PML4)基地址至 CR3
;   4. 置位 IA32_EFER LME标志位开启 IA-32 模式
;   5. 置位 CR0 PG标志位开分页
;====================================================================

	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	ss,	ax
	mov	esp,	7E00h

	call	support_long_mode
	test	eax,	eax

	jz	no_support

;   临时页目录页表项配置,页目录地址 0x90000
	mov	dword	[0x90000],	0x91007
	mov	dword	[0x90800],	0x91007

	mov	dword	[0x91000],	0x92007

	mov	dword	[0x92000],	0x000083

	mov	dword	[0x92008],	0x200083

	mov	dword	[0x92010],	0x400083

	mov	dword	[0x92018],	0x600083

	mov	dword	[0x92020],	0x800083

	mov	dword	[0x92028],	0xa00083

;   加载 GDTR 初始化段寄存器
	lgdt	[GdtPtr64]
	mov	ax,	0x10
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax

	mov	esp,	7E00h

;   置位PAE

	mov	eax,	cr4
	bts	eax,	5
	mov	cr4,	eax

;   加载 CR3

	mov	eax,	0x90000
	mov	cr3,	eax

;   开启长模式
;   IA32_EFER 位于 MSR 寄存器组内,第八位为 LME 标志位
;   操作 IA32_EFER 寄存器需要使用 (RD/WR)MSR 指令

	mov	ecx,	0C0000080h		; 使用 MSR指令 需要向 ECX 传入寄存器地址 详见 intel 手册第四卷
	rdmsr

	bts	eax,	8
	wrmsr

;   开启分页机制,完成长模式切换

	mov	eax,	cr0
	bts	eax,	0
	bts	eax,	31
	mov	cr0,	eax

	jmp	SelectorCode64:OffsetOfKernelFile

;   长模式支持检测
support_long_mode:

	mov	eax,	0x80000000
	cpuid
	cmp	eax,	0x80000001
	setnb	al
	jb	support_long_mode_done
	mov	eax,	0x80000001
	cpuid
	bt	edx,	29
	setc	al
support_long_mode_done:

	movzx	eax,	al
	ret

; no support
no_support:
	jmp	$

; 软盘读取相关和 boot 中相同
; =============================================================
; =============================================================
[SECTION .s116]
[BITS 16]

Func_ReadOneSector:

	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl
	push	bx
	mov	bl,	[BPB_SecPerTrk]
	div	bl
	inc	ah
	mov	cl,	ah
	mov	dh,	al
	shr	al,	1
	mov	ch,	al
	and	dh,	1
	pop	bx
	mov	dl,	[BS_DrvNum]
Label_Go_On_Reading:
	mov	ah,	2
	mov	al,	byte	[bp - 2]
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

; FAT
Func_GetFATEntry:

	push	es
	push	bx
	push	ax
	mov	ax,	00
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0
	mov	bx,	3
	mul	bx
	mov	bx,	2
	div	bx
	cmp	dx,	0
	jz	Label_Even
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start
	mov	cl,	2
	call	Func_ReadOneSector

	pop	dx
	add	bx,	dx
	mov	ax,	[es:bx]
	cmp	byte	[Odd],	1
	jnz	Label_Even_2
	shr	ax,	4

Label_Even_2:
	and	ax,	0FFFh
	pop	bx
	pop	es
	ret

; =============================================================
; =============================================================

; 打印相关
Label_DispAL:

	push	ecx
	push	edx
	push	edi

	mov	edi,	[DisplayPosition]
	mov	ah,	0Fh
	mov	dl,	al
	shr	al,	4
	mov	ecx,	2
.begin:

	and	al,	0Fh
	cmp	al,	9
	ja	.1
	add	al,	'0'
	jmp	.2
.1:

	sub	al,	0Ah
	add	al,	'A'
.2:

	mov	[gs:edi],	ax
	add	edi,	2

	mov	al,	dl
	loop	.begin

	mov	[DisplayPosition],	edi

	pop	edi
	pop	edx
	pop	ecx

	ret


; IDT表初始化, 填充了一堆0
IDT:
	times	0x50	dq	0
IDT_END:

IDT_POINTER:                            ; IDTR加载数据
		dw	IDT_END - IDT - 1
		dd	IDT

; 临时数据
RootDirSizeForLoop	dw	RootDirSectors
SectorNo		dw	0
Odd			db	0
OffsetOfKernelFileCount	dd	OffsetOfKernelFile
MemStructNumber		dd	0
SVGAModeCounter		dd	0
DisplayPosition		dd	0

; 显示信息
StartLoaderMessage:	db	"Start Loader"
NoLoaderMessage:	db	"ERROR:No KERNEL Found"
KernelFileName:		db	"KERNEL  BIN",0
StartGetMemStructMessage:	db	"Start Get Memory Struct (address,size,type)."
GetMemStructErrMessage:	db	"Get Memory Struct ERROR"
GetMemStructOKMessage:	db	"Get Memory Struct SUCCESSFUL!"

StartGetSVGAVBEInfoMessage:	db	"Start Get SVGA VBE Info"
GetSVGAVBEInfoErrMessage:	db	"Get SVGA VBE Info ERROR"
GetSVGAVBEInfoOKMessage:	db	"Get SVGA VBE Info SUCCESSFUL!"

StartGetSVGAModeInfoMessage:	db	"Start Get SVGA Mode Info"
GetSVGAModeInfoErrMessage:	db	"Get SVGA Mode Info ERROR"
GetSVGAModeInfoOKMessage:	db	"Get SVGA Mode Info SUCCESSFUL!"
