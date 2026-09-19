#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    long long k;
    if (scanf("%d %lld", &n, &k) != 2) return 0;
    long long *a = malloc(sizeof(long long) * n);
    for (int i = 0; i < n; ++i) scanf("%lld", &a[i]);

    int l = 0, r = n - 1;
    long long L = a[0], R = a[n-1];

    while (L < R && k > 0) {
        while (l + 1 < n && a[l+1] == L) l++;
        while (r - 1 >= 0 && a[r-1] == R) r--;
        if (l >= r) break;

        long long cntL = l + 1;
        long long cntR = n - r;

        if (cntL <= cntR) {
            long long next = a[l+1];
            long long diff = next - L;
            long long need = diff * cntL;
            if (need <= k) {
                k -= need;
                L = next;
                l++;
            } else {
                L += k / cntL;
                k = 0;
            }
        } else {
            long long prev = a[r-1];
            long long diff = R - prev;
            long long need = diff * cntR;
            if (need <= k) {
                k -= need;
                R = prev;
                r--;
            } else {
                R -= k / cntR;
                k = 0;
            }
        }
    }

    if (R < L) R = L;
    printf("%lld\n", R - L);
    free(a);
    return 0;
}