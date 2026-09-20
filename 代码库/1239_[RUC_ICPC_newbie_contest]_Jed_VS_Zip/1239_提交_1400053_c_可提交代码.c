#include <stdio.h>
#include <stdint.h>

static inline long long clamp_low(long long x, long long lo){ return x < lo ? lo : x; }
static inline long long clamp_high(long long x, long long hi){ return x > hi ? hi : x; }

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        long long h, w, xa, ya, xb, yb;
        scanf("%lld %lld %lld %lld %lld %lld", &h, &w, &xa, &ya, &xb, &yb);

        // 起始即不能行动 -> 平局
        if (xa == h || xb == 1) {
            printf("Draw\n");
            continue;
        }
        // Jed 不在 Zip 上方 -> 永远越走越远 -> 平局
        if (xa >= xb) {
            printf("Draw\n");
            continue;
        }

        long long d = xb - xa;
        if (d & 1LL) {
            // 奇数：Jed 第 m 次行动能对齐行
            long long m = (d + 1) / 2;
            // 若在到达关键回合前 Jed 已到达底行，则平局
            if (xa + m > h) { printf("Draw\n"); continue; }

            long long jed_min = clamp_low(ya - m, 1), jed_max = clamp_high(ya + m, w);
            long long zip_min = clamp_low(yb - (m - 1), 1), zip_max = clamp_high(yb + (m - 1), w);

            if (jed_min <= zip_min && jed_max >= zip_max) printf("Jed\n");
            else printf("Draw\n");
        } else {
            // 偶数：Zip 第 m 次行动能对齐行
            long long m = d / 2;
            // 若在到达关键回合前 Jed 已到达底行，则平局
            if (xa + m > h) { printf("Draw\n"); continue; }

            long long jed_min = clamp_low(ya - m, 1), jed_max = clamp_high(ya + m, w);
            // Zip 在该步可到达列区间等价于 [yb - m, yb + m]（先前区间再扩展 ±1）
            long long zip_reach_min = clamp_low(yb - m, 1), zip_reach_max = clamp_high(yb + m, w);

            if (zip_reach_min <= jed_min && zip_reach_max >= jed_max) printf("Zip\n");
            else printf("Draw\n");
        }
    }
    return 0;
}