#include <stdio.h>
#include <string.h>

int main(void) {
    char s1[1024], s2[1024];
    if (!fgets(s1, sizeof s1, stdin)) return 0;
    if (!fgets(s2, sizeof s2, stdin)) return 0;
    // 去掉行尾换行符
    size_t n = strlen(s1);
    if (n > 0 && s1[n-1] == '\n') s1[n-1] = '\0';
    n = strlen(s2);
    if (n > 0 && s2[n-1] == '\n') s2[n-1] = '\0';

    size_t i = 0;
    while (s1[i] && s2[i] && s1[i] == s2[i]) i++;
    int diff = (int)(unsigned char)s1[i] - (int)(unsigned char)s2[i];
    printf("%d\n", diff);
    return 0;
}