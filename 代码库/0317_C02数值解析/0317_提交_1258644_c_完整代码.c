#include <stdio.h>

int main(void) {
    long long sum = 0, x;
    char dump[1001]; // 输入总长<=1000，预留1个结尾符

    // 若一开始就是数字，先读入这个数字
    if (scanf("%lld", &x) == 1) {
        sum += x;
    }

    // 之后反复读取：一段非数字 + 一个数字
    while (scanf("%1000[^0-9]%lld", dump, &x) == 2) {
        sum += x;
    }

    printf("%lld\n", sum);
    return 0;
}