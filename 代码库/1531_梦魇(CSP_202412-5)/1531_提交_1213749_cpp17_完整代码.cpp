#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <iostream>
#include <vector>
using namespace std;

struct FastIO {
    static const int S = 1 << 20;
    int idx, len; char buf[S];
    FastIO(): idx(0), len(0) {}
    inline char gc() {
        if (idx >= len) { len = fread(buf,1,S,stdin); idx = 0; }
        return (len ? buf[idx++] : 0);
    }
    template<class T> bool readInt(T &x){
        x=0; int f=1; char c=gc(); if(!c) return false;
        while(c!='-' && (c<'0'||c>'9')) { c=gc(); if(!c) return false; }
        if(c=='-'){ f=-1; c=gc(); }
        for(; c>='0'&&c<='9'; c=gc()) x = x*10 + (c-'0');
        x*=f; return true;
    }
} io;

static inline long long INFLL() { return (long long)4e18; }

// 全局工作区（按 n 初始化一次，后续复用）
static int N;
static vector<long long> A, B;      // 基础数组（输入）
static vector<long long> WA, WB;    // 工作数组（就地修改/还原）
static vector<long long> SB;        // 前缀和
static vector<int> L, R;            // 最近 >=
static vector<int> headL, nxtL, endL;
static vector<long long> wLarr;
static vector<int> endR;
static vector<long long> wR;
static vector<char> hasR;
static vector<int> stk;             // 单调栈（复用缓冲）
static vector<int> stEnd;           // 扫描时的区间栈
static vector<long long> stMax;

// 每次查询的核心计算：对当前 WA/WB 求答案（O(n)）
static long long solve_once() {
    int n = N;

    // 哨兵
    WA[0]=WA[n+1]=INFLL(); WB[0]=WB[n+1]=0;

    // 前缀和
    SB[0]=0;
    for (int i=1;i<=n;i++) SB[i]=SB[i-1]+WB[i];

    // 最近左 >=
    int top=0; stk[0]=0;
    for (int i=1;i<=n+1;i++){
        while (top>=0 && WA[stk[top]] < WA[i]) --top;
        L[i]=stk[top];
        stk[++top]=i;
    }
    // 最近右 >=
    top=0; stk[0]=n+1;
    for (int i=n;i>=0;i--){
        while (top>=0 && WA[stk[top]] < WA[i]) --top;
        R[i]=stk[top];
        stk[++top]=i;
    }

    // 建左侧区间表 headL（按 i 递增头插，使 end 递减）
    std::fill(headL.begin(), headL.end(), -1);
    int totL=0;
    for (int i=1;i<=n;i++){
        int s=L[i];
        ++totL;
        endL[totL]=i-1;
        wLarr[totL]= WA[i] - (SB[i-1]-SB[s]);
        nxtL[totL]=headL[s]; headL[s]=totL;
    }

    // 右侧唯一区间
    std::fill(hasR.begin(), hasR.end(), 0);
    for (int i=1;i<=n;i++){
        endR[i]=R[i]-1;
        wR[i]= WA[i] - (SB[endR[i]] - SB[i]);
        hasR[i]=1;
    }

    // 扫描所有间隙，维护当前覆盖的区间最大值（层叠区间 => 单调栈）
    stEnd.clear(); stMax.clear(); stEnd.reserve(64); stMax.reserve(64);
    auto push = [&](int e, long long w){
        long long cur = stMax.empty()? w : max(stMax.back(), w);
        stEnd.push_back(e); stMax.push_back(cur);
    };
    auto processStart = [&](int s){
        if (s>=1 && s<=n && hasR[s]) push(endR[s], wR[s]);
        for (int t=headL[s]; t!=-1; t=nxtL[t]) push(endL[t], wLarr[t]);
    };

    long long xr=0;
    processStart(0); // 起点在 0 的区间
    for (int j=1;j<=n-1;j++){
        while(!stEnd.empty() && stEnd.back() < j){ stEnd.pop_back(); stMax.pop_back(); }
        processStart(j);
        long long cur = stMax.empty()? min(WA[j], WA[j+1]) : stMax.back();
        xr ^= cur;
    }
    return xr;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (!io.readInt(N)) return 0;
    int n = N;

    // 仅分配一次内存
    A.assign(n+2, 0); B.assign(n+2, 0);
    WA.assign(n+2, 0); WB.assign(n+2, 0);
    SB.assign(n+1, 0);
    L.assign(n+2, 0); R.assign(n+2, 0);
    headL.assign(n+2, -1); nxtL.assign(n+1, -1); endL.assign(n+1, 0);
    wLarr.assign(n+1, 0);
    endR.assign(n+2, -1); wR.assign(n+2, 0);
    hasR.assign(n+2, 0);
    stk.assign(n+2, 0);
    stEnd.reserve(64); stMax.reserve(64);

    for (int i=1;i<=n;i++){ long long t; io.readInt(t); A[i]=t; }
    for (int i=1;i<=n;i++){ long long t; io.readInt(t); B[i]=t; }
    // 设置哨兵到基础数组，并把工作数组初始化为基础数组
    A[0]=A[n+1]=INFLL(); B[0]=B[n+1]=0;
    WA = A; WB = B;

    int q; io.readInt(q);

    for (int qi=0; qi<q; ++qi){
        int k; io.readInt(k);
        vector<int> changed; changed.reserve(k);
        // 读取修改并应用到工作数组（记录要还原的下标）
        for (int t=0;t<k;t++){
            int idx; long long na, nb;
            io.readInt(idx); io.readInt(na); io.readInt(nb);
            WA[idx]=na; WB[idx]=nb;
            changed.push_back(idx);
        }
        // 可能同一下标多次出现，去重以便还原
        sort(changed.begin(), changed.end());
        changed.erase(unique(changed.begin(), changed.end()), changed.end());

        long long res = solve_once();
        cout << res << '\n';

        // 还原到基础数组（保持工作数组始终为 A/B 的拷贝状态）
        for (int idx: changed){ WA[idx]=A[idx]; WB[idx]=B[idx]; }
    }
    return 0;
}