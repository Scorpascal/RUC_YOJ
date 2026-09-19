#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void chomp(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';
    }
}

static char* sdup(const char* s) {
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    if (!r) exit(1);
    memcpy(r, s, n + 1);
    return r;
}

static size_t count_nonoverlap(const char* s, const char* p) {
    size_t ls = strlen(s), lp = strlen(p);
    if (lp == 0) return 0;
    size_t i = 0, cnt = 0;
    while (i + lp <= ls) {
        if (memcmp(s + i, p, lp) == 0) {
            cnt++;
            i += lp;
        } else {
            i++;
        }
    }
    return cnt;
}

static char* delete_first_occurrence(const char* s, const char* p) {
    size_t lp = strlen(p);
    if (lp == 0) return sdup(s);
    const char* pos = strstr(s, p);
    if (!pos) return sdup(s);
    size_t pre = (size_t)(pos - s);
    size_t ls = strlen(s);
    char* out = (char*)malloc(ls - lp + 1);
    if (!out) exit(1);
    memcpy(out, s, pre);
    memcpy(out + pre, pos + lp, ls - pre - lp);
    out[ls - lp] = '\0';
    return out;
}

static char* insert_before_last_occurrence(const char* s, const char* p, const char* ins) {
    size_t lp = strlen(p);
    if (lp == 0) return sdup(s);
    const char* last = NULL;
    const char* cur = s;
    while ((cur = strstr(cur, p)) != NULL) {
        last = cur;
        cur = cur + 1; // 允许重叠，向后滑动1
    }
    if (!last) return sdup(s);
    size_t ls = strlen(s), li = strlen(ins);
    size_t pre = (size_t)(last - s);
    char* out = (char*)malloc(ls + li + 1);
    if (!out) exit(1);
    memcpy(out, s, pre);
    memcpy(out + pre, ins, li);
    memcpy(out + pre + li, s + pre, ls - pre);
    out[ls + li] = '\0';
    return out;
}

static char* replace_all_nonrecursive(const char* s, const char* p, const char* rep) {
    size_t lp = strlen(p);
    if (lp == 0) return sdup(s);
    size_t cnt = count_nonoverlap(s, p);
    if (cnt == 0) return sdup(s);
    size_t ls = strlen(s), lr = strlen(rep);
    size_t out_len = ls - cnt * lp + cnt * lr;
    char* out = (char*)malloc(out_len + 1);
    if (!out) exit(1);

    size_t i = 0, w = 0;
    while (i < ls) {
        if (i + lp <= ls && memcmp(s + i, p, lp) == 0) {
            memcpy(out + w, rep, lr);
            w += lr;
            i += lp;
        } else {
            out[w++] = s[i++];
        }
    }
    out[w] = '\0';
    return out;
}

int main(void) {
    char s[1005];
    char cmd[1005];

    if (!fgets(s, sizeof(s), stdin)) return 0;
    if (!fgets(cmd, sizeof(cmd), stdin)) return 0;
    chomp(s);
    chomp(cmd);

    char op = 0;
    char a[1005] = {0}, b[1005] = {0};
    // 先尝试解析三段命令
    int n = sscanf(cmd, " %c %1000s %1000s", &op, a, b);
    if (n < 2) return 0;

    if (op == 'C') {
        // 统计子串（无重叠）
        size_t cnt = count_nonoverlap(s, a);
        printf("%zu\n", cnt);
    } else if (op == 'D') {
        char* out = delete_first_occurrence(s, a);
        printf("%s\n", out);
        free(out);
    } else if (op == 'I') {
        if (n < 3) { printf("%s\n", s); return 0; }
        char* out = insert_before_last_occurrence(s, a, b);
        printf("%s\n", out);
        free(out);
    } else if (op == 'R') {
        if (n < 3) { printf("%s\n", s); return 0; }
        char* out = replace_all_nonrecursive(s, a, b);
        printf("%s\n", out);
        free(out);
    } else {
        // 未知操作，按原串输出
        printf("%s\n", s);
    }
    return 0;
}