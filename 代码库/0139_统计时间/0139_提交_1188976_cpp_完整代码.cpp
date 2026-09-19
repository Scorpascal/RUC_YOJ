#include <stdio.h>
#include <limits.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    long long sum = 0;
    int t;
    int mn = INT_MAX, mx = INT_MIN;
    for (int i = 0; i < n; i++) {
        if (scanf("%d", &t) != 1) t = 0;
        if (t < mn) mn = t;
        if (t > mx) mx = t;
        sum += t;
    }
    long long avg = (sum + n/2) / n; // 四舍五入
    printf("%d %d %lld\n", mx, mn, avg);
    return 0;
}