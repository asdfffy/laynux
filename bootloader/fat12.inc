RootDirSectors	equ	14                  ; 根目录扇区数
SectorNumOfRootDirStart	equ	19          ; 根目录起始扇区号
SectorNumOfFAT1Start	equ	1           ; FAT表起始扇区号
SectorBalance	equ	17

    jmp	short Label_Start               ; 跳转语句，该位置后为非可执行程序
	nop                                 ; ### 作字节填充用不可省略！！！ ###
	BS_OEMName	db	'MINEboot'          ; 生产厂商名称
	BPB_BytesPerSec	dw	512             ; 每扇区字节数    
	BPB_SecPerClus	db	1               ; 每簇扇区数数值2^n，FAT12最小数据单元
	BPB_RsvdSecCnt	dw	1               ; 保留扇区数
	BPB_NumFATs	db	2                   ; FAT表数
	BPB_RootEntCnt	dw	224             ; 根目录可容纳目录项数量，该数值*32必须为 BPB_BytesPerSec 整数倍
	BPB_TotSec16	dw	2880            ; 总扇区数
	BPB_Media	db	0xf0                ; 介质描述符，该字段许需写入FAT[0]低字节处
	BPB_FATSz16	dw	9                   ; FAT扇区数
	BPB_SecPerTrk	dw	18              ; 每磁道扇区数
	BPB_NumHeads	dw	2               ; 磁头数
	BPB_HiddSec	dd	0                   ; 隐藏扇区数
	BPB_TotSec32	dd	0               ; 当 BPB_TotSec 16为0时此字段记录总扇区数
	BS_DrvNum	db	0                   ; int 13H的驱动器号
	BS_Reserved1	db	0               ; 保留字段
	BS_BootSig	db	0x29                ; 扩展引导标记
	BS_VolID	dd	0                   ; 卷序列号
	BS_VolLab	db	'boot loader'       ; 卷标
	BS_FileSysType	db	'FAT12   '      ; 文件系统类型字符串，只做说明使用不作为文件系统识别标准
