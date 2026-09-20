#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int cmp(const void *a, const void *b) {
    return *(int*)b - *(int*)a; // 降序
}

int N;
int a[65];
int used[65];
int total;
int target;
int groups;

bool dfs(int formedGroups, int startIdx, int currentSum) {
    if (formedGroups == groups - 1) return true; // 剩下的必然构成最后一组
    if (currentSum == target) return dfs(formedGroups + 1, 0, 0);
    int prevTried = -1;
    for (int i = startIdx; i < N; ++i) {
        if (used[i]) continue;
        int val = a[i];
        if (val == prevTried) continue; // 重复剪枝
        if (currentSum + val > target) continue;
        used[i] = 1;
        if (dfs(formedGroups, i + 1, currentSum + val)) return true;
        used[i] = 0;
        prevTried = val;
        if (currentSum == 0) return false; // 第一块失败，剪枝
        if (currentSum + val == target) return false; // 刚好凑满失败则剪枝
    }
    return false;
}

int main(void) {
    if (scanf("%d", &N) != 1) return 0;
    total = 0;
    int maxv = 0;
    for (int i = 0; i < N; ++i) {
        scanf("%d", &a[i]);
        total += a[i];
        if (a[i] > maxv) maxv = a[i];
    }
    qsort(a, N, sizeof(int), cmp);
    for (target = maxv; target <= total; ++target) {
        if (total % target != 0) continue;
        groups = total / target;
        for (int i = 0; i < N; ++i) used[i] = 0;
        if (dfs(0, 0, 0)) {
            printf("%d\n", target);
            return 0;
        }
    }
    // 理论上不会到这里
    printf("%d\n", total);
    return 0;
}