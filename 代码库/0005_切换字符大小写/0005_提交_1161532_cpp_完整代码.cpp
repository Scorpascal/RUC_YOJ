#include <stdio.h>
#include <ctype.h>

int main() {
    char ch;
    scanf("%c", &ch);
    if (isupper(ch)) {
        printf("%c\n", tolower(ch));
    } else if (islower(ch)) {
        printf("%c\n", toupper(ch));
    } else {
        printf("%c\n", ch); // 非字母原样输出
    }
    return 0;
}