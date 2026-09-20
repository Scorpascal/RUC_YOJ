#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int n, d;
    if (scanf("%d %d", &n, &d) != 2) return 0;

    long long *Q = (long long*)malloc((size_t)n * d * sizeof(long long));
    long long *K = (long long*)malloc((size_t)n * d * sizeof(long long));
    long long *V = (long long*)malloc((size_t)n * d * sizeof(long long));
    long long *W = (long long*)malloc((size_t)n * sizeof(long long));
    long long *T = (long long*)calloc((size_t)d * d, sizeof(long long)); // K^T * V
    long long *P = (long long*)malloc((size_t)n * d * sizeof(long long)); // Q * T

    // 读入 Q, K, V
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < d; ++j)
            scanf("%lld", &Q[(size_t)i * d + j]);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < d; ++j)
            scanf("%lld", &K[(size_t)i * d + j]);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < d; ++j)
            scanf("%lld", &V[(size_t)i * d + j]);

    // 读入 W
    for (int i = 0; i < n; ++i) scanf("%lld", &W[i]);

    // T = K^T * V -> d×d，其中 T[a,b] = sum_j K[j,a] * V[j,b]
    for (int a = 0; a < d; ++a) {
        for (int b = 0; b < d; ++b) {
            long long sum = 0;
            for (int j = 0; j < n; ++j) {
                sum += K[(size_t)j * d + a] * V[(size_t)j * d + b];
            }
            T[(size_t)a * d + b] = sum;
        }
    }

    // P = Q * T -> n×d，其中 P[i,b] = sum_a Q[i,a] * T[a,b]
    for (int i = 0; i < n; ++i) {
        for (int b = 0; b < d; ++b) {
            long long sum = 0;
            for (int a = 0; a < d; ++a) {
                sum += Q[(size_t)i * d + a] * T[(size_t)a * d + b];
            }
            P[(size_t)i * d + b] = sum;
        }
    }

    // 输出 (diag(W) * P)，即按行缩放
    for (int i = 0; i < n; ++i) {
        for (int b = 0; b < d; ++b) {
            long long val = P[(size_t)i * d + b] * W[i];
            if (b) putchar(' ');
            printf("%lld", val);
        }
        putchar('\n');
    }

    free(Q); free(K); free(V); free(W); free(T); free(P);
    return 0;
}