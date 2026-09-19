#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, N, q;
    cin >> n >> N >> q;

    const int total = n * N;

    // 每条缓存行用一个全局 idx 表示：idx = set * n + way
    vector<long long> blk(total, -1);   // 存的内存块编号，-1 表示空
    vector<char> dirty(total, 0);

    // LRU 双向链表（仅把“已装入”的行串起来），按 set 分开维护 head/tail
    vector<int> prv(total, -1), nxt(total, -1);
    vector<int> head(N, -1), tail(N, -1);

    auto detach = [&](int setId, int idx) {
        int p = prv[idx], nxi = nxt[idx];
        if (p != -1) nxt[p] = nxi; else head[setId] = nxi;
        if (nxi != -1) prv[nxi] = p; else tail[setId] = p;
        prv[idx] = nxt[idx] = -1;
    };

    auto attach_front = [&](int setId, int idx) {
        prv[idx] = -1;
        nxt[idx] = head[setId];
        if (head[setId] != -1) prv[head[setId]] = idx;
        else tail[setId] = idx; // 原来为空
        head[setId] = idx;
    };

    // 每个 set 的空闲行链表（只在“还没装满”时用）
    vector<int> freeHead(N, -1);
    vector<int> freeNext(total, -1);
    for (int s = 0; s < N; ++s) {
        int base = s * n;
        freeHead[s] = base;
        for (int w = 0; w < n - 1; ++w) freeNext[base + w] = base + w + 1;
        freeNext[base + (n - 1)] = -1;
    }

    auto pop_free = [&](int setId) -> int {
        int idx = freeHead[setId];
        freeHead[setId] = freeNext[idx];
        freeNext[idx] = -2; // 标记已不在空闲链
        return idx;
    };

    // 全局映射：block -> 缓存行 idx（总缓存行数 <= 65536，映射规模也 <= 65536）
    unordered_map<long long, int> loc;
    loc.reserve((size_t)total * 2);

    for (int i = 0; i < q; ++i) {
        int op;
        long long a;
        cin >> op >> a;

        // 题意映射：组号 = (a / n) % N
        int setId = (int)((a / n) % N);

        auto it = loc.find(a);
        if (it != loc.end()) {
            // 命中
            int idx = it->second;
            // 防御性检查：理论上不会跨组
            if (idx / n != setId) {
                // 若输入/实现异常，按未命中处理（但正常数据不会到这里）
                loc.erase(it);
            } else {
                detach(setId, idx);
                attach_front(setId, idx);
                if (op == 1) dirty[idx] = 1;
                continue;
            }
        }

        // 未命中：一定要从内存读入
        // 先决定用空闲行还是替换 LRU
        int idx;
        if (freeHead[setId] != -1) {
            idx = pop_free(setId);
        } else {
            // 替换 LRU（队尾）
            idx = tail[setId];
            long long old = blk[idx];

            // 若 dirty，先写回
            if (dirty[idx]) {
                cout << 1 << ' ' << old << '\n';
            }

            // 清理旧映射 & 从 LRU 中摘下
            loc.erase(old);
            detach(setId, idx);
        }

        // 读入新块
        cout << 0 << ' ' << a << '\n';

        // 装入并更新状态
        blk[idx] = a;
        dirty[idx] = (op == 1);
        loc[a] = idx;
        attach_front(setId, idx);
    }

    return 0;
}