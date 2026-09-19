#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef long long i64;

typedef struct {
    char name[128];
    int size;
    int idx;      // 输入顺序，用于稳定排序
    i64 cnt;
} Entry;

static void rtrim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}
static char* lskip(char *s) {
    while (*s && isspace((unsigned char)*s)) ++s;
    return s;
}

static int get_size(const char *name) {
    // 只要出现 '*' 就视为指针，8 字节
    for (const char *p = name; *p; ++p) {
        if (*p == '*') return 8;
    }
    if (strcmp(name, "char") == 0) return 1;
    if (strcmp(name, "bool") == 0) return 1;
    if (strcmp(name, "short") == 0) return 2;
    if (strcmp(name, "int") == 0) return 4;
    if (strcmp(name, "float") == 0) return 4;
    if (strcmp(name, "long") == 0) return 4;
    if (strcmp(name, "long long") == 0) return 8;
    if (strcmp(name, "double") == 0) return 8;
    if (strcmp(name, "long double") == 0) return 16;
    // 兜底：未知类型按指针处理（题面不会用到）
    return 8;
}

// 稳定：按 size 降序，其次按输入顺序升序
static int cmp_desc(const void *a, const void *b) {
    const Entry *x = (const Entry*)a;
    const Entry *y = (const Entry*)b;
    if (x->size != y->size) return y->size - x->size;
    return x->idx - y->idx;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    int ch;
    // 吃掉行尾
    while ((ch = getchar()) != '\n' && ch != EOF) {}

    Entry arr[64];
    int m = 0, nextIdx = 0;

    char line[256];
    for (int i = 0; i < n; ++i) {
        if (!fgets(line, sizeof(line), stdin)) return 0;
        // 允许空行
        char *p = lskip(line);
        while (*p == '\0' || *p == '\n') {
            if (!fgets(line, sizeof(line), stdin)) return 0;
            p = lskip(line);
        }
        rtrim(p);

        // 找到最后一个空白，后面是数量
        int len = (int)strlen(p);
        int pos = len - 1;
        while (pos >= 0 && !isspace((unsigned char)p[pos])) pos--;
        if (pos < 0) return 0; // 输入不合法

        i64 cnt = strtoll(p + pos + 1, NULL, 10);
        p[pos] = '\0';
        rtrim(p); // 去掉类型名尾部空白

        // 合并相同类型名（完全相同的字符串）
        int found = -1;
        for (int j = 0; j < m; ++j) {
            if (strcmp(arr[j].name, p) == 0) { found = j; break; }
        }
        if (found >= 0) {
            arr[found].cnt += cnt;
        } else {
            strncpy(arr[m].name, p, sizeof(arr[m].name)-1);
            arr[m].name[sizeof(arr[m].name)-1] = '\0';
            arr[m].size = get_size(arr[m].name);
            arr[m].idx = nextIdx++;
            arr[m].cnt = cnt;
            m++;
        }
    }

    // 计算总字节和最大成员大小
    i64 sum = 0;
    int maxSize = 1;
    for (int i = 0; i < m; ++i) {
        sum += arr[i].cnt * arr[i].size;
        if (arr[i].size > maxSize) maxSize = arr[i].size;
    }
    // 结果大小：向上取到 maxSize 的倍数
    i64 S = ((sum + maxSize - 1) / maxSize) * maxSize;

    // 生成一种达到最小的排列：按大小降序（2 的幂保证无内部填充）
    qsort(arr, m, sizeof(Entry), cmp_desc);

    // 输出
    printf("%lld\n", S);
    for (int i = 0; i < m; ++i) {
        if (arr[i].cnt > 0) {
            printf("%s %lld\n", arr[i].name, arr[i].cnt);
        }
    }
    return 0;
}