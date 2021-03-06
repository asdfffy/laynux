#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "linkage.h"

/*
 * frame_buffer 0xffff800001a00000;
 * 函数命名有问题有问题有问题
 * color_printk 流程：
 * vsprint：处理待打印数据至缓冲区 buf
 * update_SB：将待打印数据更新至屏幕缓冲
 * refresh_FB：刷新帧缓冲
 */


// 打印字符
void putchar(unsigned int * fb, int Xsize, int x, int y, unsigned int FRcolor,
        unsigned int BKcolor, unsigned char font)
{
    int i = 0;
    int j = 0;
    unsigned int * addr = NULL;
    unsigned char * fontp = NULL;
    int testval = 0;
    fontp = font_ascii[font];

    // 此处使用使用位操作与字库进行对比实现字符输出
    for (i = 0; i < 16; i++)
    {
        addr = fb + Xsize * ( y + i ) + x;
        testval = 0x100;
        for (j = 0; j < 8; j++)
        {
            testval = testval >> 1;
            if (*fontp & testval)
                *addr = FRcolor;            // 结果为真,在该像素地址打印前景色
            else
                *addr = BKcolor;            // 结果为假,在该像素地址打印背景色
            addr++;
        }
        fontp++;
    }
}

// 暂时只对换行符做处理
void update_SB(int buf_length, unsigned int FRcolor, unsigned int BKcolor)
{
    int i;
    for (i = 0; i < buf_length; i++){
        if(Pos.start > SCREEN_CHAR_HEIGHT && Pos.YPosition > SCREEN_CHAR_HEIGHT){
            Pos.start %= SCREEN_CHAR_HEIGHT;
            Pos.YPosition %= SCREEN_CHAR_HEIGHT;
        }
        if(Pos.YPosition - Pos.start >= Pos.YResolution/Pos.YCharSize)
            Pos.start++;

        if((Pos.YPosition + 1) % SCREEN_CHAR_HEIGHT == 0)
            memset(frame_buffer, 0, CLR_LENGTH / 2);
        if((Pos.YPosition + 1) % SCREEN_CHAR_HEIGHT == SCREEN_CHAR_HEIGHT / 2)
            memset(frame_buffer + CLR_LENGTH / 8, 0, CLR_LENGTH / 2);

        // 发现换行符将光标行数 +1
        if(buf[i] != '\n'){
            if(Pos.XPosition <= Pos.XResolution/Pos.XCharSize){
                putchar(frame_buffer,Pos.XResolution,Pos.XPosition++ * Pos.XCharSize,
                    (Pos.YPosition % SCREEN_CHAR_HEIGHT) * Pos.YCharSize,FRcolor,BKcolor,buf[i]);
            }
            else{
                Pos.YPosition++;
                Pos.XPosition = 0;
                putchar(frame_buffer,Pos.XResolution,Pos.XPosition++ * Pos.XCharSize,
                    (Pos.YPosition % SCREEN_CHAR_HEIGHT) * Pos.YCharSize,FRcolor,BKcolor,buf[i]);
            }
        }
        else{
            Pos.YPosition++;
            Pos.XPosition = 0;
        }
    }

}

// 将 start 指向的区域,从 frame_buffer 拷贝至 帧缓冲区
// 23040 为屏幕最下方不完整部部分 只适用 1440*900 分辨率  其它请自行调整
void refresh_FB()   
{               
    // frame_buffer 可看做一个环形数组
    // 当前页面处于缓冲区首位间时
    if(Pos.start * Pos.YCharSize * Pos.XResolution * 4 + Pos.XResolution * Pos.YResolution * 4 <= BUF_LENGTH)
        memcpy(frame_buffer + Pos.start * Pos.YCharSize * Pos.XResolution, Pos.FB_addr, Pos.XResolution * Pos.YResolution * 4 - 23040);
    
    // 当前页面需要回卷时
    else{
        memcpy(frame_buffer + Pos.start * Pos.YCharSize * Pos.XResolution, Pos.FB_addr, BUF_LENGTH - Pos.start * Pos.YCharSize * Pos.XResolution * 4);
        memcpy(frame_buffer, Pos.FB_addr + (BUF_LENGTH / 4 - Pos.start * Pos.YCharSize * Pos.XResolution),
            Pos.XResolution * Pos.YResolution * 4 - (BUF_LENGTH - Pos.start * Pos.YCharSize * Pos.XResolution * 4) - 23040);
    }
}

// 将数值字母转换为整数值
int skip_atoi(const char **s)
{
	int i=0;

	while (is_digit(**s))
		i = i*10 + *((*s)++) - '0';
	return i;
}

// 将整数值按照指定进制规格转换成字符串
static char * number(char * str, long num, int base, int size, int precision,	int type)
{
	char c,sign,tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;
	sign = 0;
	if (type&SIGN && num < 0) {
		sign='-';
		num = -num;
	} else
		sign=(type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	if (sign) size--;
	if (type & SPECIAL)
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num!=0)
		tmp[i++]=digits[do_div(num,base)];
	if (i > precision) precision=i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while(size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL)
		if (base == 8)
			*str++ = '0';
		else if (base==16)
		{
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type & LEFT))
		while(size-- > 0)
			*str++ = c;

	while(i < precision--)
		*str++ = '0';
	while(i-- > 0)
		*str++ = tmp[i];
	while(size-- > 0)
		*str++ = ' ';
	return str;
}


// 格式化字符串,并返回字符串长度
int vsprintf(char * buf, const char * fmt, va_list args)
{
    char * str;                                             // 将 col
    char * s;
    int flags;
    int field_width;
    int precision;
    int i;
    int len;
    int qualifier;

    // 循环处理每一个字符
    for(str = buf; *fmt; fmt++)
	{
        // 不为'%'认定为可显示字符,存入缓冲区
		if(*fmt != '%')
		{
			*str++ = *fmt;
			continue;
		}
		flags = 0;


        /*
         *  按照字符串格式规定,符号 '%' 后面可接 '-' 、 '+' 、 ' ' 、 '#' 、 '0' 等格式符
         *  如果下一个字符是上述格式符,则设置标志变量 flags 的标志位,接着计算数据区域的宽度
         */
		repeat:
			fmt++;
			switch(*fmt)
			{
				case '-':flags |= LEFT;
				goto repeat;
				case '+':flags |= PLUS;
				goto repeat;
				case ' ':flags |= SPACE;
				goto repeat;
				case '#':flags |= SPECIAL;
				goto repeat;
				case '0':flags |= ZEROPAD;
				goto repeat;
			}

			// 提取出后续字符串中的数字,并将其转化为数值以表示数据区域的宽度
			field_width = -1;
			if(is_digit(*fmt))
				field_width = skip_atoi(&fmt);
			else if(*fmt == '*')
			{
				fmt++;
				field_width = va_arg(args, int);
				if(field_width < 0)
				{
					field_width = -field_width;
					flags |= LEFT;
				}
			}

			// 如果数据区域的宽度后面跟有字符 '.' ,说明其后的数值是显示数据的精度
			precision = -1;
			if(*fmt == '.')
			{
				fmt++;
				if(is_digit(*fmt))
					precision = skip_atoi(&fmt);
				else if(*fmt == '*')
				{
					fmt++;
					precision = va_arg(args, int);
				}
				if(precision < 0)
					precision = 0;
			}

            // 检测显示数据的规格,如 %ld 格式化字符串中的字母 'l' 表示显示数据的规格是长整型数
			qualifier = -1;
			if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
			{
				qualifier = *fmt;
				fmt++;
			}

			switch(*fmt)
			{
                // 如果匹配出格式符 c 那么程序将可变参数转换为一个字符,并根据数据区域的宽度和对齐方式填充空格符
				case 'c':

					if(!(flags & LEFT))
						while(--field_width > 0)
							*str++ = ' ';
					*str++ = (unsigned char)va_arg(args, int);
					while(--field_width > 0)
						*str++ = ' ';
					break;

                // 将字符串的长度与显示精度进行比对,根据数据区域的宽度和精度等信息截取待显示字符串的长度并补齐空格符
				case 's':

					s = va_arg(args,char *);
					if(!s)
						s = '\0';
					len = strlen(s);
					if(precision < 0)
						precision = len;
					else if(len > precision)
						len = precision;

					if(!(flags & LEFT))
						while(len < field_width--)
							*str++ = ' ';
					for(i = 0;i < len ;i++)
						*str++ = *s++;
					while(len < field_width--)
						*str++ = ' ';
					break;

                /*
                 *  以下是各种格式化处理, 书p96
                 */


				case 'o':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),8,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),8,field_width,precision,flags);
					break;

				case 'p':

					if(field_width == -1)
					{
						field_width = 2 * sizeof(void *);
						flags |= ZEROPAD;
					}

					str = number(str,(unsigned long)va_arg(args,void *),16,field_width,precision,flags);
					break;

				case 'x':

					flags |= SMALL;

				case 'X':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),16,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),16,field_width,precision,flags);
					break;

				case 'd':
				case 'i':

					flags |= SIGN;
				case 'u':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),10,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),10,field_width,precision,flags);
					break;

				case 'n':

					if(qualifier == 'l')
					{
						long *ip = va_arg(args,long *);
						*ip = (str - buf);
					}
					else
					{
						int *ip = va_arg(args,int *);
						*ip = (str - buf);
					}
					break;

				case '%':

					*str++ = '%';
					break;

				default:

					*str++ = '%';
					if(*fmt)
						*str++ = *fmt;
					else
						fmt--;
					break;
			}

	}
	*str = '\0';
	return str - buf;
}

// 彩色输出
int color_printk(unsigned int FRcolor, unsigned int BKcolor, const char * fmt, ...)
{
    // i 字符串长度
    // count 字符串处理时的计数器
    int i = 0;
    // va:variable-argument (可变参数)
    // 可变参数的一番操作
    va_list args;
    va_start(args, fmt);
    i = vsprintf(buf, fmt, args);
    va_end(args);
    update_SB(i,FRcolor,BKcolor);
    refresh_FB();
    return i;
}
