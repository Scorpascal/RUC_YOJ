#include <stdio.h>
#include <string.h>

int main(void) {
    char s[256];
    if (!fgets(s, sizeof s, stdin)) return 0;
    double a = 0, b = 0, c = 0;
    if (sscanf(s, "%lf + %lf + %lf", &a, &b, &c) < 3) {
        if (sscanf(s, "%lf+%lf+%lf", &a, &b, &c) < 3) return 0;
    }
    printf("%.2f\n", a + b + c);
    return 0;
}