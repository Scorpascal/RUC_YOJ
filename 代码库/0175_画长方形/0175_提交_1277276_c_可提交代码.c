#include <stdio.h>

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            int d = i - j;
            if (d < 0) d = -d;
            putchar('A' + d);
        }
        putchar('\n');
    }
    return 0;
}