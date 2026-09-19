#include <stdio.h>
#include <math.h>
int main() {
    double a, b, c;
    scanf("%lf %lf %lf", &a, &b, &c);
    double s = (a + b + c) / 2.0;
    double area = sqrt(s * (s - a) * (s - b) * (s - c));
    printf("%.2f\n", area);
    return 0;
}