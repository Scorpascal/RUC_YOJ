#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static inline long long readll() {
    long long x;
    if (scanf("%lld", &x) != 1) exit(0);
    if (x < 0) { fprintf(stderr, "input must be non-negative\n"); exit(1); }
    return x;
}

void print_uint128(unsigned __int128 v) {
    if (v == 0) { putchar('0'); return; }
    char buf[1000];
    int idx = 0;
    while (v > 0) {
        unsigned int digit = (unsigned int)(v % 10);
        buf[idx++] = '0' + digit;
        v /= 10;
    }
    for (int i = idx - 1; i >= 0; --i) putchar(buf[i]);
}

int main() {
    int N, M;
    long long P;
    if (scanf("%d %d %lld", &N, &M, &P) != 3) return 0;
    if (N <= 0 || M <= 0 || P <= 0) { fprintf(stderr, "N, M, P must be positive.\n"); return 0; }

    // S 的下标从 1 开始，S[1][*] 与 S[*][1] 视为 0
    long long **S = (long long**)malloc((N + 1) * sizeof(long long*));
    for (int i = 0; i <= N; ++i) {
        S[i] = (long long*)calloc(M + 1, sizeof(long long));
    }
    for (int i = 2; i <= N; ++i) {
        for (int j = 2; j <= M; ++j) {
            S[i][j] = readll();
        }
    }

    // 原始矩阵 X
    long long **X = (long long**)malloc((N + 1) * sizeof(long long*));
    for (int i = 0; i <= N; ++i) {
        X[i] = (long long*)calloc(M + 1, sizeof(long long));
    }

    if (N == 1 || M == 1) {
        // 字典序最小：全 0
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= M; ++j) {
                X[i][j] = 0;
                if (j > 1) putchar(' ');
                printf("%lld", X[i][j]);
            }
            putchar('\n');
        }
    } else {
        // N>=2, M>=2
        // 策略：令第二行全为 0。自由变量为 t = X[1][1]。
        // 第一行：X[1][2] = S[2][2] - t，X[1][j] = S[2][j] - X[1][j-1] (j>=3)
        // 第一列：X[2][1] = 0，X[i][1] = S[i][2] - X[i-1][1] (i>=3)
        // 约束：所有 X 元素需在 [0, P-1]，先对第一行给出 t 的区间约束，取最小可行 t。

        // t 的可行区间
        long long low = 0, high = P - 1;

        // 第一行约束：X[1][2] = S22 - t ∈ [0, P-1]
        long long S22 = S[2][2];
        {
            long long lo = S22 - (P - 1);
            long long hi = S22;
            if (lo > low) low = lo;
            if (hi < high) high = hi;
        }
        // 递推得到 X[1][j] = s*t + C，更新 t 的区间
        long long s = -1, C = S22; // 对 j=2: X[1][2] = -t + S22
        for (int j = 3; j <= M; ++j) {
            long long new_s = -s;
            long long new_C = S[2][j] - C;
            s = new_s; C = new_C;
            long long lo, hi;
            if (s == +1) { // t ∈ [-C, P-1-C]
                lo = -C;
                hi = (P - 1) - C;
            } else {       // t ∈ [C-(P-1), C]
                lo = C - (P - 1);
                hi = C;
            }
            if (lo > low) low = lo;
            if (hi < high) high = hi;
        }

        if (low < 0) low = 0;
        if (high > P - 1) high = P - 1;
        if (low > high) {
            fprintf(stderr, "no feasible solution for non-negative X\n");
            return 0;
        }

        // 取字典序最小：t 取最小可行值
        long long t = low;

        // 构造第一行
        X[1][1] = t;
        X[2][1] = 0;            // 第二行置 0
        for (int j = 2; j <= M; ++j) {
            if (j == 2) X[1][j] = S22 - t;
            else X[1][j] = S[2][j] - X[1][j - 1];
        }

        // 构造第一列（与 t 无关）
        for (int i = 3; i <= N; ++i) {
            X[i][1] = S[i][2] - X[i - 1][1];
        }

        // 令第二行全为 0
        for (int j = 1; j <= M; ++j) X[2][j] = 0;
        // 令第二列（i>=2）为 0（由递推应满足）
        for (int i = 2; i <= N; ++i) X[i][2] = 0;

        // 填充内部元素 i>=3,j>=3
        for (int i = 3; i <= N; ++i) {
            for (int j = 3; j <= M; ++j) {
                X[i][j] = S[i][j] - X[i - 1][j] - X[i][j - 1] - X[i - 1][j - 1];
            }
        }

        // 输出矩阵 X
        for (int i = 1; i <= N; ++i) {
            for (int j = 1; j <= M; ++j) {
                if (j > 1) putchar(' ');
                printf("%lld", X[i][j]);
            }
            putchar('\n');
        }
    }

    // 取数游戏：每行独立，权重随轮次递增，贪心每步取两端较小的值以推迟大值
    unsigned __int128 total = 0;
    unsigned __int128 w = 2; // 2^1

    // 行左右指针（一次性初始化）
    int *l = (int*)malloc((N + 1) * sizeof(int));
    int *r = (int*)malloc((N + 1) * sizeof(int));
    for (int i = 1; i <= N; ++i) { l[i] = 1; r[i] = M; }

    for (int step = 1; step <= M; ++step) {
        __int128 round_sum = 0;
        for (int i = 1; i <= N; ++i) {
            long long left = X[i][l[i]];
            long long right = X[i][r[i]];
            if (left < right) {
                round_sum += left;
                l[i]++;
            } else if (left > right) {
                round_sum += right;
                r[i]--;
            } else {
                // 平局处理：前瞻比较，选择能使后续更小的一侧
                int li = l[i], ri = r[i];
                int choose_left = 1;
                while (li <= ri) {
                    long long vl = X[i][li];
                    long long vr = X[i][ri];
                    if (vl < vr) { choose_left = 1; break; }
                    if (vl > vr) { choose_left = 0; break; }
                    li++; ri--;
                }
                if (li > ri) choose_left = 1; // 完全相同，任选左
                if (choose_left) {
                    round_sum += left;
                    l[i]++;
                } else {
                    round_sum += right;
                    r[i]--;
                }
            }
        }
        total += round_sum * w;
        w <<= 1;
    }
    free(l); free(r);

    // 输出总得分
    print_uint128(total);
    putchar('\n');

    // 释放
    for (int i = 0; i <= N; ++i) free(S[i]);
    free(S);
    for (int i = 0; i <= N; ++i) free(X[i]);
    free(X);
    return 0;
}