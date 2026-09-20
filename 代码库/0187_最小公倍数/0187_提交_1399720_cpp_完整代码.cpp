#include <stdio.h>

// 求最大公约数
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main() {
    int a, b;
    scanf("%d %d", &a, &b);
    int lcm = a * b / gcd(a, b);
    printf("%d\n", lcm);
    return 0;
}