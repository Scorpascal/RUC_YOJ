#include <bits/stdc++.h>
using namespace std;

using u32 = uint32_t;
using u64 = uint64_t;
using i64 = int64_t;

constexpr u32 MOD = 998244353;

// n <= 1e7，因此出现 p^k, k >= 2 时必有 p <= sqrt(1e7) < 3163
constexpr int MAXP = 3163;
constexpr int MAXK = 24;

/*
    五个积性函数：

    f : 所有 C 形阵的价值和
    s : C=B（或 E=B）时的价值和
    h : C=E 时的价值和
    q : CE=B^2 时的价值和
    j : C^2 E=B^3（或 CE^2=B^3）时的价值和
*/
struct Value {
    u32 f, s, h, q, j;
};

struct Local {
    u32 f = 0, s = 0, h = 0, q = 0, j = 0;
    int ipow = 0;       // 真正的 p^k，用来求去掉 p^k 后的部分
};

// 只有 k >= 2 才使用这里
static Local cacheLocal[MAXP][MAXK];

inline u32 mul_mod(u32 a, u32 b) {
    return (u64)a * b % MOD;
}

// 计算 p^k 对五个积性函数的局部贡献
Local& getLocal(int p, int k) {
    Local &L = cacheLocal[p][k];

    if (L.ipow != 0)
        return L;

    // p^0 ... p^(3k) (mod MOD)
    u32 pw[70];
    pw[0] = 1;

    for (int i = 1; i <= 3 * k; ++i)
        pw[i] = (u64)pw[i - 1] * p % MOD;

    // 真正的整数 p^k
    int ip = 1;
    for (int i = 0; i < k; ++i)
        ip *= p;

    L.ipow = ip;

    // ----------------------------------------------------
    // F(p^k)
    //
    // sum_{r=0}^{k-1} (k+r+1)p^r
    // +
    // sum_{r=k}^{3k} (3k-r+1)p^r
    // ----------------------------------------------------
    u64 f = 0;

    for (int r = 0; r < k; ++r) {
        f += (u64)(k + r + 1) * pw[r];
    }

    for (int r = k; r <= 3 * k; ++r) {
        f += (u64)(3 * k - r + 1) * pw[r];
    }

    L.f = f % MOD;

    // ----------------------------------------------------
    // S(p^k) = 1 + p + ... + p^(2k)
    // ----------------------------------------------------
    u64 s = 0;

    for (int r = 0; r <= 2 * k; ++r)
        s += pw[r];

    L.s = s % MOD;

    // ----------------------------------------------------
    // H(p^k) = p^(3k) + p^(3k-2) + ...
    // ----------------------------------------------------
    u64 h = 0;

    for (int r = 3 * k; r >= 0; r -= 2)
        h += pw[r];

    L.h = h % MOD;

    // ----------------------------------------------------
    // K(p^k) = (2k+1) p^k
    // ----------------------------------------------------
    L.q = (u64)(2 * k + 1) * pw[k] % MOD;

    // ----------------------------------------------------
    // J(p^k)
    // = sum p^r,
    //   ceil(k/2) <= r <= floor(3k/2)
    // ----------------------------------------------------
    u64 j = 0;

    int l = (k + 1) / 2;
    int r = 3 * k / 2;

    for (int e = l; e <= r; ++e)
        j += pw[e];

    L.j = j % MOD;

    return L;
}

// ------------------------------------------------------------
// op = 0
// 只需要维护 F，节省大量内存和常数
// ------------------------------------------------------------
u32 solve0(int n) {

    if (n == 1)
        return 1;

    vector<int> lp(n + 1);
    vector<uint8_t> cnt(n + 1);
    vector<u32> f(n + 1);

    vector<int> primes;
    primes.reserve(700000);

    f[1] = 1;

    u32 ans = 1;    // F(1)=1

    for (int i = 2; i <= n; ++i) {

        // i 是质数
        if (lp[i] == 0) {

            lp[i] = i;
            cnt[i] = 1;
            primes.push_back(i);

            u64 p = i;
            u64 p2 = p * p % MOD;
            u64 p3 = p2 * p % MOD;

            // F(p) = p^3 + 2p^2 + 3p + 2
            f[i] = (p3 + 2 * p2 + 3 * p + 2) % MOD;

            ans += f[i];
            if (ans >= MOD)
                ans -= MOD;
        }

        for (int p : primes) {

            i64 xx = (i64)i * p;
            if (xx > n)
                break;

            int x = (int)xx;
            lp[x] = p;

            if (p == lp[i]) {

                // p 的指数增加 1
                int k = cnt[i] + 1;
                cnt[x] = (uint8_t)k;

                Local &L = getLocal(p, k);

                // x = rest * p^k, gcd(rest,p)=1
                int rest = x / L.ipow;

                f[x] = (u64)f[rest] * L.f % MOD;

                ans += f[x];
                if (ans >= MOD)
                    ans -= MOD;

                // 线性筛的关键
                break;

            } else {

                // p 不整除 i，所以 x=i*p
                cnt[x] = 1;

                // 积性
                f[x] = (u64)f[i] * f[p] % MOD;

                ans += f[x];
                if (ans >= MOD)
                    ans -= MOD;
            }
        }
    }

    return ans;
}

// ------------------------------------------------------------
// 当前 b 的完美阵价值
//
// P(b) = F - 2S - H - K - 2J + 5b
// ------------------------------------------------------------
inline u32 perfectValue(const Value &v, int b) {

    i64 res = v.f;

    res -= 2LL * v.s;
    res -= v.h;
    res -= v.q;
    res -= 2LL * v.j;
    res += 5LL * b;

    res %= MOD;

    if (res < 0)
        res += MOD;

    return (u32)res;
}

// ------------------------------------------------------------
// op = 1
// ------------------------------------------------------------
u32 solve1(int n) {

    if (n == 1)
        return 0;

    vector<int> lp(n + 1);
    vector<uint8_t> cnt(n + 1);
    vector<Value> val(n + 1);

    vector<int> primes;
    primes.reserve(700000);

    val[1] = {1, 1, 1, 1, 1};

    // b=1 不可能完美
    u32 ans = 0;

    for (int i = 2; i <= n; ++i) {

        // ------------------------------------------------
        // i 为质数，k=1 的局部函数可以直接写公式
        // ------------------------------------------------
        if (lp[i] == 0) {

            lp[i] = i;
            cnt[i] = 1;
            primes.push_back(i);

            u64 p = i;
            u64 p2 = p * p % MOD;
            u64 p3 = p2 * p % MOD;

            val[i].f = (p3 + 2 * p2 + 3 * p + 2) % MOD;
            val[i].s = (p2 + p + 1) % MOD;
            val[i].h = (p3 + p) % MOD;
            val[i].q = 3 * p % MOD;
            val[i].j = p % MOD;

            u32 cur = perfectValue(val[i], i);

            ans += cur;
            if (ans >= MOD)
                ans -= MOD;
        }

        // ------------------------------------------------
        // Euler 线性筛
        // ------------------------------------------------
        for (int p : primes) {

            i64 xx = (i64)i * p;

            if (xx > n)
                break;

            int x = (int)xx;
            lp[x] = p;

            // p 已经是 i 的最小质因子
            if (p == lp[i]) {

                int k = cnt[i] + 1;
                cnt[x] = (uint8_t)k;

                Local &L = getLocal(p, k);

                // x = rest * p^k
                int rest = x / L.ipow;

                val[x].f = (u64)val[rest].f * L.f % MOD;
                val[x].s = (u64)val[rest].s * L.s % MOD;
                val[x].h = (u64)val[rest].h * L.h % MOD;
                val[x].q = (u64)val[rest].q * L.q % MOD;
                val[x].j = (u64)val[rest].j * L.j % MOD;

                u32 cur = perfectValue(val[x], x);

                ans += cur;
                if (ans >= MOD)
                    ans -= MOD;

                break;

            } else {

                // gcd(i,p)=1，直接使用积性
                cnt[x] = 1;

                val[x].f = (u64)val[i].f * val[p].f % MOD;
                val[x].s = (u64)val[i].s * val[p].s % MOD;
                val[x].h = (u64)val[i].h * val[p].h % MOD;
                val[x].q = (u64)val[i].q * val[p].q % MOD;
                val[x].j = (u64)val[i].j * val[p].j % MOD;

                u32 cur = perfectValue(val[x], x);

                ans += cur;
                if (ans >= MOD)
                    ans -= MOD;
            }
        }
    }

    return ans;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int op, n;
    cin >> op >> n;

    if (op == 0)
        cout << solve0(n) << '\n';
    else
        cout << solve1(n) << '\n';

    return 0;
}