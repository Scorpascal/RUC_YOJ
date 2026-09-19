#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <utility>

using namespace std;

static const int MOD = 998244353;
static inline int addmod(int a, int b){ a += b; if(a >= MOD) a -= MOD; return a; }
static inline int mulmod(long long a, long long b){ return int((a*b) % MOD); }

struct Node {
    int nxt[26];
    int fail;
    bool out_ccf;
    bool out_cspark;
    Node(): fail(0), out_ccf(false), out_cspark(false) {
        for(int i=0;i<26;i++) nxt[i] = -1;
    }
};

static inline int cid(char ch){ return ch - 'a'; }

struct ACAutomaton {
    vector<Node> t;
    ACAutomaton(){ t.emplace_back(); }

    void insert(const string& s, int which){
        int u = 0;
        for(char ch: s){
            int c = cid(ch);
            if(t[u].nxt[c] == -1){
                t[u].nxt[c] = (int)t.size();
                t.emplace_back();
            }
            u = t[u].nxt[c];
        }
        if(which == 1) t[u].out_ccf = true;
        else t[u].out_cspark = true;
    }

    void build(){
        queue<int> q;
        for(int c=0;c<26;c++){
            int v = t[0].nxt[c];
            if(v == -1) t[0].nxt[c] = 0;
            else { t[v].fail = 0; q.push(v); }
        }
        while(!q.empty()){
            int u = q.front(); q.pop();
            int f = t[u].fail;
            t[u].out_ccf = t[u].out_ccf || t[f].out_ccf;
            t[u].out_cspark = t[u].out_cspark || t[f].out_cspark;

            for(int c=0;c<26;c++){
                int v = t[u].nxt[c];
                if(v == -1) t[u].nxt[c] = t[f].nxt[c];
                else { t[v].fail = t[f].nxt[c]; q.push(v); }
            }
        }
    }
};

// flag: 0=未见ccf, 1=见过ccf但未达成, 2=已达成(吸收)
static inline int nextFlag(int flag, bool out_ccf, bool out_cspark){
    if(flag == 2) return 2;
    if(flag == 1) return out_cspark ? 2 : 1;
    return out_ccf ? 1 : 0;
}

static const int MAXN = 30;

struct Mat {
    int n;
    int a[MAXN][MAXN];
    Mat(int n_=0, bool ident=false): n(n_) {
        for(int i=0;i<MAXN;i++) for(int j=0;j<MAXN;j++) a[i][j] = 0;
        if(ident){
            for(int i=0;i<n;i++) a[i][i] = 1;
        }
    }
};

static inline Mat mulMat(const Mat& A, const Mat& B){
    Mat C(A.n);
    int n = A.n;
    for(int i=0;i<n;i++){
        for(int k=0;k<n;k++){
            int aik = A.a[i][k];
            if(!aik) continue;
            long long x = aik;
            for(int j=0;j<n;j++){
                int bkj = B.a[k][j];
                if(!bkj) continue;
                C.a[i][j] = (C.a[i][j] + x * bkj) % MOD;
            }
        }
    }
    return C;
}

// Y = A * X，其中 X 是 n×3（X[state][col]）
static inline array<array<int,3>, MAXN> mulMat3(const Mat& A, const array<array<int,3>, MAXN>& X){
    int n = A.n;
    array<array<int,3>, MAXN> Y;
    for(int i=0;i<MAXN;i++) Y[i] = {0,0,0};
    for(int i=0;i<n;i++){
        for(int k=0;k<n;k++){
            int aik = A.a[i][k];
            if(!aik) continue;
            long long x = aik;
            Y[i][0] = (Y[i][0] + x * X[k][0]) % MOD;
            Y[i][1] = (Y[i][1] + x * X[k][1]) % MOD;
            Y[i][2] = (Y[i][2] + x * X[k][2]) % MOD;
        }
    }
    return Y;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    long long n;
    int m;
    cin >> n >> m;
    vector<long long> a(m);
    for(int i=0;i<m;i++) cin >> a[i];

    // AC
    ACAutomaton ac;
    ac.insert("ccf", 1);
    ac.insert("cspark", 2);
    ac.build();

    int S = (int)ac.t.size();   // <= 10
    int N = S * 3;              // <= 30

    auto id = [&](int flag, int node){ return flag * S + node; };

    // 8 类字母：a,c,f,k,p,r,s,other(19)，other 用 'b' 代表
    int repChar[8] = {cid('a'), cid('c'), cid('f'), cid('k'), cid('p'), cid('r'), cid('s'), cid('b')};
    int catCnt[8]  = {1,1,1,1,1,1,1,19};

    // edges[from] = {(to, weight)}
    vector<vector<pair<int,int>>> edges(N);
    for(int flag=0; flag<3; flag++){
        for(int node=0; node<S; node++){
            int from = id(flag,node);
            int toTmp[8], wTmp[8];
            for(int k=0;k<8;k++){
                int c = repChar[k];
                int node2 = ac.t[node].nxt[c];
                int flag2 = nextFlag(flag, ac.t[node2].out_ccf, ac.t[node2].out_cspark);
                toTmp[k] = id(flag2, node2);
                wTmp[k] = catCnt[k];
            }
            vector<pair<int,int>> v;
            v.reserve(8);
            for(int k=0;k<8;k++){
                int to = toTmp[k], w = wTmp[k];
                bool ok = false;
                for(auto &pr: v){
                    if(pr.first == to){ pr.second += w; ok = true; break; }
                }
                if(!ok) v.push_back({to,w});
            }
            for(auto &pr: v) pr.second %= MOD;
            edges[from] = std::move(v);
        }
    }

    // 一步矩阵 M（列向量 vec' = M * vec）
    Mat M(N);
    for(int from=0; from<N; from++){
        for(auto &e: edges[from]){
            int to = e.first, w = e.second;
            M.a[to][from] = addmod(M.a[to][from], w);
        }
    }

    // 小段预处理阈值
    const int T = 200000;
    vector<array<int,9>> Bsmall(T+1);
    for(int s=0;s<3;s++) for(int f=0;f<3;f++) Bsmall[0][s*3+f] = (s==f);

    array<array<int,3>, MAXN> D, D2;
    for(int i=0;i<MAXN;i++) D[i] = {0,0,0};
    for(int s=0;s<3;s++) D[id(s,0)][s] = 1;

    auto collapseToB = [&](array<int,9>& outB, const array<array<int,3>, MAXN>& curD){
        for(int s=0;s<3;s++){
            for(int f=0;f<3;f++){
                long long sum = 0;
                int base = id(f,0);
                for(int node=0; node<S; node++){
                    sum += curD[base + node][s];
                }
                outB[s*3+f] = int(sum % MOD);
            }
        }
    };

    for(int len=1; len<=T; len++){
        for(int i=0;i<MAXN;i++) D2[i] = {0,0,0};
        for(int from=0; from<N; from++){
            auto cur = D[from];
            if(cur[0]==0 && cur[1]==0 && cur[2]==0) continue;
            for(auto &e: edges[from]){
                int to = e.first, w = e.second;
                long long ww = w;
                D2[to][0] = (D2[to][0] + ww * cur[0]) % MOD;
                D2[to][1] = (D2[to][1] + ww * cur[1]) % MOD;
                D2[to][2] = (D2[to][2] + ww * cur[2]) % MOD;
            }
        }
        D = D2;
        collapseToB(Bsmall[len], D);
    }

    // M^(2^k)
    vector<Mat> pw;
    pw.reserve(32);
    pw.push_back(M);
    for(int k=1;k<32;k++) pw.push_back(mulMat(pw[k-1], pw[k-1]));

    unordered_map<long long, array<int,9>> cache;
    cache.reserve(8192);
    cache.max_load_factor(0.7f);

    auto getB = [&](long long L)->array<int,9>{
        if(L <= T) return Bsmall[(int)L];
        auto it = cache.find(L);
        if(it != cache.end()) return it->second;

        array<array<int,3>, MAXN> X;
        for(int i=0;i<MAXN;i++) X[i] = {0,0,0};
        for(int s=0;s<3;s++) X[id(s,0)][s] = 1;

        long long x = L;
        int bit = 0;
        while(x){
            if(x & 1LL) X = mulMat3(pw[bit], X);
            x >>= 1LL;
            bit++;
        }
        array<int,9> B;
        collapseToB(B, X);
        cache.emplace(L, B);
        return B;
    };

    // 切段
    vector<long long> seg;
    seg.reserve(m+1);
    long long prev = 0;
    for(int i=0;i<m;i++){
        long long len = a[i] - prev - 1;
        if(len < 0) len = 0;
        seg.push_back(len);
        prev = a[i];
    }
    seg.push_back(n - prev);

    // 全局 dp[flag]
    array<int,3> dp = {1,0,0};
    for(long long L: seg){
        auto B = getB(L);
        array<int,3> ndp = {0,0,0};
        for(int s=0;s<3;s++){
            if(!dp[s]) continue;
            long long ds = dp[s];
            ndp[0] = (ndp[0] + ds * B[s*3+0]) % MOD;
            ndp[1] = (ndp[1] + ds * B[s*3+1]) % MOD;
            ndp[2] = (ndp[2] + ds * B[s*3+2]) % MOD;
        }
        dp = ndp;
    }

    cout << dp[2] << "\n";
    return 0;
}