#include <stdio.h>
#include <limits.h>

int main() {
    int opp[8];
    if (scanf("%d %d %d %d %d %d %d %d",
              &opp[0], &opp[1], &opp[2], &opp[3],
              &opp[4], &opp[5], &opp[6], &opp[7]) != 8) return 0;

    long long cost_unit[8] = {100,200,500,1000,1000,600,600,2000};
    // rows = 我方兵种 A..H, cols = 对方兵种 A..H
    long long need[8][8] = {
        {100, 500, -1, 10000, 200, 500, 400, 1200},   // 我方 A
        {20,  100, -1, 10000, 300, -1,  -1,  -1   },   // 我方 B
        {10,  8,   100, 17,    22,  200, 1000, 3000 },   // 我方 C
        {1,   1,   -1,  100,   1,   2000,1500, 5000 },   // 我方 D
        {50,  33,  -1,  10000, 100, 50,  30,  70   },   // 我方 E
        {20,  5,   50,  5,     200, 100, 50,  400  },   // 我方 F
        {50,  20,  10,  7,     350, 200, 100, 1000 },   // 我方 G
        {8,   5,   10,  10,    200, 30,  20,  100  }    // 我方 H
    };

    long long total = 0;
    for (int col = 0; col < 8; ++col) {
        int groups = opp[col] / 100;
        if (groups == 0) continue;
        long long best = LLONG_MAX;
        for (int row = 0; row < 8; ++row) {
            if (need[row][col] < 0) continue;
            long long units = need[row][col] * (long long)groups;
            long long c = units * cost_unit[row];
            if (c < best) best = c;
        }
        if (best == LLONG_MAX) {
            // 无法对抗该种兵（题目数据应保证不出现），输出 0 并退出
            printf("0\n");
            return 0;
        }
        total += best;
    }

    printf("%lld\n", total);
    return 0;
}