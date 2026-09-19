#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int cmp_str(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

static char *lower_dup(const char *s) {
    size_t n = strlen(s);
    char *t = (char *)malloc(n + 1);
    for (size_t i = 0; i < n; ++i) t[i] = (char)tolower((unsigned char)s[i]);
    t[n] = '\0';
    return t;
}

// 分组跳重：arr[idx..k) 同值，返回下一组起点 k
static int next_group(char **arr, int len, int idx) {
    int k = idx + 1;
    while (k < len && strcmp(arr[k], arr[idx]) == 0) ++k;
    return k;
}

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    char buf[32];
    char **a = (char **)malloc(sizeof(char *) * n);
    char **b = (char **)malloc(sizeof(char *) * m);

    for (int i = 0; i < n; ++i) {
        if (scanf("%31s", buf) != 1) return 0;
        a[i] = lower_dup(buf);
    }
    for (int i = 0; i < m; ++i) {
        if (scanf("%31s", buf) != 1) return 0;
        b[i] = lower_dup(buf);
    }

    qsort(a, n, sizeof(char *), cmp_str);
    qsort(b, m, sizeof(char *), cmp_str);

    int i = 0, j = 0, inter = 0, uni = 0;
    while (i < n && j < m) {
        int ni = next_group(a, n, i);
        int nj = next_group(b, m, j);
        int c = strcmp(a[i], b[j]);
        if (c == 0) { ++inter; ++uni; i = ni; j = nj; }
        else if (c < 0) { ++uni; i = ni; }
        else { ++uni; j = nj; }
    }
    while (i < n) { ++uni; i = next_group(a, n, i); }
    while (j < m) { ++uni; j = next_group(b, m, j); }

    printf("%d\n%d\n", inter, uni);

    for (int k = 0; k < n; ++k) free(a[k]);
    for (int k = 0; k < m; ++k) free(b[k]);
    free(a); free(b);
    return 0;
}