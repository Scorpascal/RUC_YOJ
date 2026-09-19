#include <stdio.h>

int main() {
    int n, m, p;
    if (scanf("%d %d %d", &n, &m, &p) != 3) return 0;

    long long A[105][105], B[105][105], C[105][105];

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            scanf("%lld", &A[i][j]);
        }
    }
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < p; ++j) {
            scanf("%lld", &B[i][j]);
        }
    }

    // 初始化 C 为 0 并计算乘积
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < p; ++j) {
            long long sum = 0;
            for (int k = 0; k < m; ++k) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }

    // 输出结果
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < p; ++j) {
            if (j) putchar(' ');
            printf("%lld", C[i][j]);
        }
        putchar('\n');
    }
    return 0;
}