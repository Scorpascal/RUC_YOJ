#include <stdio.h>
#include <string.h>

int main() {
    char s[5];
    scanf("%4s", s); // 读取最多4位数字
    int len = strlen(s);

    // 按个十百千顺序输出
    for (int i = len - 1; i >= 0; i--) {
        printf("%c", s[i]);
        if (i > 0) printf(",");
    }
    printf(",%d\n", len);

    return 0;
}