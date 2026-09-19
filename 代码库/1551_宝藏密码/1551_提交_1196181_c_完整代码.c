#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    for (int i = 1; i <= n; ++i) {
        for (int j = 0; j < i; ++j) putchar('$');
        if (i < n) putchar('\n');
    }
    return 0;
}