#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAXN 3050

typedef long long ll;
typedef struct { ll cost; int id; } Cand;

int cmpCostThenId(const void *a, const void *b){
    const Cand *x = (const Cand*)a;
    const Cand *y = (const Cand*)b;
    if (x->cost < y->cost) return -1;
    if (x->cost > y->cost) return 1;
    if (x->id   < y->id)   return -1;
    if (x->id   > y->id)   return 1;
    return 0;
}
int cmpId(const void *a, const void *b){
    const Cand *x = (const Cand*)a;
    const Cand *y = (const Cand*)b;
    if (x->id < y->id) return -1;
    if (x->id > y->id) return 1;
    return 0;
}
int lower_bound_ll(const ll *arr, int n, ll key){
    int l=0, r=n;
    while(l<r){
        int m = l + (r-l)/2;
        if (arr[m] < key) l = m+1;
        else r = m;
    }
    return l;
}

int main(){
    ll s[3];
    if (scanf("%lld%lld%lld",&s[0],&s[1],&s[2])!=3) return 0;
    int n;
    if (scanf("%d",&n)!=1) return 0;

    // 三个批次分别的候选场（每个场只能接一个批次）
    Cand cand[8][MAXN];
    int cnt[8] = {0};

    for(int i=0;i<n;i++){
        int id, status;
        ll cap, base, per;
        if (scanf("%d%d%lld%lld%lld",&id,&status,&cap,&base,&per)!=5) return 0;
        if (status==1) continue; // 运行中不可用
        if (cap >= s[0]){ cand[1][cnt[1]].cost = base + per * s[0]; cand[1][cnt[1]].id = id; cnt[1]++; }
        if (cap >= s[1]){ cand[2][cnt[2]].cost = base + per * s[1]; cand[2][cnt[2]].id = id; cnt[2]++; }
        if (cap >= s[2]){ cand[4][cnt[4]].cost = base + per * s[2]; cand[4][cnt[4]].id = id; cnt[4]++; }
    }

    if (cnt[1]==0 || cnt[2]==0 || cnt[4]==0){
        printf("NO\n");
        return 0;
    }

    // 按成本升序（成本相同按 id 升序），用于求 best
    qsort(cand[1], cnt[1], sizeof(Cand), cmpCostThenId);
    qsort(cand[2], cnt[2], sizeof(Cand), cmpCostThenId);
    qsort(cand[4], cnt[4], sizeof(Cand), cmpCostThenId);

    // cand[4] 的前三个全局最小成本候选（排除两个 id 至多跳过 2 个，第三个一定可用）
    int top4 = cnt[4] < 3 ? cnt[4] : 3;

    const ll INFLL = (1LL<<62);
    ll best = INFLL;

    // O(n1*n2) 计算最小总成本（第三项用 cand4 的前三名规避冲突）
    for(int i=0;i<cnt[1];i++){
        for(int j=0;j<cnt[2];j++){
            if (cand[1][i].id == cand[2][j].id) continue;
            ll c3 = -1;
            for(int t=0;t<top4;t++){
                int idc = cand[4][t].id;
                if (idc!=cand[1][i].id && idc!=cand[2][j].id){
                    c3 = cand[4][t].cost;
                    break;
                }
            }
            if (c3<0) continue; // cand4 不足以规避 id 冲突
            ll tot = cand[1][i].cost + cand[2][j].cost + c3;
            if (tot < best) best = tot;
        }
    }

    if (best==INFLL){
        printf("NO\n");
        return 0;
    }

    // 为了按字典序最小输出方案：对 cand1、cand2 按 id 升序遍历；
    // 对 cand4 按“成本值 -> 最小三个 id”建索引，快速找满足 need 的最小可用 id
    // 1) 构建 cand4 的成本分组索引
    static ll  costKey[MAXN];
    static int costIdA[MAXN], costIdB[MAXN], costIdC[MAXN];
    int keyCnt = 0;
    for(int i=0;i<cnt[4];i++){
        if (keyCnt==0 || cand[4][i].cost != costKey[keyCnt-1]){
            costKey[keyCnt] = cand[4][i].cost;
            costIdA[keyCnt] = cand[4][i].id;
            costIdB[keyCnt] = -1;
            costIdC[keyCnt] = -1;
            keyCnt++;
        }else{
            if (costIdB[keyCnt-1] == -1) costIdB[keyCnt-1] = cand[4][i].id;
            else if (costIdC[keyCnt-1] == -1) costIdC[keyCnt-1] = cand[4][i].id;
            // 只需前三小 id，足以排除两次冲突
        }
    }

    // 2) cand1、cand2 按 id 升序数组
    static Cand c1ById[MAXN], c2ById[MAXN];
    for(int i=0;i<cnt[1];i++) c1ById[i] = cand[1][i];
    for(int j=0;j<cnt[2];j++) c2ById[j] = cand[2][j];
    qsort(c1ById, cnt[1], sizeof(Cand), cmpId);
    qsort(c2ById, cnt[2], sizeof(Cand), cmpId);

    int ans1=-1, ans2=-1, ans3=-1;
    int found = 0;
    for(int i=0;i<cnt[1] && !found;i++){
        for(int j=0;j<cnt[2] && !found;j++){
            if (c1ById[i].id == c2ById[j].id) continue;
            ll need = best - c1ById[i].cost - c2ById[j].cost;
            // 二分找到 need
            int pos = lower_bound_ll(costKey, keyCnt, need);
            if (pos>=keyCnt || costKey[pos]!=need) continue;
            // 在该成本组挑最小可用 id（排除两次冲突）
            int kId = -1;
            int a = costIdA[pos], b = costIdB[pos], c = costIdC[pos];
            if (a!=-1 && a!=c1ById[i].id && a!=c2ById[j].id) kId = a;
            else if (b!=-1 && b!=c1ById[i].id && b!=c2ById[j].id) kId = b;
            else if (c!=-1 && c!=c1ById[i].id && c!=c2ById[j].id) kId = c;
            if (kId==-1) continue; // 该成本组只有冲突 id
            // 找到字典序最小三元组
            ans1 = c1ById[i].id;
            ans2 = c2ById[j].id;
            ans3 = kId;
            found = 1;
        }
    }

    // 理论上一定能找到（因为 best 来自可行三元组）
    if (!found){
        // 保险起见，回退输出 NO
        printf("NO\n");
        return 0;
    }

    printf("%lld\n", best);
    printf("%d %d %d\n", ans1, ans2, ans3);
    return 0;
}