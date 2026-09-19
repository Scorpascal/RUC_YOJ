#include <stdio.h>

void print_expr(int n); // 打印 n 的表达式形式（用于括号内部和整体）

// 打印一个项：对应 2 的 i 次方
void print_term(int i) {
    if (i == 0) {
        printf("2(0)");
    } else if (i == 1) {
        printf("2");
    } else if (i == 2) {
        printf("2(2)");
    } else {
        printf("2(");
        print_expr(i);
        printf(")");
    }
}

// 打印 n 的表达式：按从高位到低位的置位输出项，使用 '+' 连接
void print_expr(int n) {
    int first = 1;
    for (int i = 31; i >= 0; --i) {
        if ((n >> i) & 1) {
            if (!first) printf("+");
            print_term(i);
            first = 0;
        }
    }
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    print_expr(n);
    return 0;
}