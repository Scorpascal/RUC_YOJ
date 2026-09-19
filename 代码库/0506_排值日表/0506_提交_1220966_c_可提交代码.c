#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cmp_str(const void *p, const void *q){
    return strcmp((const char*)p, (const char*)q);
}//十分巧妙,聪明的GPT-5 mini!!!

int main(void){
    int n;
    if (scanf("%d", &n) != 1 || n <= 0) return 0;
    char a[n][65], b[n][65];
    for (int i = 0; i < n; i++) {
        if (scanf("%64s", a[i]) != 1) a[i][0] = '\0';
    }
    for (int i = 0; i < n; i++) {
        if (scanf("%64s", b[i]) != 1) b[i][0] = '\0';
    }

    qsort(a, n, sizeof(a[0]), cmp_str);
    qsort(b, n, sizeof(b[0]), cmp_str);

    for (int i = 0; i < n; i++) {
        printf("%s %s\n", a[i], b[i]);
    }
    return 0;
}