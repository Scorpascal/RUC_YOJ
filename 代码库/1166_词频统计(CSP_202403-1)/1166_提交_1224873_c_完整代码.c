#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    int n, m;
    if (scanf("%d %d", &n, &m) != 2) return 0;

    int *doc_cnt = (int *)calloc(m + 1, sizeof(int));   // xi：包含该词的文章数
    int *total_cnt = (int *)calloc(m + 1, sizeof(int)); // yi：该词总出现次数
    if (!doc_cnt || !total_cnt) return 0;

    for (int i = 0; i < n; ++i) {
        int l;
        if (scanf("%d", &l) != 1) { free(doc_cnt); free(total_cnt); return 0; }

        unsigned char *seen = (unsigned char *)calloc(m + 1, 1); // 本篇是否出现过
        if (!seen) { free(doc_cnt); free(total_cnt); return 0; }

        for (int j = 0; j < l; ++j) {
            int w;
            if (scanf("%d", &w) != 1) { free(seen); free(doc_cnt); free(total_cnt); return 0; }
            if (w >= 1 && w <= m) {
                total_cnt[w]++;             // 统计总出现次数
                if (!seen[w]) {             // 首次在本篇出现
                    seen[w] = 1;
                    doc_cnt[w]++;
                }
            }
        }
        free(seen);
    }

    for (int i = 1; i <= m; ++i) {
        printf("%d %d\n", doc_cnt[i], total_cnt[i]);
    }

    free(doc_cnt);
    free(total_cnt);
    return 0;
}