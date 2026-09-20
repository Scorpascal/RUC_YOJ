#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

typedef struct { char c; int cnt; } Item;

static int cmp_item(const void *a, const void *b) {
    const Item *x = a;
    const Item *y = b;
    if (x->cnt != y->cnt) return y->cnt - x->cnt; // 按次数降序
    return x->c - y->c; // 次数相同时按字母升序
}

int main(void) {
    char s[512];
    if (!fgets(s, sizeof s, stdin)) return 0;
    size_t len = strlen(s);
    if (len > 0 && s[len-1] == '\n') s[len-1] = '\0';

    int cnt[26] = {0};
    for (size_t i = 0; s[i]; ++i) {
        if (isalpha((unsigned char)s[i])) {
            char lc = tolower((unsigned char)s[i]);
            cnt[lc - 'a']++;
        }
    }

    Item items[26];
    int m = 0;
    for (int i = 0; i < 26; ++i) {
        if (cnt[i] > 0) {
            items[m].c = 'a' + i;
            items[m].cnt = cnt[i];
            m++;
        }
    }

    qsort(items, m, sizeof(Item), cmp_item);

    for (int i = 0; i < m; ++i) {
        printf("%c %d\n", items[i].c, items[i].cnt);
    }
    return 0;
}