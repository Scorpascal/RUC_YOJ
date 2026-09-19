#include <stdio.h>
int main(void){
    char s[8];
    if (scanf("%4s", s) != 1) return 0;
    printf("%c %c %c %c\n", s[0], s[1], s[2], s[3]);
    return 0;
}