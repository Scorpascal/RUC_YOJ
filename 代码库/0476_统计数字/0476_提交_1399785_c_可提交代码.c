#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    char line[1000];
    fgets(line, sizeof(line), stdin);
    // 移除换行符
    line[strcspn(line, "\n")] = 0;
    int int_count = 0, float_count = 0;
    int i = 0;
    while (line[i] != '\0') {
        if (isdigit(line[i])) {
            // 开始数字序列
            int start = i;
            while (isdigit(line[i])) i++;
            if (line[i] == '.') {
                // 检查后面是否是数字
                i++;
                if (isdigit(line[i])) {
                    while (isdigit(line[i])) i++;
                    float_count++;
                } else {
                    // 不是浮点数，回退
                    i = start;
                    while (isdigit(line[i])) i++;
                    int_count++;
                }
            } else {
                int_count++;
            }
        } else {
            i++;
        }
    }
    printf("%d %d\n", int_count, float_count);
    return 0;
}