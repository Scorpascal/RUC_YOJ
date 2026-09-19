#include <stdio.h>

int main(void) {
    int n, m, a, b;
    if (scanf("%d %d %d %d", &n, &m, &a, &b) != 4) return 0;

    int denom = a - b;
    int x = (m - b * n) / denom; // A 的数量
    int y = n - x;               // B 的数量

    printf("%d %d\n", x, y);
    return 0;
}