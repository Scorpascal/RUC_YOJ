#include <stdio.h>
#include <stdlib.h>

static int cmp_int(const void *a, const void *b) {
    unsigned int x = *(const unsigned int *)a;
    unsigned int y = *(const unsigned int *)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

int main(void) {
    int n;
    unsigned int m;
    if (scanf("%d", &n) != 1) return 0;
    if (scanf("%u", &m) != 1) return 0;
    unsigned int *a = (unsigned int *)malloc(sizeof(unsigned int) * n);
    if (!a) return 0;
    for (int i = 0; i < n; ++i) {
        if (scanf("%u", &a[i]) != 1) { free(a); return 0; }
    }

    qsort(a, n, sizeof(unsigned int), cmp_int);

    int left = 0, right = n - 1;
    int idx = -1;
    int compares = 0;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        compares++; // 比较 m 与 a[mid]
        if (m == a[mid]) {
            idx = mid;
            break;
        } else if (m < a[mid]) {
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    printf("%d\n", idx);
    printf("%d\n", compares);

    free(a);
    return 0;
}