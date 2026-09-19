#include <stdio.h>
#include <ctype.h>

int main(void) {
    int ch;
    int letters = 0;

    while ((ch = getchar()) != EOF) {
        putchar(ch);
        if (isalpha((unsigned char)ch)) {
            letters++;
            if (letters == 3) break;
        }
    }
    return 0;
}