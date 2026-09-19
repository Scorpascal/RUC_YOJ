#include <stdio.h>
#include <math.h>

// 分段函数定义
double f(double x) {
    if (fabs(x) <= 1)
        return fabs(x - 1) - 2;
    else
        return 1.0 / (1 + x * x);
}

int main() {
    double x;
    scanf("%lf", &x);
    double result = f(f(x));
    // 四舍五入保留两位小数
    printf("%.2f\n", round(result * 100) / 100);
    return 0;
}