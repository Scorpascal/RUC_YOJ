#include <stdio.h>
#include <stdint.h>

#define MAXN 1005

int n, m;
int64_t A[MAXN][MAXN], B[MAXN][MAXN];

void print128(__int128 x){
    if(x<0){ putchar('-'); x=-x; }
    if(x>=10) print128(x/10);
    putchar((int)(x%10)+'0');
}

static __int128 sq(int64_t v){ return (__int128)v * v; }

int main(){
    if(scanf("%d%d",&n,&m)!=2) return 0;
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            scanf("%lld",&A[i][j]);
    for(int i=0;i<m;i++)
        for(int j=0;j<m;j++)
            scanf("%lld",&B[i][j]);

    // 原始主/副对角线平方和
    __int128 orig_main = 0, orig_anti = 0;
    for(int i=0;i<n;i++){
        orig_main += sq(A[i][i]);
        orig_anti += sq(A[i][n-1-i]);
    }

    // 主对角线前缀 (i,i)
    static __int128 preMain[MAXN+1];
    for(int i=0;i<n;i++) preMain[i+1] = preMain[i] + sq(A[i][i]);

    // 副对角线前缀 (i, n-1-i)
    static __int128 preAnti[MAXN+1];
    for(int i=0;i<n;i++) preAnti[i+1] = preAnti[i] + sq(A[i][n-1-i]);

    // 预处理 B：差 (i-j) 与 和 (i+j) 的平方和
    int diffSize = 2*m - 1;
    static __int128 sumB_diff[2*MAXN]; // index shift: + (m-1)
    static __int128 sumB_sum [2*MAXN]; // index: i+j
    for(int i=0;i<m;i++){
        for(int j=0;j<m;j++){
            __int128 s = sq(B[i][j]);
            sumB_diff[i - j + (m-1)] += s;
            sumB_sum [i + j]          += s;
        }
    }

    __int128 ans = -((__int128)1<<126);

    int limit = n - m;
    for(int x=0; x<=limit; x++){
        for(int y=0; y<=limit; y++){
            // main
            __int128 mainVal = orig_main;
            int delta = y - x;
            if(delta < 0 ? -delta < m : delta < m){ // |delta| < m -> 有交集
                int len = m - (delta<0?-delta:delta);
                int t_start = x > y ? x : y;
                int t_end = t_start + len - 1;
                __int128 remA = preMain[t_end+1] - preMain[t_start];
                __int128 addB = sumB_diff[delta + (m-1)];
                mainVal = orig_main - remA + addB;
            }

            // anti
            __int128 antiVal = orig_anti;
            int T = (n - 1) - (x + y); // 需要的 i+j
            if(0 <= T && T <= 2*m - 2){
                // 计算 A 副对角线交集区间 t (行号)：
                // 条件: x <= t <= x+m-1
                //       y <= n-1 - t <= y+m-1  => n-1 - (y+m-1) <= t <= n-1 - y
                int low = x > (n - y - m) ? x : (n - y - m);
                int high = (x + m -1) < (n -1 - y) ? (x + m -1) : (n -1 - y);
                if(low <= high){
                    __int128 remA = preAnti[high+1] - preAnti[low];
                    __int128 addB = sumB_sum[T];
                    antiVal = orig_anti - remA + addB;
                }
            }

            __int128 diff = mainVal - antiVal;
            if(diff > ans) ans = diff;
        }
    }

    print128(ans);
    putchar('\n');
    return 0;
}