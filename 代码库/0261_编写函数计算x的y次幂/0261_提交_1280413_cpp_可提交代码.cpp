#include <stdio.h>
#include <stdlib.h>

int power(int x, int y) {
    if (y == 0) return 1;
    long long base = x;
    long long result = 1;
    while (y > 0) {
        if (y & 1) result *= base;
        base *= base;
        y >>= 1;
    }
    return (int)result;
}

int main()
{
    int x, y;
    scanf("%d%d", &x, &y);
    printf("%d", power(x, y));
    return 0;

}