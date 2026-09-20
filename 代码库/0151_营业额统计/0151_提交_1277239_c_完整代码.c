#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    long long sum = 0;

    int *sorted = (int *)malloc(sizeof(int) * n);
    int sz = 0;

    for (int i = 0; i < n; i++) {
        int a;
        if (scanf("%d", &a) != 1) a = 0;

        if (i == 0) {
            sum += a;
            sorted[sz++] = a;
            continue;
        }

        // 二分查找插入位置
        int l = 0, r = sz;
        while (l < r) {
            int m = (l + r) >> 1;
            if (sorted[m] < a) l = m + 1;
            else r = m;
        }
        int pos = l;

        // 计算与前驱/后继的最小差
        int diff = 1000000000;
        if (pos > 0) {
            int d = a - sorted[pos - 1];
            if (d < diff) diff = d;
        }
        if (pos < sz) {
            int d = sorted[pos] - a;
            if (d < diff) diff = d;
        }
        sum += diff;

        // 插入保持有序
        for (int j = sz; j > pos; --j) sorted[j] = sorted[j - 1];
        sorted[pos] = a;
        sz++;
    }

    printf("%lld\n", sum);
    free(sorted);
    return 0;
}