#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef long long i64;
typedef unsigned long long u64;

typedef struct { int to, w, next; } Edge;

typedef struct {
    int node;
    u64 mask;
    unsigned char pc;   // mask中1的个数（节点数）
    i64 dist;
} State;

typedef struct {
    State* a;
    int n, cap;
} Vec;

static void vec_init(Vec* v){ v->a=NULL; v->n=0; v->cap=0; }
static void vec_reserve(Vec* v, int need){
    if(need<=v->cap) return;
    int nc = v->cap? v->cap:256;
    while(nc < need) nc <<= 1;
    v->a = (State*)realloc(v->a, (size_t)nc*sizeof(State));
    v->cap = nc;
}
static void vec_push(Vec* v, State s){ vec_reserve(v, v->n+1); v->a[v->n++] = s; }
static void vec_clear(Vec* v){ v->n = 0; }

static int cmp_node_mask(const void* pa, const void* pb){
    const State* a=(const State*)pa; const State* b=(const State*)pb;
    if(a->node != b->node) return a->node - b->node;
    if(a->mask < b->mask) return -1;
    if(a->mask > b->mask) return 1;
    // 同key按dist降序，方便去重时取第一个
    if(a->dist < b->dist) return 1;
    if(a->dist > b->dist) return -1;
    return 0;
}

// 对v(按node,mask排序)去重：同(node,mask)保留dist最大的那条
static void dedup(Vec* v){
    if(v->n==0) return;
    qsort(v->a, (size_t)v->n, sizeof(State), cmp_node_mask);
    int w=0;
    for(int i=0;i<v->n;){
        int j=i+1;
        while(j<v->n && v->a[j].node==v->a[i].node && v->a[j].mask==v->a[i].mask) j++;
        // 因为已按dist降序，i就是最大
        v->a[w] = v->a[i];
        // 保险起见，pc按mask重算一次
        v->a[w].pc = (unsigned char)__builtin_popcountll(v->a[w].mask);
        w++;
        i=j;
    }
    v->n = w;
}

// 记录每个节点的区间起止索引，便于按节点分组遍历
static void build_node_index(const Vec* v, int N, int* start, int* end){
    for(int i=0;i<N;i++){ start[i]=-1; end[i]=-1; }
    for(int i=0;i<v->n;i++){
        int u = v->a[i].node;
        if(start[u]==-1) start[u]=i;
        end[u]=i+1;
    }
}

// 从start出发，最多走maxStep条边，构造所有到达状态(按node,mask去重并排序)
static void build_half_states(
    int N, int maxStep, int startNode,
    const int* head, const Edge* E,
    const int* C, Vec* out)
{
    Vec frontier, next, all;
    vec_init(&frontier); vec_init(&next); vec_init(&all);

    State s0 = { startNode, 1ULL << C[startNode], 1, 0 };
    vec_push(&frontier, s0);
    vec_push(&all, s0);

    for(int step=0; step<maxStep; step++){
        vec_clear(&next);
        for(int i=0;i<frontier.n;i++){
            State s = frontier.a[i];
            for(int e=head[s.node]; e!=-1; e=E[e].next){
                int v = E[e].to;
                u64 bit = 1ULL << C[v];
                if(s.mask & bit) continue; // 颜色重复，禁止
                State t = { v, s.mask | bit, (unsigned char)(s.pc+1), s.dist + E[e].w };
                vec_push(&next, t);
            }
        }
        if(next.n==0) break;
        dedup(&next);          // 当层去重
        // 累加到总体集合
        for(int i=0;i<next.n;i++) vec_push(&all, next.a[i]);
        // 下一层
        // 直接交换frontier与next
        Vec tmp = frontier; frontier = next; next = tmp;
    }

    // 总体集合去重并输出
    dedup(&all);
    *out = all;

    // 释放临时
    free(frontier.a);
    free(next.a);
}

// 读取整型数组
static void read_ints(int* a, int n){
    for(int i=0;i<n;i++) scanf("%d", &a[i]);
}

int main(){
    // 输入：N M L K
    int N, M, L, K;
    if(scanf("%d %d %d %d", &N, &M, &L, &K)!=4) return 0;

    int* C = (int*)malloc(sizeof(int)*N);
    read_ints(C, N);

    int* U = (int*)malloc(sizeof(int)*M);
    int* V = (int*)malloc(sizeof(int)*M);
    int* D = (int*)malloc(sizeof(int)*M);
    read_ints(U, M);
    read_ints(V, M);
    read_ints(D, M);

    // 建邻接表(正向与反向)
    Edge* Eout = (Edge*)malloc(sizeof(Edge)*M);
    Edge* Ein  = (Edge*)malloc(sizeof(Edge)*M);
    int* headOut = (int*)malloc(sizeof(int)*N);
    int* headIn  = (int*)malloc(sizeof(int)*N);
    for(int i=0;i<N;i++){ headOut[i]=-1; headIn[i]=-1; }
    for(int i=0;i<M;i++){
        int u=U[i], v=V[i], w=D[i];
        Eout[i].to=v; Eout[i].w=w; Eout[i].next=headOut[u]; headOut[u]=i;
        Ein[i].to=u;  Ein[i].w=w;  Ein[i].next=headIn[v];  headIn[v]=i;
    }

    // 折半深度
    int maxEdges = (L>=1? L-1: 0);
    if(maxEdges < 0) maxEdges = 0;
    if(maxEdges > 8) maxEdges = 8; // 因为L≤9
    int leftStep  = maxEdges/2;
    int rightStep = maxEdges - leftStep;

    // 前向、后向状态
    Vec F, B;
    build_half_states(N, leftStep, 0, headOut, Eout, C, &F);
    build_half_states(N, rightStep, N-1, headIn,  Ein,  C, &B);

    // 按节点建立索引
    int* Fst = (int*)malloc(sizeof(int)*N);
    int* Fed = (int*)malloc(sizeof(int)*N);
    int* Bst = (int*)malloc(sizeof(int)*N);
    int* Bed = (int*)malloc(sizeof(int)*N);
    build_node_index(&F, N, Fst, Fed);
    build_node_index(&B, N, Bst, Bed);

    // 合并计算答案
    i64 ans = -1;
    for(int v=0; v<N; v++){
        int i1 = Fst[v], i2 = Fed[v];
        int j1 = Bst[v], j2 = Bed[v];
        if(i1==-1 || j1==-1) continue;
        u64 vbit = 1ULL << C[v];
        for(int i=i1; i<i2; i++){
            const State* s = &F.a[i];
            for(int j=j1; j<j2; j++){
                const State* t = &B.a[j];
                if( (s->mask & t->mask) != vbit ) continue; // 仅在v的颜色处相交
                int totalNodes = (int)s->pc + (int)t->pc - 1;
                if(totalNodes > L) continue;
                i64 cand = s->dist + t->dist;
                if(cand > ans) ans = cand;
            }
        }
    }

    // 输出
    printf("%lld\n", ans);

    // 释放
    free(C); free(U); free(V); free(D);
    free(Eout); free(Ein); free(headOut); free(headIn);
    free(F.a); free(B.a);
    free(Fst); free(Fed); free(Bst); free(Bed);
    return 0;
}