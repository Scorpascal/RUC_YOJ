#include <stdio.h>
#include <string.h>

#define MAXS 200005
#define SIGMA 63
#define LOGK 32

int idx[256];
char rev[SIGMA];
int nxt[LOGK][SIGMA];

static void build_alphabet() {
    for (int i = 0; i < 256; ++i) idx[i] = -1;
    int p = 0;
    for (char c = 'A'; c <= 'Z'; ++c) { idx[(unsigned char)c] = p; rev[p++] = c; }
    for (char c = 'a'; c <= 'z'; ++c) { idx[(unsigned char)c] = p; rev[p++] = c; }
    for (char c = '0'; c <= '9'; ++c) { idx[(unsigned char)c] = p; rev[p++] = c; }
    idx[(unsigned char)' '] = p; rev[p++] = ' ';
}

int main() {
    build_alphabet();

    char s[MAXS];
    // 读初始字符串（被 # 包裹）
    if (scanf(" #%[^#]#", s) != 1) return 0;

    int n;
    scanf("%d", &n);

    // 初始映射：未定义时保持不变
    for (int i = 0; i < SIGMA; ++i) nxt[0][i] = i;

    // 读 n 条替换规则 #xy#
    for (int i = 0; i < n; ++i) {
        char x, y;
        scanf(" #%c%c#", &x, &y);
        int xi = idx[(unsigned char)x];
        int yi = idx[(unsigned char)y];
        if (xi >= 0 && yi >= 0) nxt[0][xi] = yi;
    }

    // 倍增预处理：nxt[p][i] 表示应用 2^p 次后的字符索引
    for (int p = 1; p < LOGK; ++p) {
        for (int i = 0; i < SIGMA; ++i) {
            nxt[p][i] = nxt[p-1][ nxt[p-1][i] ];
        }
    }

    int m;
    scanf("%d", &m);

    for (int qi = 0; qi < m; ++qi) {
        unsigned int k;  // 题中 k ≤ 1e9
        scanf("%u", &k);

        char out[MAXS];
        size_t len = strlen(s);
        for (size_t i = 0; i < len; ++i) {
            int cur = idx[(unsigned char)s[i]];
            // 理论上输入保证字符属于集合
            for (int p = 0; p < LOGK; ++p) {
                if (k & (1u << p)) cur = nxt[p][cur];
            }
            out[i] = rev[cur];
        }
        out[len] = '\0';
        printf("#%s#\n", out);
    }
    return 0;
}