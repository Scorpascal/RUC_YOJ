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
#include <ctype.h>
#include <stdlib.h>
int skip_spaces()
{
    while (in_buf[in_idx] && isspace((unsigned char)in_buf[in_idx]))
        in_idx++;
    return in_idx;
}

void myscanf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    in_idx = 0;
    int cnt = 0;

    for (int fi = 0; fmt[fi]; ++fi)
    {
        if (isspace((unsigned char)fmt[fi]))
        {
            skip_spaces();
            continue;
        }
        if (fmt[fi] != '%')
        {
            if (in_buf[in_idx] == fmt[fi])
                in_idx++;
            continue;
        }

        ++fi;
        int width = 0;
        while (fmt[fi] && isdigit((unsigned char)fmt[fi]))
        {
            width = width * 10 + (fmt[fi] - '0');
            ++fi;
        }
        char spec = fmt[fi];
        if (spec == 'l' && fmt[fi + 1])
        {
            spec = fmt[++fi]; // handle %lf
        }

        if (spec != 'c') skip_spaces();

        if (spec == 'd')
        {
            char buf[128];
            int k = 0;
            if (in_buf[in_idx] == '+' || in_buf[in_idx] == '-')
            {
                buf[k++] = in_buf[in_idx++];
                if (width > 0) width--;
            }
            while (in_buf[in_idx] && isdigit((unsigned char)in_buf[in_idx]) && (width == 0 || k < width + (buf[0] == '+' || buf[0] == '-')))
            {
                buf[k++] = in_buf[in_idx++];
            }
            buf[k] = '\0';
            if (k > 0 && !(k == 1 && (buf[0] == '+' || buf[0] == '-')))
            {
                long v = strtol(buf, NULL, 10);
                *(va_arg(ap, int *)) = (int)v;
                cnt++;
            }
        }
        else if (spec == 'f' || spec == 'e' || spec == 'E')
        {
            char buf[256];
            int k = 0;
            while (in_buf[in_idx] &&
                   !isspace((unsigned char)in_buf[in_idx]) &&
                   (width == 0 || k < width))
            {
                buf[k++] = in_buf[in_idx++];
            }
            buf[k] = '\0';
            if (k > 0)
            {
                double v = strtod(buf, NULL);
                *(va_arg(ap, float *)) = (float)v;
                cnt++;
            }
        }
        else if (spec == 'c')
        {
            int w = (width > 0) ? width : 1;
            char *dst = va_arg(ap, char *);
            for (int t = 0; t < w && in_buf[in_idx]; ++t)
                dst[t] = in_buf[in_idx++];
            if (w == 1) cnt++;
        }
        else if (spec == 's')
        {
            char *dst = va_arg(ap, char *);
            int k = 0;
            while (in_buf[in_idx] && !isspace((unsigned char)in_buf[in_idx]) &&
                   (width == 0 || k < width))
            {
                dst[k++] = in_buf[in_idx++];
            }
            dst[k] = '\0';
            cnt++;
        }
    }

    va_end(ap);
}

void myprintf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    out_idx = 0;
    out_buf[0] = '\0';

    for (int fi = 0; fmt[fi]; ++fi)
    {
        if (fmt[fi] == '\\')
        {
            char esc = fmt[fi + 1];
            if (esc == 'n') { out_buf[out_idx++] = '\n'; fi++; continue; }
            else if (esc == 't') { out_buf[out_idx++] = '\t'; fi++; continue; }
            else if (esc == '\\') { out_buf[out_idx++] = '\\'; fi++; continue; }
        }

        if (fmt[fi] != '%')
        {
            out_buf[out_idx++] = fmt[fi];
            continue;
        }

        int start = fi;
        ++fi;
        while (fmt[fi] && strchr("cdisfFeEgG", fmt[fi]) == NULL)
        {
            fi++;
        }
        char spec = fmt[fi];
        char subfmt[64] = {0};
        int len = fi - start + 1;
        strncpy(subfmt, fmt + start, len);
        subfmt[len] = '\0';

        char tmp[256] = {0};
        if (spec == 'd')
        {
            int v = va_arg(ap, int);
            snprintf(tmp, sizeof(tmp), subfmt, v);
        }
        else if (spec == 'c')
        {
            int v = va_arg(ap, int);
            snprintf(tmp, sizeof(tmp), subfmt, v);
        }
        else if (spec == 's')
        {
            char *v = va_arg(ap, char *);
            snprintf(tmp, sizeof(tmp), subfmt, v);
        }
        else // float / double formats
        {
            double v = va_arg(ap, double);
            snprintf(tmp, sizeof(tmp), subfmt, v);

            if (spec == 'e' || spec == 'E')
            {
                char *p = strchr(tmp, spec);
                if (p && *(p + 1))
                {
                    char sign = *(p + 1); // '+' or '-'
                    char *expdigits = p + 2;
                    int dlen = (int)strlen(expdigits);
                    if (dlen > 0 && dlen < 3)
                    {
                        char fixed[256];
                        int need = 3 - dlen;
                        char signbuf[2] = {sign, '\0'};
                        snprintf(fixed, sizeof(fixed), "%.*s%c%s%0*d",
                                 (int)(p - tmp), tmp, spec, signbuf, need, 0);
                        strncat(fixed, expdigits, sizeof(fixed) - strlen(fixed) - 1);
                        strncpy(tmp, fixed, sizeof(tmp) - 1);
                        tmp[sizeof(tmp) - 1] = '\0';
                    }
                }
            }
        }

        size_t tlen = strlen(tmp);
        if (out_idx + (int)tlen < MAX_LEN)
        {
            memcpy(out_buf + out_idx, tmp, tlen);
            out_idx += (int)tlen;
        }
    }

    out_buf[out_idx] = '\0';
    va_end(ap);
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