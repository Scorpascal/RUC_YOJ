#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__APPLE__)
#define GETC getchar_unlocked
#else
#define GETC getchar
#endif

static inline int read_int() {
    int c = GETC();
    while (c!='-' && (c<'0' || c>'9')) c = GETC();
    int sign = 1;
    if (c=='-') { sign = -1; c = GETC(); }
    int x = 0;
    while (c>='0' && c<='9') {
        x = x*10 + (c - '0');
        c = GETC();
    }
    return sign * x;
}

static inline void swap(int *a, int *b) {
    int t = *a; *a = *b; *b = t;
}

static inline int median_of_three(int *arr, int l, int m, int r) {
    int a = arr[l], b = arr[m], c = arr[r];
    if (a < b) {
        if (b < c) return m;         // a < b < c
        else if (a < c) return r;    // a < c <= b
        else return l;               // c <= a < b
    } else {
        if (a < c) return l;         // b <= a < c
        else if (b < c) return r;    // b < c <= a
        else return m;               // c <= b <= a
    }
}

static int quickselect(int *arr, int n, int k) {
    // Find k-th smallest, k is 0-based
    int left = 0, right = n - 1;
    while (left <= right) {
        // median-of-three pivot
        int mid = left + ((right - left) >> 1);
        int pidx = median_of_three(arr, left, mid, right);
        int pivot = arr[pidx];
        // move pivot to end
        swap(&arr[pidx], &arr[right]);

        // partition (Lomuto)
        int i = left;
        for (int j = left; j < right; ++j) {
            if (arr[j] < pivot) {
                swap(&arr[i], &arr[j]);
                ++i;
            }
        }
        swap(&arr[i], &arr[right]); // place pivot

        if (k == i) return arr[i];
        else if (k < i) right = i - 1;
        else left = i + 1;
    }
    // Should not reach here
    return arr[k];
}

int main() {
    // fast input
    int n = read_int();
    int k = read_int();
    if (n <= 0 || k < 1 || k > n) return 0;

    int *arr = (int*)malloc(sizeof(int) * (size_t)n);
    if (!arr) return 0;

    for (int i = 0; i < n; ++i) arr[i] = read_int();

    int ans = quickselect(arr, n, k - 1);
    printf("%d\n", ans);

    free(arr);
    return 0;
}