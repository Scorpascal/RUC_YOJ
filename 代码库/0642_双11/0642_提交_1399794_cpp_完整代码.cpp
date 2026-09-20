#include <stdio.h>

int main(void) {
    int p1, p2, q1, q2;
    scanf("%d%d", &p1, &p2);
    scanf("%d%d", &q1, &q2);
    
    double total = p1 * q1 + p2 * q2;
    int total_qty = q1 + q2;
    
    // 店家优惠
    double discount = 1.0;
    if (total_qty == 2) discount = 0.9;
    else if (total_qty >= 3 && total_qty <= 4) discount = 0.8;
    else if (total_qty == 5) discount = 0.7;
    double cost1 = total * discount;
    
    // 双11特惠
    int reduce = (int)(total / 500) * 100;
    double cost2 = total - reduce;
    
    // 比较并输出
    if (cost1 <= cost2) {
        printf("1 %.1f\n", cost1);
    } else {
        printf("2 %.1f\n", cost2);
    }
    
    return 0;
}