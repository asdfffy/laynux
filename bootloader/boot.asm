
;|----------------------|
;|	100000 ~ END	    |
;|	   KERNEL	        |
;|----------------------|
;|	F0000 ~ FFFFF	    |     BIOS入口地址位于 FFFFF 此处内容为16字节的跳转指令
;|   系统BIOS 640kb      |    jmp F000:E05B 地址于系统BIOS内部 执行相关初始化工作
;|----------------------|     之后跳转至 07C00 地址处
;|	C8000 ~ EFFFF	    |
;|  映射硬件适配器ROM      |
;|  或内存映射式IO  32kb  |
;|----------------------|
;|	C0000 ~ C7FFF	    |
;|   显示适配器BIOS 32kb  |
;|----------------------|
;|	B0000 ~ B7FFF	    |
;|    黑白显示适配器 32kb  |
;|----------------------|
;|	A0000 ~ AFFFF	    |
;|   彩色显示适配器 64kb  |
;|----------------------|
;|	9FC00 ~ 9FFFF 	    |
;|	 扩展 BIOS数据区 1kb  |
;|----------------------|
;|	90000 ~ 9FBFF	|
;|	 kernel tmpbuf	|
;|----------------------|
;|	10000 ~ 90000	    |
;|	 loader加载区域	      |  
;|----------------------|
;|	8000 ~ 10000	    |
;|	  vbe info	        |
;|----------------------|
;|	7c00 ~ 7DFF         |
;|MBR被加载区域 512B      |  
;|----------------------|
;|	500 ~ 7BFF          |	
;|	 30464B 约 30KB      |
;|----------------------|
;|	400 ~ 4FF	        |
;|	BIOS 数据区	         |
;|----------------------|
;|	000 ~ 3FF	        |
;|	中断向量表            |  
;|----------------------|



; -------------------------重要目录项结构---------------------
; 名称          偏移            长度        描述
; DIR_Name     0x00            11         文件名8B,扩展名3B      
; DIR_Attr     0x0B            1          文件属性
;              ... 
; DIR_FstClus  0x1A            2          起始簇号           
; DIR_FileSize 0x1C            4          文件大小
; ----------------------------------------------------------


org	0x7c00

BaseOfStack	equ	0x7c00
BaseOfLoader	equ	0x1000
OffsetOfLoader	equ	0x00

%include	"fat12.inc"



Label_Start:

;   初始化各段寄存器和堆栈指针
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

;   清屏 
	mov	ax,	0600h
	mov	bx,	0700h
	mov	cx,	0
	mov	dx,	0184fh
	int	10h

;   设置光标位置
	mov	ax,	0200h
	mov	bx,	0000h
	mov	dx,	0000h
	int	10h

;   打印字符串
	mov	ax,	1301h
	mov	bx,	000fh
	mov	dx,	0000h
	mov	cx,	10
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage
	int	10h

;   复位软盘驱动器
	xor	ah,	ah
	xor	dl,	dl
	int	13h

;   在软盘中搜索 loader.bin 文件
	mov	word	[SectorNo],	SectorNumOfRootDirStart     ; 根目录起始扇区号

Lable_Search_In_Root_Dir_Begin:

	cmp	word	[RootDirSizeForLoop],	0               ; 根目录扇区数
	jz	Label_No_LoaderBin
	dec	word	[RootDirSizeForLoop]
	mov	ax,	00h                                         
	mov	es,	ax
	mov	bx,	8000h
	mov	ax,	[SectorNo]
	mov	cl,	1                                           ; 调用软盘读取模块，一次读取1(CL)个扇区至 0:8000 地址处                            
	call	Func_ReadOneSector                          ; AX=起始扇区号 CL=读取扇区长度 ES:BX=目标内存地址
	mov	si,	LoaderFileName                              ; 需要读取的文件名
	mov	di,	8000h                                       
	cld                                                 ; 清方位(DF标志位),控制搜索方向
	mov	dx,	10h                                         ; 每个扇区可容纳目录项数 512/32 = 0x10

Label_Search_For_LoaderBin:

	cmp	dx,	0                                           ; 当一个扇区搜索完
	jz	Label_Goto_Next_Sector_In_Root_Dir                 
	dec	dx                                              
	mov	cx,	11                                          ; 文件名长度

Label_Cmp_FileName:

	cmp	cx,	0                                          
	jz	Label_FileName_Found                            ; 条件成立即找到文件
	dec	cx
	lodsb                                               ; 每次调用将 DS:SI 数据读取至 AL 寄存器,指针行进方向由 FLAGS 寄存器中的 DF 标志位控制
	cmp	al,	byte	[es:di]                             ; 对比二者数据
	jz	Label_Go_On                                     ; 当前数据吻合则继续对比下一个,否则跳转至不同状态处理模块
	jmp	Label_Different

Label_Go_On:

	inc	di                                              ; 匹配 lodsb 的方向
	jmp	Label_Cmp_FileName

Label_Different:                                        ; 0xFFE0 = 1111 1111 1110 0000 || 0x20 = 0010 0000

	and	di,	0ffe0h                                      ; 向下取整. 一个目录项占 32 字节即 0x20, 跳转至此即说明该目录项不匹配向下取整后方便搜索下一个目录项
	add	di,	20h                                         ; 目录项 +1
	mov	si,	LoaderFileName                              ; 重置待搜索文件名指针
	jmp	Label_Search_For_LoaderBin                      ; 搜索下一个目录项

Label_Goto_Next_Sector_In_Root_Dir:                     

	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin

; 显示信息
Label_No_LoaderBin:

	mov	ax,	1301h
	mov	bx,	008ch
	mov	dx,	0100h
	mov	cx,	21
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage
	int	10h
	jmp	$

; 找到了需要的文件
Label_FileName_Found:

	mov	ax,	RootDirSectors                              ; 0xFFE0 = 1111 1111 1110 0000 || 0x001A = 0001 1010
	and	di,	0ffe0h                                      ; 向下取整,指向目录项头位置
	add	di,	01ah                                        ; 指向目录项中该文件的起始簇号
	mov	cx,	word	[es:di]
	push	cx                                          ; "pop ax" 此值下一步传入 Func_GetFATEntry 处理
	add	cx,	ax                                          ; 说实话... 离得有点远
	add	cx,	SectorBalance
	mov	ax,	BaseOfLoader
	mov	es,	ax
	mov	bx,	OffsetOfLoader
	mov	ax,	cx

Label_Go_On_Loading_File:
	push	ax
	push	bx
	mov	ah,	0eh
	mov	al,	'.'
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax                                              ; 对应 Label_FileName_Found 中的 CX
	call	Func_GetFATEntry
	cmp	ax,	0fffh
	jz	Label_File_Loaded
	push	ax
	mov	dx,	RootDirSectors
	add	ax,	dx
	add	ax,	SectorBalance
	add	bx,	[BPB_BytesPerSec]
	jmp	Label_Go_On_Loading_File

Label_File_Loaded:

	jmp	BaseOfLoader:OffsetOfLoader

;   软盘读取模块
;   接受参数 AX=起始扇区号 CL=读取扇区长度 ES:BX=目标内存地址
Func_ReadOneSector:

	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl      ; 手动开辟2字节空间存放 CL参数
	push	bx                      ; 将LBA寻址转换为CHS寻址
	mov	bl,	[BPB_SecPerTrk]         ; 每磁道扇区数
	div	bl                          ; LBA扇区号/每磁道扇区数  AX/BL = AL
	inc	ah                          ;
	mov	cl,	ah                      ; 中断调用参数 起始扇区号磁道号高2位
	mov	dh,	al                      ; 磁头号 AL
	shr	al,	1                       ; 柱面号 = 磁头号>>1        此处原书公式将'>>'印成了'≥'给我看了好久,头疼....
	mov	ch,	al                      ; 中断调用参数 柱面号低8位
	and	dh,	1                       ; 中断调用参数 磁头号       只有两个磁头故'and 1'   
	pop	bx                          ; 
	mov	dl,	[BS_DrvNum]             ; 中断调用参数 int 13H的驱动器号
Label_Go_On_Reading:
	mov	ah,	2                       
	mov	al,	byte	[bp - 2]        ; 读取扇区长度,即传入的 CL 参数
	int	13h
	jc	Label_Go_On_Reading
	add	esp,	2
	pop	bp
	ret

; 以下操作 p50 页介绍的比较详细,解决一个FAT表项占用1.5B的读取问题
; 索引FAT表项
Func_GetFATEntry:                   ; 每个FAT表项占用 12bit 即两个表项占用 3B

	push	es
	push	bx
	push	ax                      
	mov	ax,	00
	mov	es,	ax
	pop	ax                          ; 获得 FAT表项
	mov	byte	[Odd],	0           ; 奇偶标志           
	mov	bx,	3
	mul	bx                          ; AX * BX = [DX:AX] 此处 AX 是FAT表项, Label_FileName_Found push 的 cx
	mov	bx,	2
	div	bx                          ; [DX:AX]/BX = DX 余 AX
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
	and	ax,	0fffh
	pop	bx
	pop	es
	ret


; 其它信息
RootDirSizeForLoop	dw	RootDirSectors              ; define 根目录扇区数, 为啥不直接用呢...
SectorNo		    dw	0
Odd			        db	0

StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0             ; FAT12文件系统只存在大写字符,小写字符只做显示用

; 引导扇区0填充
    
	times	510 - ($ - $$)	db	0                   ; '$'当前机器码地址, '$$'本节程序机器码地址
	dw	0xaa55
