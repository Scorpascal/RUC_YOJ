#include <bits/stdc++.h>
using namespace std;

struct CustomHash {
    static uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    size_t operator()(uint64_t x) const {
        static const uint64_t FIXED_RANDOM =
            chrono::steady_clock::now().time_since_epoch().count();
        return splitmix64(x + FIXED_RANDOM);
    }
};

struct DSU {
    vector<int> fa;

    DSU(int n) : fa(n) {
        iota(fa.begin(), fa.end(), 0);
    }

    int find(int x) {
        // 迭代式路径压缩，避免很长的递归链导致栈溢出
        while (fa[x] != x) {
            fa[x] = fa[fa[x]];
            x = fa[x];
        }
        return x;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, q;
    cin >> n >> q;

    /*
        边界编号为 0..n。
        n+1 作为不存在下一跳时的哨兵。
    */
    const int INF = n + 1;

    vector<int> nxt(n + 2, INF);

    /*
        lastEvenOdd[v]:
        最大的偶数边界 x，使 O[x] = v

        lastOddEven[v]:
        最大的奇数边界 x，使 E[x] = v
    */
    unordered_map<uint32_t, int, CustomHash> lastEvenOdd;
    unordered_map<uint32_t, int, CustomHash> lastOddEven;

    lastEvenOdd.max_load_factor(0.7f);
    lastOddEven.max_load_factor(0.7f);

    lastEvenOdd.reserve(n / 2 + 16);
    lastOddEven.reserve(n / 2 + 16);

    uint32_t oddXor = 0;
    uint32_t evenXor = 0;

    // 边界 0 为偶数，O[0] = 0
    lastEvenOdd[0] = 0;

    /*
        mx 表示已经出现过的最大 p[y]。
        nxt[0..mx] 均已确定。
    */
    int mx = -1;

    for (int y = 1; y <= n; ++y) {
        uint32_t b;
        cin >> b;

        if (y & 1)
            oddXor ^= b;
        else
            evenXor ^= b;

        int p = -1;

        // x 为偶数，需要 O[x] = O[y]
        auto it1 = lastEvenOdd.find(oddXor);
        if (it1 != lastEvenOdd.end()) {
            p = max(p, it1->second);
        }

        // x 为奇数，需要 E[x] = E[y]
        auto it2 = lastOddEven.find(evenXor);
        if (it2 != lastOddEven.end()) {
            p = max(p, it2->second);
        }

        /*
            如果当前 p 创造了新的前缀最大值，
            那么所有还没有 nxt 的 x ∈ [mx+1, p]
            第一次找到合法的最早右端点 y。
        */
        if (p > mx) {
            for (int x = mx + 1; x <= p; ++x) {
                nxt[x] = y;
            }
            mx = p;
        }

        // 当前边界 y 插入哈希表，供未来位置使用
        if ((y & 1) == 0) {
            // y 为偶数，未来使用 O[y]
            lastEvenOdd[oddXor] = y;
        } else {
            // y 为奇数，未来使用 E[y]
            lastOddEven[evenXor] = y;
        }
    }

    nxt[n + 1] = n + 1;

    /*
        dep[x] = 从 x 沿 nxt 一直走到哨兵还需要多少条边。

        因为 nxt[x] > x，
        所以倒序即可。
    */
    vector<int> dep(n + 2, 0);

    dep[n + 1] = 0;

    for (int x = n; x >= 0; --x) {
        dep[x] = dep[nxt[x]] + 1;
    }

    /*
        将询问按照 R 分桶。

        head[R] 是右端点恰好为 R 的询问链表头。
    */
    vector<int> head(n + 1, -1);
    vector<int> queryStart(q);
    vector<int> queryNext(q);
    vector<int> answer(q);

    for (int i = 0; i < q; ++i) {
        int L, R;
        cin >> L >> R;

        // 真正使用的是左边界 L-1
        queryStart[i] = L - 1;

        queryNext[i] = head[R];
        head[R] = i;
    }

    DSU dsu(n + 2);

    /*
        由于 nxt[x] 单调不减，
        满足 nxt[x] <= R 的 x 一定形成一个前缀。
    */
    int ptr = 0;

    for (int R = 1; R <= n; ++R) {

        /*
            激活所有：
                x -> nxt[x]
            且 nxt[x] <= R 的边。
        */
        while (ptr <= n && nxt[ptr] <= R) {
            /*
                ptr 自己的父边此前尚未激活，
                所以 ptr 当前一定是这一棵并查集树的根。

                直接把它连向 nxt[ptr]。
            */
            dsu.fa[ptr] = nxt[ptr];
            ++ptr;
        }

        // 回答所有右端点为 R 的询问
        for (int id = head[R]; id != -1; id = queryNext[id]) {
            int A = queryStart[id];

            /*
                z 是从 A 出发，通过所有 nxt <= R 的边
                最终能到达的位置。
            */
            int z = dsu.find(A);

            answer[id] = dep[A] - dep[z];
        }
    }

    for (int i = 0; i < q; ++i) {
        cout << answer[i] << '\n';
    }

    return 0;
}