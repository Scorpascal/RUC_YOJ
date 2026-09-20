#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 2005

// 反转字符串
void reverse(char *s) {
    int n = strlen(s);
    for (int i = 0; i < n / 2; ++i) {
        char t = s[i];
        s[i] = s[n - 1 - i];
        s[n - 1 - i] = t;
    }
}

// 字符串加法，结果存到res
void add(char *a, char *b, char *res) {
    int la = strlen(a), lb = strlen(b);
    int lmax = la > lb ? la : lb;
    int carry = 0, i;
    for (i = 0; i < lmax || carry; ++i) {
        int x = i < la ? a[i] - '0' : 0;
        int y = i < lb ? b[i] - '0' : 0;
        int sum = x + y + carry;
        res[i] = sum % 10 + '0';
        carry = sum / 10;
    }
    res[i] = '\0';
}

// 字符串减法，a >= b，结果存到res
void sub(char *a, char *b, char *res) {
    int la = strlen(a), lb = strlen(b);
    int carry = 0, i;
    for (i = 0; i < la; ++i) {
        int x = a[i] - '0';
        int y = i < lb ? b[i] - '0' : 0;
        int diff = x - y - carry;
        if (diff < 0) {
            diff += 10;
            carry = 1;
        } else {
            carry = 0;
        }
        res[i] = diff + '0';
    }
    // 去除前导0
    while (i > 1 && res[i - 1] == '0') --i;
    res[i] = '\0';
}

// 乘以10^k（即字符串左移k位），结果存到res
void shift(char *a, int k, char *res) {
    int la = strlen(a);
    for (int i = 0; i < k; ++i) res[i] = '0';
    for (int i = 0; i < la; ++i) res[i + k] = a[i];
    res[la + k] = '\0';
}

// 传统乘法
void multiply_basic(char *a, char *b, char *res) {
    int la = strlen(a), lb = strlen(b);
    int temp[MAXN] = {0};
    for (int i = 0; i < la; ++i)
        for (int j = 0; j < lb; ++j)
            temp[i + j] += (a[i] - '0') * (b[j] - '0');
    int len = la + lb, carry = 0;
    for (int i = 0; i < len; ++i) {
        temp[i] += carry;
        carry = temp[i] / 10;
        temp[i] %= 10;
    }
    while (len > 1 && temp[len - 1] == 0) --len;
    for (int i = 0; i < len; ++i) res[i] = temp[i] + '0';
    res[len] = '\0';
}

// Karatsuba 乘法
void karatsuba(char *a, char *b, char *res) {
    int la = strlen(a), lb = strlen(b);
    if (la < lb) { karatsuba(b, a, res); return; }
    if (la == 0 || lb == 0) { res[0] = '0'; res[1] = '\0'; return; }
    if (la <= 32 || lb <= 32) { multiply_basic(a, b, res); return; }

    int m = la / 2;
    char a0[MAXN], a1[MAXN], b0[MAXN], b1[MAXN];
    strncpy(a0, a, m); a0[m] = '\0';
    strcpy(a1, a + m);
    int lb0 = lb < m ? lb : m;
    strncpy(b0, b, lb0); b0[lb0] = '\0';
    strcpy(b1, b + lb0);

    char z0[MAXN] = {0}, z1[MAXN] = {0}, z2[MAXN] = {0};
    char t1[MAXN] = {0}, t2[MAXN] = {0};

    karatsuba(a1, b1, z2);
    karatsuba(a0, b0, z0);

    add(a0, a1, t1);
    add(b0, b1, t2);
    karatsuba(t1, t2, z1);

    sub(z1, z2, z1);
    sub(z1, z0, z1);

    char z2s[MAXN] = {0}, z1s[MAXN] = {0};
    shift(z2, 2 * m, z2s);
    shift(z1, m, z1s);

    char temp[MAXN] = {0};
    add(z0, z1s, temp);
    add(temp, z2s, res);
}

// 去除前导0
void strip_leading_zeros(char *s) {
    int n = strlen(s);
    int i = n - 1;
    while (i > 0 && s[i] == '0') --i;
    s[i + 1] = '\0';
}

int main() {
    char sa[MAXN], sb[MAXN];
    scanf("%s %s", sa, sb);
    reverse(sa);
    reverse(sb);
    char res[MAXN * 2] = {0};
    karatsuba(sa, sb, res);
    strip_leading_zeros(res);
    reverse(res);
    printf("%s\n", res);
    return 0;
}