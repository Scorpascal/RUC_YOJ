#include <stdio.h>
#include <string.h>

int main(void) {
    char s1[128], s2[128];
    if (!fgets(s1, sizeof(s1), stdin)) return 0;
    if (!fgets(s2, sizeof(s2), stdin)) return 0;
    s1[strcspn(s1, "\r\n")] = 0;
    s2[strcspn(s2, "\r\n")] = 0;

    int n = strlen(s1), m = strlen(s2);
    if (m > n) { printf("false\n"); return 0; }

    char rot[128];
    for (int k = 0; k < n; ++k) {
        // 右循环移位 k 次：把最后 k 个字符放到前面
        if (k == 0) {
            strcpy(rot, s1);
        } else {
            int start = n - k;
            strcpy(rot, s1 + start);
            strncat(rot, s1, start);
        }
        if (strstr(rot, s2)) { printf("true\n"); return 0; }
    }

    printf("false\n");
    return 0;
}