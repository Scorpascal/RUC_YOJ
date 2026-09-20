#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int a = 0, b = 0;
    int x;
    while (scanf("%d", &x) == 1) {
        if (x == 0) a++;
        else if (x == 1) b++;
        else continue; /* 忽略非 0/1 的输入 */
        if ((a >= 21 || b >= 21) && abs(a - b) >= 2) {
            printf("%d\n", a > b ? 0 : 1);
            return 0;
        }
    }
    return 0;
}