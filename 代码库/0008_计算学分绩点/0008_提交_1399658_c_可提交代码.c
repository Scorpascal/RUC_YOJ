#include <stdio.h>

int main() {
    int score;
    scanf("%d", &score);

    double point = 0.0;
    if (score >= 90 && score <= 100) point = 4.0;
    else if (score >= 86) point = 3.7;
    else if (score >= 83) point = 3.3;
    else if (score >= 80) point = 3.0;
    else if (score >= 76) point = 2.7;
    else if (score >= 73) point = 2.3;
    else if (score >= 70) point = 2.0;
    else if (score >= 66) point = 1.7;
    else if (score >= 63) point = 1.3;
    else if (score >= 60) point = 1.0;
    else point = 0;

    if (point < 1.0)
        printf("%d\n", (int)point);
    else
        printf("%.1f\n", point);

    return 0;
}