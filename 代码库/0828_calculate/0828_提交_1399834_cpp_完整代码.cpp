#include <stdio.h>
#include <math.h>

int main() {
    // 1. 递归分数
    double y1 = 1 + 1.0 / (1 + 1.0 / (1 + 1.0 / 5));

    // 2. 勾股定理
    double y2 = sqrt(3 * 3 + 4 * 4);

    // 3. 三角函数
    double y3 = sqrt((1 - cos(M_PI / 3)) / 2);

    // 4. 复合三角表达式
    double s = sin(M_PI / 4);
    double c = cos(M_PI / 4);
    double y4 = 2 * s * s + s * c - c * c;

    // 5. 根号与加法
    double y5 = 2 * sqrt(5) * (sqrt(6) + sqrt(3)) / (6 + 3);

    // 6. 对数与三角
    double y6 = (log(5) * log(3) - log(2)) / sin(M_PI / 3);

    printf("%.3f\n", y1);
    printf("%.3f\n", y2);
    printf("%.3f\n", y3);
    printf("%.3f\n", y4);
    printf("%.3f\n", y5);
    printf("%.3f\n", y6);

    return 0;
}