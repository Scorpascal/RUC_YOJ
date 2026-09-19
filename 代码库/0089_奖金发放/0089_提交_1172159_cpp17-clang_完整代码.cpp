#include <stdio.h>

#define MAX_N 110

int main() {
    int n;
    int bound[MAX_N];
    double rate[MAX_N];
    int profit;
    double bonus = 0.0;

    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%d %lf", &bound[i], &rate[i]);
    }
    scanf("%d", &profit);

    if (profit < 0) {
        printf("NO\n");
        return 0;
    }

    int last = 0;
    for (int i = 0; i < n; i++) {
        if (bound[i] == -1) {
            // 超过最后一个分界值的部分
            bonus += (profit - last) * rate[i];
            break;
        }
        if (profit > bound[i]) {
            bonus += (bound[i] - last) * rate[i];
            last = bound[i];
        } else {
            bonus += (profit - last) * rate[i];
            break;
        }
    }
    printf("%.2lf\n", bonus);
    return 0;
}