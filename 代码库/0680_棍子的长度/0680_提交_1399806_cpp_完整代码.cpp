#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int n, stick[12], used[12], total, target, m;

int cmp(const void *a, const void *b) {
    return *(int*)b - *(int*)a; // 降序
}

// 尝试拼第cur根大棍，当前已拼长度len，从第idx根小棍开始选
int dfs(int cur, int len, int idx) {
    if (cur > m) return 1; // 拼完所有大棍
    if (len == target) // 当前大棍拼完，拼下一根
        return dfs(cur + 1, 0, 0);
    int last = -1;
    for (int i = idx; i < n; ++i) {
        if (!used[i] && len + stick[i] <= target && stick[i] != last) {
            used[i] = 1;
            if (dfs(cur, len + stick[i], i + 1)) return 1;
            used[i] = 0;
            if (len == 0 || len + stick[i] == target) return 0; // 剪枝
            last = stick[i];
        }
    }
    return 0;
}

int main() {
    scanf("%d", &n);
    total = 0;
    for (int i = 0; i < n; ++i) {
        scanf("%d", &stick[i]);
        total += stick[i];
    }
    qsort(stick, n, sizeof(int), cmp);
    for (target = stick[0]; target <= total; ++target) {
        if (total % target != 0) continue;
        m = total / target;
        memset(used, 0, sizeof(used));
        if (dfs(1, 0, 0)) {
            printf("%d\n", target);
            break;
        }
    }
    return 0;
}