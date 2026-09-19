#include <stdio.h>

#define MAXN 500000
#define INFN (MAXN + 5)

/* 读入 */
static inline int rd() {
    int c = getchar_unlocked(), x = 0, s = 1;
    while (c != '-' && (c < '0' || c > '9')) c = getchar_unlocked();
    if (c == '-') { s = -1; c = getchar_unlocked(); }
    while (c >= '0' && c <= '9') { x = x * 10 + (c - '0'); c = getchar_unlocked(); }
    return x * s;
}

/* 树状数组：区间加、点查 */
int N;
int bit[INFN];
static inline void bit_add(int i, int v){ for(; i<=N; i+=i&-i) bit[i]+=v; }
static inline void range_add(int l,int r,int v){ bit_add(l,v); bit_add(r+1,-v); }
static inline int bit_sum(int i){ int s=0; for(; i>0; i-=i&-i) s+=bit[i]; return s; }

/* 原树欧拉序（独立于动态结构） */
int head0[INFN], nxt0[INFN];
int tin[INFN], tout[INFN], dep0[INFN];

void euler_build(int n){
    for(int i=1;i<=n;i++) head0[i]=0;
}
static inline void add_edge0(int p,int v){
    nxt0[v]=head0[p]; head0[p]=v;
}
void euler_dfs_iter(int root){
    static int stk[INFN], cur[INFN];
    int top=0, tim=0;
    dep0[root]=1; stk[++top]=root; cur[root]=head0[root]; tin[root]=++tim;
    while(top){
        int u=stk[top];
        if(cur[u]){
            int v=cur[u]; cur[u]=nxt0[v];
            dep0[v]=dep0[u]+1; tin[v]=++tim;
            stk[++top]=v; cur[v]=head0[v];
        }else{
            tout[u]=tim; top--;
        }
    }
}

/* 动态结构：孩子单链表 + 尾指针 + 孩子数 */
int head[INFN], tail[INFN], nxt[INFN], cnt[INFN];
int fa[INFN];
long long d[INFN];

static inline void add_child(int p,int v){
    if(!head[p]) head[p]=tail[p]=v;
    else { nxt[tail[p]]=v; tail[p]=v; }
    cnt[p]++;
}

int main(){
    int n = rd(), m = rd();
    N = n;

    fa[1]=0;
    for(int i=2;i<=n;i++) fa[i]=rd();

    for(int i=1;i<=n;i++){ long long x = rd(); d[i]=x; }

    /* 构建动态孩子链表 */
    for(int i=1;i<=n;i++){ head[i]=tail[i]=0; nxt[i]=0; cnt[i]=0; }
    for(int i=2;i<=n;i++) add_child(fa[i], i);

    /* 构建原树欧拉序（用独立邻接，避免后续修改影响） */
    euler_build(n);
    for(int i=2;i<=n;i++) add_edge0(fa[i], i);
    euler_dfs_iter(1);

    while(m--){
        int op = rd(), x = rd();
        if(op==2){
            int ans = dep0[x] + bit_sum(tin[x]);
            printf("%d\n", ans);
        }else{
            /* 合并文件夹 x：删除其“当前所有直接子节点” */
            long long addData = 0;
            int newH = 0, newT = 0, newC = 0;

            int y = head[x];
            head[x]=tail[x]=0; cnt[x]=0;        /* 清空 x 原有孩子，准备接收合并结果 */

            while(y){
                int ny = nxt[y];                /* 记录下一个兄弟（仅遍历旧孩子） */
                addData += d[y];

                /* 把 y 的孩子整段拼到 x 的新孩子链 */
                if(head[y]){
                    if(!newH){ newH = head[y]; newT = tail[y]; }
                    else{ nxt[newT] = head[y]; newT = tail[y]; }
                    newC += cnt[y];
                }

                /* 深度整体 -1：删除 y 对其原子树的影响 */
                range_add(tin[y], tout[y], -1);

                /* y 被删除后不再使用，其孩子已被移动 */
                // 可选清理：head[y]=tail[y]=0; cnt[y]=0;

                y = ny;
            }

            head[x]=newH; tail[x]=newT; cnt[x]=newC;
            d[x] += addData;

            printf("%d %lld\n", cnt[x], d[x]);
        }
    }
    return 0;
}