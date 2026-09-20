#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    int *a = (int*)malloc(sizeof(int) * n * m);
    if (!a) return 0;

    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < m; ++k) {
            scanf("%d", &a[i * m + k]);
        }
    }

    for (int i = 0; i < n; ++i) {
        int ans = 0; // 默认没有上级
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            bool ok = true;
            for (int k = 0; k < m; ++k) {
                if (a[j * m + k] <= a[i * m + k]) { // 需严格大于
                    ok = false;
                    break;
                }
            }
            if (ok) { // 第一个满足的即为编号最小
                ans = j + 1; // 转为1-based
                break;
            }
        }
        printf("%d\n", ans);
    }

    free(a);
    return 0;
}