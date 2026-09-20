#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1 || n <= 0) return 0;

    double max = 0.0, min = 0.0, sum = 0.0;
    int total = n * n;
    for (int i = 0; i < total; ++i) {
        double h;
        if (scanf("%lf", &h) != 1) return 0;
        if (i == 0) {
            max = min = h;
        } else {
            if (h > max) max = h;
            if (h < min) min = h;
        }
        sum += h;
    }

    double avg = sum / total;
    printf("%.3f %.3f %.2f\n", max, min, avg);
    return 0;
}