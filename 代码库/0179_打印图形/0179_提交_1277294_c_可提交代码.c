#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    // 顶边
    for (int i = 0; i < n - 1; ++i) putchar(' ');
    for (int i = 0; i < n; ++i) putchar('*');
    putchar('\n');

    // 上斜边部分
    for (int i = 1; i <= n - 1; ++i) {
        int lead = n - 1 - i;
        for (int j = 0; j < lead; ++j) putchar(' ');
        putchar('*');
        int inner = n + 2 * (i - 1);
        for (int j = 0; j < inner; ++j) putchar(' ');
        putchar('*');
        putchar('\n');
    }

    // 下斜边部分（对称）
    for (int i = n - 2; i >= 1; --i) {
        int lead = n - 1 - i;
        for (int j = 0; j < lead; ++j) putchar(' ');
        putchar('*');
        int inner = n + 2 * (i - 1);
        for (int j = 0; j < inner; ++j) putchar(' ');
        putchar('*');
        putchar('\n');
    }

    // 底边
    for (int i = 0; i < n - 1; ++i) putchar(' ');
    for (int i = 0; i < n; ++i) putchar('*');
    putchar('\n');

    return 0;
}