#include <stdio.h>

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    int m = 0;
    // 按鸡翁、鸡母、鸡雏的顺序遍历，确保输出已按要求排序
    for (int x = 0; x <= n; ++x) {            // 鸡翁
        for (int y = 0; y <= n - x; ++y) {    // 鸡母
            int z = n - x - y;                // 鸡雏
            if (z < 0) continue;
            if (z % 3 != 0) continue;         // 雏鸡必须3只1钱
            // 成本：5x + 3y + z/3 == n
            if (5 * x + 3 * y + z / 3 == n) {
                printf("%d %d %d\n", x, y, z);
                m++;
            }
        }
    }
    printf("%d\n", m);
    return 0;
}