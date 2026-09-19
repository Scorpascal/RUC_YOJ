#include <stdio.h>
#include <stdlib.h>

typedef long long ll;

typedef struct {
    int d;     // deadline
    ll w;      // penalty if late
} Job;

int cmp_job_desc_w(const void* a, const void* b) {
    const Job* x = (const Job*)a;
    const Job* y = (const Job*)b;
    if (x->w < y->w) return 1;
    if (x->w > y->w) return -1;
    return x->d - y->d; // 次要按截止时间升序
}

int find(int* parent, int x) {
    if (x <= 0) return 0;
    if (parent[x] == x) return x;
    parent[x] = find(parent, parent[x]);
    return parent[x];
}

int main(void) {
    ll m;
    int n;
    if (scanf("%lld", &m) != 1) return 0;
    if (scanf("%d", &n) != 1) return 0;

    Job* jobs = (Job*)malloc(sizeof(Job) * n);
    for (int i = 0; i < n; ++i) scanf("%d", &jobs[i].d);
    for (int i = 0; i < n; ++i) {
        ll w;
        scanf("%lld", &w);
        jobs[i].w = w;
        if (jobs[i].d < 0) jobs[i].d = 0;
        // 题面保证 1 <= ti <= n，但为稳妥处理越界
        if (jobs[i].d > n) jobs[i].d = n;
    }

    // 按扣款从大到小排序
    qsort(jobs, n, sizeof(Job), cmp_job_desc_w);

    // 并查集：parent[t] 表示 t 这个时段是否空；占用后 union 到 t-1
    int* parent = (int*)malloc(sizeof(int) * (n + 1));
    for (int i = 0; i <= n; ++i) parent[i] = i;

    ll total_penalty = 0;
    ll saved_penalty = 0;

    for (int i = 0; i < n; ++i) {
        total_penalty += jobs[i].w;
        int t = jobs[i].d;
        int slot = find(parent, t); // 找到不超过 d 的最晚空闲时段
        if (slot > 0) {
            // 占用该时段
            parent[slot] = find(parent, slot - 1);
            saved_penalty += jobs[i].w;
        }
        // 否则无法按时完成，产生扣款（计入 total_penalty - saved_penalty）
    }

    ll result = m - (total_penalty - saved_penalty);
    if (result < 0) result = 0; // 按题意“不会赔钱”，稳妥起见做下限保护

    printf("%lld\n", result);

    free(jobs);
    free(parent);
    return 0;
}