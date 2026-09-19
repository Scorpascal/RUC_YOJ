#include <stdio.h>

int main() {
    int k;
    scanf("%d", &k);
    double sum = 0.0;
    int n = 0;
    while (sum <= k) {
        n++;
        sum += 1.0 / n;
    }
    printf("%d\n", n);
    return 0;
}