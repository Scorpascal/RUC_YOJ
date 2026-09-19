#include <stdio.h>

int main() {
    int F;
    scanf("%d", &F);
    double C = (F - 32) * 5.0 / 9.0;
    printf("%.2f\n", C);
    return 0;
}