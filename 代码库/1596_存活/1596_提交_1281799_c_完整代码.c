#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#define BUF_SIZE (1 << 20)

static char ibuf[BUF_SIZE];
static size_t bi = 0, bn = 0;

static inline int gc() {
    if (bi >= bn) {
        bn = fread(ibuf, 1, BUF_SIZE, stdin);
        bi = 0;
        if (bn == 0) return EOF;
    }
    return ibuf[bi++];
}

static inline int read_ll(long long *out) {
    int c = gc();
    if (c == EOF) return 0;
    while (c != '-' && (c < '0' || c > '9')) {
        c = gc();
        if (c == EOF) return 0;
    }
    int neg = 0;
    if (c == '-') { neg = 1; c = gc(); }
    long long x = 0;
    for (; c >= '0' && c <= '9'; c = gc()) x = x * 10 + (c - '0');
    *out = neg ? -x : x;
    return 1;
}

int main(void) {
    long long nll;
    if (!read_ll(&nll)) return 0;
    if (nll <= 0) { printf("0\n"); return 0; }
    int n = (int)nll;

    unsigned long long *a = (unsigned long long*)malloc(sizeof(unsigned long long) * n);
    unsigned long long *b = (unsigned long long*)malloc(sizeof(unsigned long long) * n);
    if (!a || !b) return 0;

    // First pass: read and compute max1, max2, cnt1
    unsigned long long max1 = 0, max2 = 0;
    int cnt1 = 0;

    for (int i = 0; i < n; ++i) {
        long long ai, bi;
        read_ll(&ai);
        read_ll(&bi);
        a[i] = (unsigned long long)ai;
        b[i] = (unsigned long long)bi;

        if (a[i] > max1) {
            max2 = max1;
            max1 = a[i];
            cnt1 = 1;
        } else if (a[i] == max1) {
            cnt1++;
        } else if (a[i] > max2) {
            max2 = a[i];
        }
    }

    // Second pass: classify into U (unkillable) and K (killable)
    int c = 0; // |U|
    unsigned long long Au_max = 0; // max a in U
    unsigned long long Bk_min = ULLONG_MAX; // min b in K
    int kcnt = 0; // |K|

    for (int i = 0; i < n; ++i) {
        unsigned long long max_other = (a[i] == max1 && cnt1 == 1) ? max2 : max1;
        if (b[i] > max_other) {
            // Unkillable
            c++;
            if (a[i] > Au_max) Au_max = a[i];
        } else {
            // Killable
            kcnt++;
            if (b[i] < Bk_min) Bk_min = b[i];
        }
    }

    int ans;
    if (kcnt == 0) {
        // Everyone is unkillable
        ans = n;
    } else if (c == 0) {
        // No unkillables -> can always reduce to 1
        ans = 1;
    } else {
        // Have both U and K
        if (Au_max >= Bk_min) ans = c;      // U can finish K
        else                 ans = c + 1;  // U cannot finish K -> one K remains
    }

    printf("%d\n", ans);

    free(a);
    free(b);
    return 0;
}//1596AK