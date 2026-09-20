#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int sum_digits(const char *s) {
    int sum = 0;
    for (const char *p = s; *p; ++p) {
        if (isdigit((unsigned char)*p)) sum += *p - '0';
    }
    return sum;
}

static int cmp_numstr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    size_t la = strlen(sa), lb = strlen(sb);
    if (la != lb) return (la < lb) ? -1 : 1;
    return strcmp(sa, sb);
}

static char *dupstr(const char *s) {
    size_t len = strlen(s);
    char *p = (char *)malloc(len + 1);
    if (p) memcpy(p, s, len + 1);
    return p;
}

int main(void) {
    int n, k;
    if (scanf("%d %d", &n, &k) != 2) return 0;

    char **ok = (char **)malloc(sizeof(char *) * (size_t)n);
    int m = 0;

    char buf[256];
    for (int i = 0; i < n; ++i) {
        if (scanf("%255s", buf) != 1) return 0;
        if (sum_digits(buf) % k == 0) {
            ok[m++] = dupstr(buf);
        }
    }

    qsort(ok, (size_t)m, sizeof(char *), cmp_numstr);

    printf("%d\n", m);
    for (int i = 0; i < m; ++i) {
        printf("%s\n", ok[i]);
        free(ok[i]);
    }
    free(ok);
    return 0;
}