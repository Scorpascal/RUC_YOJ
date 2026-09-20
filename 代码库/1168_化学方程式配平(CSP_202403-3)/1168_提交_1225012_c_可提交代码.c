#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_M  50     // 题目给的上限安全冗余
#define MAX_E  60
#define NAME_LEN 64
#define BUF_LEN  256
#define EPS 1e-12L

static int find_or_add(char elems[][NAME_LEN], int *ecnt, const char *name) {
    for (int i = 0; i < *ecnt; ++i) {
        if (strcmp(elems[i], name) == 0) return i;
    }
    strcpy(elems[*ecnt], name);
    return (*ecnt)++;
}

static void parse_formula(const char *s, int col,
                          long double A[MAX_E][MAX_M],
                          char elems[][NAME_LEN], int *ecnt) {
    int n = (int)strlen(s), i = 0;
    while (i < n) {
        // 读元素名（连续小写字母）
        int j = i;
        while (j < n && islower((unsigned char)s[j])) ++j;
        char name[NAME_LEN];
        int len = j - i;
        if (len <= 0) return; // 防御
        memcpy(name, s + i, (size_t)len);
        name[len] = '\0';

        // 读数字（至少一位，题目保证1也不省略）
        int k = j, cnt = 0;
        while (k < n && isdigit((unsigned char)s[k])) {
            cnt = cnt * 10 + (s[k] - '0');
            ++k;
        }
        int idx = find_or_add(elems, ecnt, name);
        A[idx][col] += (long double)cnt;

        i = k;
    }
}

static void swap_rows(long double A[MAX_E][MAX_M], int r1, int r2, int cols) {
    if (r1 == r2) return;
    for (int c = 0; c < cols; ++c) {
        long double t = A[r1][c];
        A[r1][c] = A[r2][c];
        A[r2][c] = t;
    }
}

static int rank_of_matrix(long double A[MAX_E][MAX_M], int rows, int cols) {
    int r = 0;
    for (int c = 0; c < cols && r < rows; ++c) {
        // 选主元（绝对值最大）
        int p = r;
        long double best = fabsl(A[p][c]);
        for (int i = r + 1; i < rows; ++i) {
            long double v = fabsl(A[i][c]);
            if (v > best) { best = v; p = i; }
        }
        if (best < EPS) continue; // 本列全近似为0

        swap_rows(A, r, p, cols);

        // 消去其他行的该列
        for (int i = 0; i < rows; ++i) if (i != r) {
            long double factor = A[i][c] / A[r][c];
            if (fabsl(factor) < EPS) continue;
            for (int j = c; j < cols; ++j)
                A[i][j] -= factor * A[r][j];
        }
        ++r;
    }
    return r;
}

int main(void) {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    for (int cas = 0; cas < T; ++cas) {
        int m;
        if (scanf("%d", &m) != 1) return 0;

        char buf[BUF_LEN];
        char elems[MAX_E][NAME_LEN];
        int ecnt = 0;
        long double A[MAX_E][MAX_M] = {0};

        for (int i = 0; i < m; ++i) {
            if (scanf("%255s", buf) != 1) return 0;
            parse_formula(buf, i, A, elems, &ecnt);
        }

        int rk = rank_of_matrix(A, ecnt, m);
        puts(rk < m ? "Y" : "N");
    }
    return 0;
}