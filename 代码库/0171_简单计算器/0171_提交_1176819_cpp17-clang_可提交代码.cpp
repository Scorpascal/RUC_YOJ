#include <stdio.h>

int main() {
    long long a, b, result;
    char op;
    if (scanf("%lld %lld %c", &a, &b, &op) != 3) {
        printf("NO\n");
        return 0;
    }
    switch (op) {
        case '+':
            result = a + b;
            printf("%lld\n", result);
            break;
        case '-':
            result = a - b;
            printf("%lld\n", result);
            break;
        case '*':
            result = a * b;
            printf("%lld\n", result);
            break;
        case '/':
            if (b == 0) {
                printf("NO\n");
            } else {
                result = a / b;
                printf("%lld\n", result);
            }
            break;
        case '%':
            if (b == 0) {
                printf("NO\n");
            } else {
                result = a % b;
                printf("%lld\n", result);
            }
            break;
        default:
            printf("NO\n");
    }
    return 0;
}