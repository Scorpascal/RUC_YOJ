#include <stdio.h>

int main(void) {
    double price;
    int qty;
    if (scanf("%lf %d", &price, &qty) != 2) return 0;

    int discounted_count = qty > 5 ? 5 : qty;
    int extra_count = qty > 5 ? qty - 5 : 0;

    double discount = 1.0;
    if (qty >= 3) {
        discount = 0.7;
    } else if (qty >= 2) {
        discount = 0.8;
    }

    double total = discounted_count * price * discount + extra_count * price * 1.1;
    printf("%.2f\n", total);
    return 0;
}