#include <stdio.h>
#include <limits.h>

int main(void) {
    int m[6]; // 主菜 1..5
    int d[4]; // 饮料 1..3
    if (scanf("%d %d %d %d %d", &m[1], &m[2], &m[3], &m[4], &m[5]) != 5) return 0;
    if (scanf("%d %d %d", &d[1], &d[2], &d[3]) != 3) return 0;
    int budget;
    if (scanf("%d", &budget) != 1) return 0;

    int count = 0;
    int best_i = 0, best_j = 0, best_k = 0;
    int best_price = INT_MAX;

    for (int i = 1; i <= 5; ++i) {
        for (int j = i + 1; j <= 5; ++j) {
            // 主菜1和主菜3不能同时选择
            if (i == 1 && j == 3) continue;
            for (int k = 1; k <= 3; ++k) {
                // 如果选择主菜5，则必须选择饮料3
                if (i == 5 || j == 5) {
                    if (k != 3) continue;
                }
                // 如果选择主菜2，则不能选择饮料1
                if (i == 2 || j == 2) {
                    if (k == 1) continue;
                }
                int total = m[i] + m[j] + d[k];
                if (total > budget) continue;
                ++count;
                if (total < best_price ||
                    (total == best_price && (best_i == 0 ||
                     (i < best_i || (i == best_i && (j < best_j || (j == best_j && k < best_k))))))) {
                    best_price = total;
                    best_i = i; best_j = j; best_k = k;
                }
            }
        }
    }

    if (count == 0) {
        printf("No solution\n");
    } else {
        printf("%d\n", count);
        printf("%d %d %d %d\n", best_i, best_j, best_k, best_price);
    }
    return 0;
}