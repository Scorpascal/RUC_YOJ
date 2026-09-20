#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct { int l, r; } Interval;

int cmp_r(const void *a, const void *b) {
    const Interval *ia = a;
    const Interval *ib = b;
    if (ia->r != ib->r) return (ia->r < ib->r) ? -1 : 1;
    return (ia->l < ib->l) ? -1 : (ia->l > ib->l) ? 1 : 0;
}

int main(void) {
    int M;
    if (scanf("%d", &M) != 1) return 0;
    Interval *arr = malloc(sizeof(Interval) * M);
    for (int i = 0; i < M; ++i) {
        scanf("%d %d", &arr[i].l, &arr[i].r);
    }
    qsort(arr, M, sizeof(Interval), cmp_r);
    long long last = (long long)LLONG_MIN; // 上次选的整数点
    int ans = 0;
    for (int i = 0; i < M; ++i) {
        if (last < arr[i].l) {
            last = arr[i].r;
            ans++;
        }
    }
    printf("%d\n", ans);
    free(arr);
    return 0;
}