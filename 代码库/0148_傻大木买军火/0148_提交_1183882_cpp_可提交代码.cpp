#include <stdio.h>
#include <stdlib.h>

typedef struct { int x, g, b; } Sol;

int cmp_sol(const void *a, const void *b) {
    Sol *A = (Sol*)a;
    Sol *B = (Sol*)b;
    if (A->x != B->x) return A->x - B->x;
    if (A->g != B->g) return A->g - B->g;
    return A->b - B->b;
}

int main() {
    int N;
    if (scanf("%d", &N) != 1) return 0;
    Sol sols[10000];
    int cnt = 0;

    for (int b = 8; b <= 1000; b += 10) { // b 的尾数是 8
        if (b > N) break; // 总价至少为 b，超过 N 则无需再试
        for (int g = 1; g <= N; ++g) {
            for (int x = 2; x <= N; x += 2) { // 手雷成对
                if (x == g || x == b || g == b) continue; // 三种数量各不相同
                if (!((b < x && x < g) || (g < x && x < b))) continue; // x 在两者之间
                if (g < 10) { // 冲锋枪一位数时，手雷必须是两位数
                    if (x < 10 || x > 99) continue;
                }
                int cost = 2 * x + 6 * g + 1 * b; // 单位：万元
                if (cost <= N && cost * 10 > 9 * N) { // 花费不超过 N 且严格超过 0.9*N
                    sols[cnt].x = x;
                    sols[cnt].g = g;
                    sols[cnt].b = b;
                    cnt++;
                }
            }
        }
    }

    if (cnt == 0) {
        printf("no answer\n");
        return 0;
    }

    qsort(sols, cnt, sizeof(Sol), cmp_sol);
    for (int i = 0; i < cnt; ++i) {
        printf("%d %d %d\n", sols[i].x, sols[i].g, sols[i].b);
    }
    return 0;
}