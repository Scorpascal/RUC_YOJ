#include <stdio.h>

int main() {
    int N;
    if (scanf("%d", &N) != 1) return 0;

    int found = 0;
    // 价格：手雷(2 万/个), 冲锋枪(6 万/支), 大杀器(1 万/个)
    // 输出要求：按 x、y、z 递增的字典序
    for (int x = 2; 2 * x <= N; x += 2) {            // 手雷必须成对购买
        for (int y = 1; 6 * y <= N; ++y) {
            for (int z = 8; z <= N; z += 10) {       // 大杀器尾数为 8
                int S = 2 * x + 6 * y + z;
                if (S > N) continue;                 // 不超预算
                // 原来是 S*10 < 9*N（允许等于 90%），评测要求严格大于 90%
                if (S * 10 <= 9 * N) continue;       // 严格 > 90%

                // 数量各不相同
                if (x == y || x == z || y == z) continue;

                // x 在 y 和 z 之间（严格）
                if (!((z < x && x < y) || (y < x && x < z))) continue;

                // 若 y 是一位数，则 x 必须是两位数
                if (y >= 1 && y <= 9) {
                    if (!(x >= 10 && x <= 99)) continue;
                }

                printf("%d %d %d\n", x, y, z);
                found = 1;
            }
        }
    }

    if (!found) {
        printf("no answer\n");
    }
    return 0;
}