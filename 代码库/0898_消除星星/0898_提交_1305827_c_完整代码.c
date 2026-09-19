#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    long long *a = (long long *)malloc((n + 2) * sizeof(long long));
    for (int i = 1; i <= n; ++i) scanf("%lld", &a[i]);
    a[0] = 0;          // 左哨兵
    a[n + 1] = 0;      // 右哨兵，便于清栈

    int *st = (int *)malloc((n + 2) * sizeof(int));
    int top = 0;
    st[top++] = 0;

    long long ans = 0;
    for (int i = 1; i <= n + 1; ++i) {
        while (top > 0 && a[st[top - 1]] > a[i]) {
            int h_idx = st[--top];
            long long height = a[h_idx];
            int left_idx = st[top - 1];   // 弹出后新的栈顶
            long long width = i - left_idx - 1;
            long long area = height * width;
            if (area > ans) ans = area;
        }
        st[top++] = i;
    }

    printf("%lld\n", ans);
    free(a);
    free(st);
    return 0;
}