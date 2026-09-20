#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    int idx;
    int val;
} Node;

int cmp_int(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y);
}

// lower_bound: first index with arr[idx] >= key
static inline int lower_bound(const int *arr, int n, int key) {
    int l = 0, r = n;
    while (l < r) {
        int m = l + ((r - l) >> 1);
        if (arr[m] < key) l = m + 1;
        else r = m;
    }
    return l;
}

int main() {
    // Fast IO via stdio is enough for C
    int n;
    if (scanf("%d", &n) != 1) return 0;

    int *x = (int*)malloc((n + 1) * sizeof(int));
    int *m = (int*)malloc((n + 1) * sizeof(int));
    int *k = (int*)malloc(n * sizeof(int));
    if (!x || !m || !k) return 0;

    for (int i = 1; i <= n; ++i) scanf("%d", &x[i]);
    for (int i = 1; i <= n; ++i) scanf("%d", &m[i]);

    // Monotonic deque for window [L_i, i-1], right endpoint grows by 1 each i.
    Node *dq = (Node*)malloc((n + 1) * sizeof(Node));
    int head = 0, tail = 0;

    // Build k_i
    for (int i = 1; i <= n; ++i) {
        // Add new right endpoint: index i-1 (if exists)
        if (i - 1 >= 1) {
            int idx = i - 1;
            int val = x[idx];
            while (tail > head && dq[tail - 1].val <= val) tail--;
            dq[tail++] = (Node){ idx, val };
        }
        // Compute L_i, clamp to 1
        int64_t Li64 = (int64_t)i - (int64_t)m[i];
        int L = (Li64 < 1) ? 1 : (int)Li64;

        // Pop left while out of window
        while (tail > head && dq[head].idx < L) head++;

        // Window [L, i-1]; if empty, k_i=0
        if (tail == head) k[i - 1] = 0; // store k for day i at position i-1 (0-based)
        else k[i - 1] = dq[head].val;
    }

    // Sort k
    qsort(k, n, sizeof(int), cmp_int);

    int T;
    scanf("%d", &T);
    for (int t = 0; t < T; ++t) {
        int p, q;
        scanf("%d %d", &p, &q);
        int low = lower_bound(k, n, p);
        int mid = lower_bound(k, n, q) - low;
        printf("%d %d\n", low, mid);
    }

    free(x); free(m); free(k); free(dq);
    return 0;
}