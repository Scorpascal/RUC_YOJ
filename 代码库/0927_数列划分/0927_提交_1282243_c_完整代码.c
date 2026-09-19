#include <stdio.h>

int main() {
    int n;
    long long m;
    if (scanf("%d %lld", &n, &m) != 2) return 0;

    long long x, sum = 0;
    int segments = 0;

    for (int i = 0; i < n; i++) {
        if (scanf("%lld", &x) != 1) return 0;
        if (x > m) { // 单个元素超过上限，无法划分
            printf("-1\n");
            return 0;
        }
        if (sum == 0) { // 开始新的段
            segments++;
            sum = x;
        } else if (sum + x <= m) {
            sum += x;
        } else {
            // 需要新段
            segments++;
            sum = x;
        }
    }

    printf("%d\n", segments);
    return 0;
}