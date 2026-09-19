#include <stdio.h>

int main() {
    int a, b;
    float x, y;
    char c1, c2;

    // 读取第一行，两个整数，中间逗号隔开
    scanf("%d,%d", &a, &b);

    // 读取第二行，两个浮点数，空格隔开
    scanf("%f %f", &x, &y);

    // 读取第三行，格式为" a, b"
    getchar(); // 吸收上一个输入后的换行符
    scanf(" %c, %c", &c1, &c2);

    // 输出
    printf("%d,%d\n", a, b);
    printf("%.1f %.1f\n", x, y);
    printf(" %c, %c\n", c1, c2);

    return 0;
}