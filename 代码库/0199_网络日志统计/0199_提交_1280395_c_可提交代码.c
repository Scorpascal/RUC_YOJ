#include <stdio.h>
#include <stdlib.h>

typedef long long ll;

typedef struct {
    ll a, b;
} Interval;

int cmp(const void *p1, const void *p2) {
    const Interval *x = (const Interval *)p1;
    const Interval *y = (const Interval *)p2;
    if (x->a < y->a) return -1;
    if (x->a > y->a) return 1;
    if (x->b < y->b) return -1;
    if (x->b > y->b) return 1;
    return 0;
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    Interval *arr = (Interval *)malloc(sizeof(Interval) * n);
    for (int i = 0; i < n; ++i) {
        ll a, b;
        scanf("%lld %lld", &a, &b);
        // 题目为闭区间 [a, b]，无需调整
        if (a > b) { ll t = a; a = b; b = t; }
        arr[i].a = a;
        arr[i].b = b;
    }

    qsort(arr, n, sizeof(Interval), cmp);

    // 合并区间
    ll maxOnlineLen = 0;
    ll maxGapLen = 0;

    ll curL = arr[0].a;
    ll curR = arr[0].b;

    for (int i = 1; i < n; ++i) {
        if (arr[i].a <= curR) {
            // 有交集或相连（由于区间是闭区间，[x,y] 与 [y,z] 有交集点 y）
            if (arr[i].b > curR) curR = arr[i].b;
        } else {
            // 断开：统计当前合并段长度与间隔
            ll curLen = curR - curL;           // 闭区间长度用差值即可，样例使用差值
            if (curLen > maxOnlineLen) maxOnlineLen = curLen;

            ll gap = arr[i].a - curR;          // 相邻合并段之间的间隔
            if (gap > maxGapLen) maxGapLen = gap;

            // 开始新的合并段
            curL = arr[i].a;
            curR = arr[i].b;
        }
    }
    // 最后一个合并段
    ll lastLen = curR - curL;
    if (lastLen > maxOnlineLen) maxOnlineLen = lastLen;

    // 输出：最长在线时间段 和 从首次上线后最长无用户在线时间段
    printf("%lld %lld\n", maxOnlineLen, maxGapLen);

    free(arr);
    return 0;
}