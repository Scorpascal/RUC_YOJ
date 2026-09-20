#include <stdio.h>
#include <math.h>

int main(void) {
    double pi = acos(-1.0);

    double y1 = 1.0 + 1.0 / (1.0 + 1.0 / (1.0 + 1.0 / 5.0));
    double y2 = sqrt(3.0 * 3.0 + 4.0 * 4.0);
    double y3 = sqrt((1.0 - cos(pi / 3.0)) / 2.0);
    double s = sin(pi / 4.0), c = cos(pi / 4.0);
    double y4 = 2.0 * s * s + s * c - c * c;
    double y5 = 2.0 * sqrt(5.0) * (sqrt(6.0) + sqrt(3.0)) / (6.0 + 3.0);

    printf("%.3f\n", y1);
    printf("%.3f\n", y2);
    printf("%.3f\n", y3);
    printf("%.3f\n", y4);
    printf("%.3f\n", y5);

    return 0;
}