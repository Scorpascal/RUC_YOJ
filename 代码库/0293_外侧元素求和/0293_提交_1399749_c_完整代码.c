#include <stdio.h>

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    long long sum = 0; // 和可能较大，使用 long long
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            int x;
            if (scanf("%d", &x) != 1) return 0;
            // 判断是否为外侧元素：首行/末行或首列/末列
            if (i == 0 || i == n - 1 || j == 0 || j == m - 1) {
                sum += x;
            }
        }
    }

    printf("%lld\n", sum);
    return 0;
}