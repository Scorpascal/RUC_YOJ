#include <stdio.h>
#include <math.h>

int main(void) {
    long long a_ll;
    if (scanf("%lld", &a_ll) != 1) return 0;
    double a = (double)a_ll;

    double x = a / 2.0;
    double next = 0.5 * (x + a / x);
    int t = 1;
    const double eps = 1e-5;

    while (fabs(next - x) >= eps) {
        x = next;
        next = 0.5 * (x + a / x);
        t++;
    }

    printf("%.6f\n%d\n", next, t);
    return 0;
}