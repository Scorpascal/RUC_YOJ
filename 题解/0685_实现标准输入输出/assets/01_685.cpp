#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define MAX_LEN 4096

//各种数据类型的变量
char c = '\0';
short s = 0;
int i = 0;
long long ll = 0;
float f = 0.0;
double d = 0.0;
char str[MAX_LEN] = "";
void *p = NULL;

//输入、输出缓存区
char in_buf[MAX_LEN], out_buf[MAX_LEN];

//输入、输出缓冲区当前位置
char in_fmt[MAX_LEN], out_fmt[MAX_LEN];

//输入、输出格式控制字符串
int in_idx, out_idx;

//用于自行定义辅助函数，如果没有，可以空缺
____qcodep____

void myscanf(char *fmt, ...)
{
    ____qcodep____
}

void myprintf(char *fmt, ...)
{
    ____qcodep____
}

int main()
{   //main函数的代码都不需要管，只需实现myscanf和myprintf函数即可。
    int n, m;
    scanf("%d.%d\n", &n, &m);
    if (n == 1)
    {
        if (m == 1)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &i);
            myprintf(out_fmt, i);
            puts(out_buf);
        }
        else if (m == 2)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &c);
            myprintf(out_fmt, c);
            puts(out_buf);
        }
        else if (m == 3)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, str);
            myprintf(out_fmt, str);
            puts(out_buf);
        }
        else if (m == 4)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &f);
            myprintf(out_fmt, f);
            puts(out_buf);
        }
    }
    else if (n == 2)
    {
        if (m == 1)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &i);
            myprintf(out_fmt, i);
            puts(out_buf);
        }
        else if (m == 2)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &f);
            myprintf(out_fmt, f);
            puts(out_buf);
        }
        else if (m == 3)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, str);
            myprintf(out_fmt, str);
            puts(out_buf);
        }
    }
    else if (n == 3)
    {
        if (m == 1)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, str, &i, &c);
            myprintf(out_fmt, str, i, c);
            puts(out_buf);
        }
        else if (m == 2)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &i, &c);
            myprintf(out_fmt, i, c);
            puts(out_buf);
        }
    }
    else
    {
        if (m == 1)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, str, &i, &c);
            myprintf(out_fmt, str, i, c);
            puts(out_buf);
        }
        else if (m == 2)
        {
            gets(in_fmt);
            gets(out_fmt);
            gets(in_buf);
            myscanf(in_fmt, &f);
            myprintf(out_fmt, f);
            puts(out_buf);
        }
    }
    return 0;
}