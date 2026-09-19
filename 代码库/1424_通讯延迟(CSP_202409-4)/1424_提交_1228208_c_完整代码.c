#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef long long i64;

typedef struct { i64 x, y; } Pt;
typedef struct { i64 x; int id; } XEntry;

typedef struct { i64 d; int id; } HeapItem;
typedef struct {
    HeapItem *a; int sz, cap;
} MinHeap;

static void heap_init(MinHeap *h, int cap){
    h->sz = 0; h->cap = cap>0?cap:1;
    h->a = (HeapItem*)malloc(sizeof(HeapItem)*h->cap);
}
static void heap_swap(HeapItem *x, HeapItem *y){ HeapItem t=*x;*x=*y;*y=t; }
static int heap_less(HeapItem a, HeapItem b){ return a.d < b.d; }
static void heap_push(MinHeap *h, HeapItem v){
    if(h->sz+1 >= h->cap){
        h->cap <<= 1;
        h->a = (HeapItem*)realloc(h->a, sizeof(HeapItem)*h->cap);
    }
    int i = ++h->sz;
    h->a[i] = v;
    while(i>1 && heap_less(h->a[i], h->a[i>>1])){
        heap_swap(&h->a[i], &h->a[i>>1]); i>>=1;
    }
}
static int heap_empty(MinHeap *h){ return h->sz==0; }
static HeapItem heap_pop(MinHeap *h){
    HeapItem ret = h->a[1];
    h->a[1] = h->a[h->sz--];
    int i=1;
    while(1){
        int l=i<<1, r=l+1, m=i;
        if(l<=h->sz && heap_less(h->a[l], h->a[m])) m=l;
        if(r<=h->sz && heap_less(h->a[r], h->a[m])) m=r;
        if(m==i) break;
        heap_swap(&h->a[i], &h->a[m]); i=m;
    }
    return ret;
}

static int cmpX(const void* A, const void* B){
    const XEntry *a=(const XEntry*)A, *b=(const XEntry*)B;
    if(a->x < b->x) return -1;
    if(a->x > b->x) return 1;
    return 0;
}
static int lower_bound_x(XEntry *arr, int n, i64 v){
    int l=0, r=n;
    while(l<r){
        int m=(l+r)>>1;
        if(arr[m].x >= v) r=m; else l=m+1;
    }
    return l;
}
static int upper_bound_x(XEntry *arr, int n, i64 v){
    int l=0, r=n;
    while(l<r){
        int m=(l+r)>>1;
        if(arr[m].x > v) r=m; else l=m+1;
    }
    return l;
}

int main(){
    int n, m;
    if(scanf("%d %d", &n, &m)!=2) return 0;
    Pt *node = (Pt*)malloc(sizeof(Pt)*(n+1));
    for(int i=1;i<=n;i++){
        long long x,y; scanf("%lld %lld",&x,&y);
        node[i].x=x; node[i].y=y;
    }
    Pt *bs = (Pt*)malloc(sizeof(Pt)*(m+1));
    i64 *rad = (i64*)malloc(sizeof(i64)*(m+1));
    i64 *del = (i64*)malloc(sizeof(i64)*(m+1));
    for(int j=1;j<=m;j++){
        long long x,y,r,t; scanf("%lld %lld %lld %lld",&x,&y,&r,&t);
        bs[j].x=x; bs[j].y=y; rad[j]=r; del[j]=t;
    }

    // 为基站 -> 节点的 0 边做 x 方向二分的索引
    XEntry *xs = (XEntry*)malloc(sizeof(XEntry)*n);
    for(int i=0;i<n;i++){ xs[i].x = node[i+1].x; xs[i].id = i+1; }
    qsort(xs, n, sizeof(XEntry), cmpX);

    int V = n + m;
    const i64 INF = (i64)4e18;
    i64 *dist = (i64*)malloc(sizeof(i64)*(V+1));
    char *vis = (char*)calloc(V+1,1);
    for(int i=1;i<=V;i++) dist[i]=INF;
    dist[1]=0;

    MinHeap h; heap_init(&h, 1<<12);
    heap_push(&h, (HeapItem){0,1});

    while(!heap_empty(&h)){
        HeapItem it = heap_pop(&h);
        int u = it.id; i64 du = it.d;
        if(vis[u]) continue;
        if(du!=dist[u]) continue;
        vis[u]=1;
        if(u==n) break; // 已得到最短路

        if(u<=n){
            // 节点 -> 所有包含它的基站（代价 t_j）
            i64 xi=node[u].x, yi=node[u].y;
            for(int j=1;j<=m;j++){
                // 点在正方形内: |x - xj|<=r && |y - yj|<=r
                if(llabs(xi - bs[j].x) <= rad[j] && llabs(yi - bs[j].y) <= rad[j]){
                    int v = n + j;
                    i64 nd = du + del[j];
                    if(nd < dist[v]){
                        dist[v]=nd;
                        heap_push(&h, (HeapItem){nd, v});
                    }
                }
            }
        }else{
            // 基站 -> 覆盖内的所有节点（0 代价）
            int j = u - n;
            i64 L = bs[j].x - rad[j], R = bs[j].x + rad[j];
            int Lid = lower_bound_x(xs, n, L);
            int Rid = upper_bound_x(xs, n, R);
            for(int k=Lid;k<Rid;k++){
                int i = xs[k].id;
                if(llabs(node[i].y - bs[j].y) <= rad[j]){
                    if(du < dist[i]){
                        dist[i]=du;
                        heap_push(&h, (HeapItem){du, i});
                    }
                }
            }
        }
    }

    if(dist[n] >= INF/2) {
        puts("Nan");
    } else {
        printf("%lld\n", dist[n]);
    }

    // 释放
    free(node); free(bs); free(rad); free(del);
    free(xs); free(dist); free(vis); free(h.a);
    return 0;
}