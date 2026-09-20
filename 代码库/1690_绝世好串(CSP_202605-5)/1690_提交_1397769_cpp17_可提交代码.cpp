#define NDEBUG

#include <cstdio>
#include <vector>
#include <algorithm>
#include <set>
#include <deque>
#include <numeric>
#include <cstdint>
#include <utility>
#include <cassert>

// 如果仍然 Compile TLE，再取消下一行注释：
#pragma GCC optimize("O1")

using namespace std;

static const int INF = 1000000000;

int n;
vector<vector<int> > adj;

struct TreeIndex {
    int n, timer, LOG;
    vector<int> parent, depth, sub, heavy;
    vector<int> top, tin, tout;
    vector<vector<int> > up;

    TreeIndex() : n(0), timer(0), LOG(0) {}

    explicit TreeIndex(int size)
        : n(size), timer(0), LOG(0),
          parent(size, -1), depth(size), sub(size),
          heavy(size, -1), top(size), tin(size), tout(size) {}

    void build(int root = 0) {
        vector<int> order;
        order.reserve(n);
        order.push_back(root);

        parent[root] = -1;
        depth[root] = 0;

        for (int i = 0; i < (int)order.size(); ++i) {
            int u = order[i];

            for (int v : adj[u]) {
                if (v == parent[u]) continue;

                parent[v] = u;
                depth[v] = depth[u] + 1;
                order.push_back(v);
            }
        }

        fill(sub.begin(), sub.end(), 1);
        fill(heavy.begin(), heavy.end(), -1);

        for (int i = n - 1; i >= 0; --i) {
            int u = order[i];
            int best = 0;

            for (int v : adj[u]) {
                if (parent[v] != u) continue;

                sub[u] += sub[v];

                if (sub[v] > best) {
                    best = sub[v];
                    heavy[u] = v;
                }
            }
        }

        timer = 0;

        vector<pair<int,int> > st;
        st.push_back(make_pair(root, root));

        while (!st.empty()) {
            int start = st.back().first;
            int chainTop = st.back().second;
            st.pop_back();

            for (int u = start; u != -1; u = heavy[u]) {
                top[u] = chainTop;
                tin[u] = timer++;

                for (int i = (int)adj[u].size() - 1; i >= 0; --i) {
                    int v = adj[u][i];

                    if (parent[v] == u && v != heavy[u]) {
                        st.push_back(make_pair(v, v));
                    }
                }
            }
        }

        for (int u = 0; u < n; ++u)
            tout[u] = tin[u] + sub[u];

        LOG = 1;
        while ((1 << LOG) <= n) ++LOG;

        up.assign(LOG, vector<int>(n));

        for (int v = 0; v < n; ++v)
            up[0][v] = parent[v] == -1 ? v : parent[v];

        for (int k = 1; k < LOG; ++k)
            for (int v = 0; v < n; ++v)
                up[k][v] = up[k - 1][up[k - 1][v]];
    }

    bool ancestor(int u, int v) const {
        return tin[u] <= tin[v] && tin[v] < tout[u];
    }

    int kth_up(int v, int k) const {
        for (int b = 0; b < LOG; ++b)
            if ((k >> b) & 1)
                v = up[b][v];

        return v;
    }

    int lca(int u, int v) const {
        if (ancestor(u, v)) return u;
        if (ancestor(v, u)) return v;

        int x = u;

        for (int k = LOG - 1; k >= 0; --k) {
            int a = up[k][x];

            if (!ancestor(a, v))
                x = a;
        }

        return parent[x];
    }

    int dist(int u, int v) const {
        int w = lca(u, v);
        return depth[u] + depth[v] - 2 * depth[w];
    }

    int toward(int u, int v) const {
        assert(u != v);

        if (!ancestor(u, v))
            return parent[u];

        int jump = depth[v] - depth[u] - 1;
        return kth_up(v, jump);
    }
};

TreeIndex treeIndex;

struct Fenwick {
    int n;
    vector<int> bit;

    Fenwick() : n(0) {}

    explicit Fenwick(int size)
        : n(size), bit(size + 1) {}

    void add(int p, int delta) {
        for (++p; p <= n; p += p & -p)
            bit[p] += delta;
    }

    int prefix(int r) const {
        int ans = 0;

        for (; r > 0; r -= r & -r)
            ans += bit[r];

        return ans;
    }

    int range(int l, int r) const {
        return prefix(r) - prefix(l);
    }
};

struct SuccessorSet {
    int n, alive;
    vector<int> nxt;
    vector<unsigned char> on;

    SuccessorSet() : n(0), alive(0) {}

    explicit SuccessorSet(int size)
        : n(size), alive(size),
          nxt(size + 1), on(size, 1) {
        iota(nxt.begin(), nxt.end(), 0);
    }

    int find(int x) {
        int r = x;

        while (nxt[r] != r)
            r = nxt[r];

        while (nxt[x] != x) {
            int y = nxt[x];
            nxt[x] = r;
            x = y;
        }

        return r;
    }

    bool contains(int x) const {
        return 0 <= x && x < n && on[x];
    }

    void erase(int x) {
        if (!contains(x)) return;

        on[x] = 0;
        --alive;
        nxt[x] = find(x + 1);
    }

    int first() {
        return find(0);
    }

    int next_from(int x) {
        return find(x);
    }

    int size() const {
        return alive;
    }
};

struct DirectedDSU {
    vector<int> p;

    DirectedDSU() {}

    explicit DirectedDSU(int size)
        : p(size) {
        iota(p.begin(), p.end(), 0);
    }

    int find(int x) {
        int r = x;

        while (p[r] != r)
            r = p[r];

        while (p[x] != x) {
            int y = p[x];
            p[x] = r;
            x = y;
        }

        return r;
    }

    void link(int a, int b) {
        a = find(a);
        b = find(b);

        if (a != b)
            p[a] = b;
    }
};

struct NeighborCursor {
    vector<int> off;
    vector<int> parent;

    NeighborCursor() {}

    explicit NeighborCursor(int size)
        : off(size + 1) {

        for (int u = 0; u < size; ++u)
            off[u + 1] = off[u] + (int)adj[u].size() + 1;

        parent.resize(off[size]);
        iota(parent.begin(), parent.end(), 0);
    }

    int find(int x) {
        int r = x;

        while (parent[r] != r)
            r = parent[r];

        while (parent[x] != x) {
            int y = parent[x];
            parent[x] = r;
            x = y;
        }

        return r;
    }

    void erase(int u, int v) {
        int k = lower_bound(adj[u].begin(), adj[u].end(), v)
              - adj[u].begin();

        assert(k < (int)adj[u].size() && adj[u][k] == v);

        int p = off[u] + k;

        if (find(p) == p)
            parent[p] = find(p + 1);
    }

    int first(int u) {
        int p = find(off[u]);

        if (p == off[u + 1] - 1)
            return INF;

        return adj[u][p - off[u]];
    }

    int lower(int u, int x) {
        int k = lower_bound(adj[u].begin(), adj[u].end(), x)
              - adj[u].begin();

        int p = find(off[u] + k);

        if (p == off[u + 1] - 1)
            return INF;

        return adj[u][p - off[u]];
    }
};

struct RedHistory {
    struct TNode {
        int l, r, id;
        uint32_t pri;
        unsigned char active, any;

        TNode()
            : l(-1), r(-1), id(-1),
              pri(0), active(0), any(0) {}
    };

    int n, base;
    vector<int> roots;
    vector<int> keyVal;
    vector<unsigned char> built;
    vector<TNode> pool;

    uint32_t rngState;

    RedHistory()
        : n(0), base(1),
          rngState(0x9e3779b9u) {}

    explicit RedHistory(int size)
        : n(size), base(1),
          keyVal(size, -1),
          built(size, 0),
          rngState(0x9e3779b9u) {

        while (base < n)
            base <<= 1;

        roots.assign(base << 1, -1);
        pool.reserve(size * 24 + 16);
    }

    uint32_t rnd() {
        uint32_t x = rngState;

        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;

        rngState = x;
        return x;
    }

    bool has(int p) const {
        return p != -1 && pool[p].any;
    }

    bool lessId(int a, int b) const {
        if (keyVal[a] != keyVal[b])
            return keyVal[a] < keyVal[b];

        return a < b;
    }

    void pull(int p) {
        pool[p].any =
            pool[p].active ||
            has(pool[p].l) ||
            has(pool[p].r);
    }

    int makeNode(int id, bool active) {
        int p = (int)pool.size();

        TNode x;
        x.id = id;
        x.pri = rnd();
        x.active = active;
        x.any = active;

        pool.push_back(x);
        return p;
    }

    int rotateRight(int p) {
        int q = pool[p].l;

        pool[p].l = pool[q].r;
        pool[q].r = p;

        pull(p);
        pull(q);

        return q;
    }

    int rotateLeft(int p) {
        int q = pool[p].r;

        pool[p].r = pool[q].l;
        pool[q].l = p;

        pull(p);
        pull(q);

        return q;
    }

    int insertNode(int p, int id, bool on) {
        if (p == -1)
            return makeNode(id, on);

        if (lessId(id, pool[p].id)) {
            pool[p].l = insertNode(pool[p].l, id, on);

            if (pool[pool[p].l].pri > pool[p].pri)
                p = rotateRight(p);
        } else {
            pool[p].r = insertNode(pool[p].r, id, on);

            if (pool[pool[p].r].pri > pool[p].pri)
                p = rotateLeft(p);
        }

        pull(p);
        return p;
    }

    void setNode(int p, int id, bool on) {
        assert(p != -1);

        if (pool[p].id == id)
            pool[p].active = on;
        else if (lessId(id, pool[p].id))
            setNode(pool[p].l, id, on);
        else
            setNode(pool[p].r, id, on);

        pull(p);
    }

    int maxActive(int p) const {
        assert(has(p));

        if (has(pool[p].r))
            return maxActive(pool[p].r);

        if (pool[p].active)
            return pool[p].id;

        return maxActive(pool[p].l);
    }

    int predecessor(int p, int head) const {
        if (!has(p))
            return -1;

        int id = pool[p].id;

        if (keyVal[id] >= head)
            return predecessor(pool[p].l, head);

        int q = predecessor(pool[p].r, head);

        if (q != -1)
            return q;

        if (pool[p].active)
            return id;

        if (has(pool[p].l))
            return maxActive(pool[p].l);

        return -1;
    }

    void touchRange(
        int l, int r,
        int id,
        bool on,
        bool firstBuild
    ) {
        if (l >= r) return;

        l += base;
        r += base;

        while (l < r) {
            if (l & 1) {
                if (firstBuild)
                    roots[l] = insertNode(roots[l], id, on);
                else
                    setNode(roots[l], id, on);

                ++l;
            }

            if (r & 1) {
                --r;

                if (firstBuild)
                    roots[r] = insertNode(roots[r], id, on);
                else
                    setNode(roots[r], id, on);
            }

            l >>= 1;
            r >>= 1;
        }
    }

    void change(int x, int tag, bool on) {
        bool firstBuild = !built[x];

        if (firstBuild) {
            built[x] = 1;
            keyVal[x] = tag;
        } else {
            assert(keyVal[x] == tag);
        }

        int first = adj[x][0];

        int second =
            adj[x].size() >= 2
            ? adj[x][1]
            : INF;

        assert(tag == first || tag == second);

        if (tag == first) {
            if (treeIndex.parent[first] == x) {
                touchRange(
                    0,
                    treeIndex.tin[first],
                    x, on, firstBuild
                );

                touchRange(
                    treeIndex.tout[first],
                    n,
                    x, on, firstBuild
                );
            } else {
                touchRange(
                    treeIndex.tin[x],
                    treeIndex.tout[x],
                    x, on, firstBuild
                );
            }
        } else {
            if (treeIndex.parent[first] == x) {
                touchRange(
                    treeIndex.tin[first],
                    treeIndex.tout[first],
                    x, on, firstBuild
                );
            } else {
                touchRange(
                    0,
                    treeIndex.tin[x],
                    x, on, firstBuild
                );

                touchRange(
                    treeIndex.tout[x],
                    n,
                    x, on, firstBuild
                );
            }
        }
    }

    pair<int,int> query(
        int rootPos,
        int head
    ) const {
        int best = -1;

        for (int p = rootPos + base; p; p >>= 1) {
            int q = predecessor(roots[p], head);

            if (
                q != -1 &&
                (
                    best == -1 ||
                    lessId(best, q)
                )
            ) {
                best = q;
            }
        }

        if (best == -1)
            return make_pair(-1, -1);

        return make_pair(keyVal[best], best);
    }
};

struct FinishedDSU {
    struct HeapNode {
        int l, r, v;

        HeapNode(int L = -1, int R = -1, int V = -1)
            : l(L), r(R), v(V) {}
    };

    int n;

    vector<int> p, sz;
    vector<int> mn1, mn2;
    vector<int> heapRoot;

    vector<unsigned char> on;

    vector<HeapNode> heap;
    vector<set<pair<int,int> > > frontier;

    FinishedDSU() : n(0) {}

    explicit FinishedDSU(int size)
        : n(size),
          p(size),
          sz(size, 1),
          mn1(size),
          mn2(size, INF),
          heapRoot(size, -1),
          on(size, 0),
          frontier(size) {

        iota(p.begin(), p.end(), 0);
        iota(mn1.begin(), mn1.end(), 0);

        heap.reserve(size > 1 ? size - 1 : 1);
    }

    int find(int x) {
        int r = x;

        while (p[r] != r)
            r = p[r];

        while (p[x] != x) {
            int y = p[x];
            p[x] = r;
            x = y;
        }

        return r;
    }

    int minExceptNeighbor(int u, int banned) const {
        if (adj[u].empty())
            return INF;

        if (adj[u][0] != banned)
            return adj[u][0];

        if (adj[u].size() >= 2)
            return adj[u][1];

        return INF;
    }

    void addMin(int r, int x) {
        if (x < mn1[r]) {
            mn2[r] = mn1[r];
            mn1[r] = x;
        } else if (
            x != mn1[r] &&
            x < mn2[r]
        ) {
            mn2[r] = x;
        }
    }

    int meld(int a, int b) {
        if (a == -1) return b;
        if (b == -1) return a;

        if (heap[b].v < heap[a].v)
            std::swap(a, b);

        heap[a].r = meld(heap[a].r, b);

        std::swap(heap[a].l, heap[a].r);

        return a;
    }

    int heapPush(int root, int v) {
        int id = (int)heap.size();

        heap.push_back(HeapNode(-1, -1, v));

        return meld(root, id);
    }

    void cleanHeap(int r) {
        while (
            heapRoot[r] != -1 &&
            on[heap[heapRoot[r]].v]
        ) {
            int q = heapRoot[r];

            heapRoot[r] =
                meld(
                    heap[q].l,
                    heap[q].r
                );
        }
    }

    int unite(int a, int b) {
        a = find(a);
        b = find(b);

        if (a == b)
            return a;

        long long weightA =
            (long long)sz[a] +
            frontier[a].size();

        long long weightB =
            (long long)sz[b] +
            frontier[b].size();

        if (weightA < weightB)
            std::swap(a, b);

        p[b] = a;
        sz[a] += sz[b];

        addMin(a, mn1[b]);
        addMin(a, mn2[b]);

        heapRoot[a] =
            meld(
                heapRoot[a],
                heapRoot[b]
            );

        heapRoot[b] = -1;

        frontier[a].insert(
            frontier[b].begin(),
            frontier[b].end()
        );

        frontier[b].clear();

        return a;
    }

    void activate(int x) {
        assert(!on[x]);

        on[x] = 1;

        for (int y : adj[x]) {
            if (on[y]) continue;

            heapRoot[x] =
                heapPush(
                    heapRoot[x],
                    y
                );

            int w =
                minExceptNeighbor(
                    y,
                    x
                );

            if (w != INF)
                frontier[x].insert(make_pair(w, y));
        }

        for (int y : adj[x]) {
            if (!on[y]) continue;

            int r = find(y);

            int w =
                minExceptNeighbor(
                    x,
                    y
                );

            if (w != INF)
                frontier[r].erase(make_pair(w, x));

            unite(x, r);
        }
    }

    int low(int x, int candidateRoot) {
        int r = find(x);

        cleanHeap(r);

        int ans =
            mn1[r] == candidateRoot
            ? mn2[r]
            : mn1[r];

        if (heapRoot[r] != -1)
            ans =
                min(
                    ans,
                    heap[heapRoot[r]].v
                );

        return ans;
    }

    int nextFrontier(int x, int head) {
        int r = find(x);

        set<pair<int,int> >::iterator it =
            frontier[r].upper_bound(
                make_pair(head, INF)
            );

        if (it == frontier[r].end())
            return -1;

        return it->second;
    }

    bool same(int a, int b) {
        return find(a) == find(b);
    }
};

struct SingleRootSolver {
    int root;

    vector<int> parent;
    vector<int> order;
    vector<int> minChild;
    vector<int> redParent;
    vector<int> bucketHead;
    vector<int> bucketNext;

    vector<vector<int> > redChildren;

    vector<unsigned char> active;
    vector<unsigned char> frontierFlag;
    vector<unsigned char> redFlag;

    set<pair<int,int> > frontier;
    set<pair<int,int> > visibleRed;

    explicit SingleRootSolver(int start)
        : root(start),
          parent(n, -1),
          minChild(n, INF),
          redParent(n, -2),
          bucketHead(n, -1),
          bucketNext(n, -1),
          redChildren(n),
          active(n, 0),
          frontierFlag(n, 0),
          redFlag(n, 0) {}

    int firstChild(int u) const {
        for (int v : adj[u])
            if (v != parent[u])
                return v;

        return INF;
    }

    void removeRed(int x) {
        if (!redFlag[x]) return;

        redFlag[x] = 0;

        int p = redParent[x];

        if (p == -1)
            visibleRed.erase(make_pair(minChild[x], x));

        for (int y : redChildren[x]) {
            if (
                !redFlag[y] ||
                redParent[y] != x
            )
                continue;

            redParent[y] = p;

            if (p == -1)
                visibleRed.insert(make_pair(minChild[y], y));
            else
                redChildren[p].push_back(y);
        }
    }

    int firstChildAtLeast(int u, int value) const {
        vector<int>::const_iterator it =
            lower_bound(
                adj[u].begin(),
                adj[u].end(),
                value
            );

        while (
            it != adj[u].end() &&
            *it == parent[u]
        )
            ++it;

        if (it == adj[u].end())
            return -1;

        return *it;
    }

    vector<int> run() {
        order.reserve(n);
        order.push_back(root);

        for (int i = 0; i < (int)order.size(); ++i) {
            int u = order[i];

            for (int v : adj[u]) {
                if (v == parent[u]) continue;

                parent[v] = u;
                order.push_back(v);
            }
        }

        vector<int> initial = adj[root];

        initial.insert(
            lower_bound(
                initial.begin(),
                initial.end(),
                root
            ),
            root
        );

        deque<int> ans;

        for (int x : initial)
            ans.push_back(x);

        int head = ans.front();

        for (int u = 0; u < n; ++u) {
            if (u == root) continue;

            int childCount =
                (int)adj[u].size() - 1;

            if (childCount <= 0)
                continue;

            active[u] = 1;
            minChild[u] = firstChild(u);

            if (minChild[u] < head) {
                redFlag[u] = 1;

                bucketNext[u] =
                    bucketHead[minChild[u]];

                bucketHead[minChild[u]] = u;
            }
        }

        for (int v : adj[root]) {
            if (!active[v]) continue;

            frontierFlag[v] = 1;
            frontier.insert(make_pair(minChild[v], v));
        }

        vector<pair<int,int> > st;
        st.push_back(make_pair(root, -1));

        while (!st.empty()) {
            int u = st.back().first;
            int lastRed = st.back().second;

            st.pop_back();

            for (int i = (int)adj[u].size() - 1; i >= 0; --i) {
                int v = adj[u][i];

                if (v == parent[u])
                    continue;

                if (redFlag[v]) {
                    redParent[v] = lastRed;

                    if (lastRed == -1)
                        visibleRed.insert(make_pair(minChild[v], v));
                    else
                        redChildren[lastRed].push_back(v);

                    st.push_back(make_pair(v, v));
                } else {
                    st.push_back(make_pair(v, lastRed));
                }
            }
        }

        while ((int)ans.size() < n) {
            int red = -1;
            int redAfter = -1;

            if (!visibleRed.empty()) {
                red = visibleRed.rbegin()->second;

                redAfter =
                    firstChildAtLeast(
                        red,
                        head
                    );
            }

            int open = -1;

            set<pair<int,int> >::iterator it =
                frontier.upper_bound(
                    make_pair(head, INF)
                );

            if (it != frontier.end())
                open = it->second;

            int x;

            if (
                open != -1 &&
                (
                    red == -1 ||
                    !frontierFlag[red] ||
                    (
                        redAfter != -1 &&
                        minChild[open] < redAfter
                    )
                )
            ) {
                x = open;
            } else {
                x = red;
            }

            if (x == -1)
                break;

            if (frontierFlag[x]) {
                frontier.erase(make_pair(minChild[x], x));
                frontierFlag[x] = 0;
            }

            removeRed(x);

            int oldHead = head;

            int cut =
                lower_bound(
                    adj[x].begin(),
                    adj[x].end(),
                    oldHead
                ) - adj[x].begin();

            for (int i = cut - 1; i >= 0; --i) {
                int v = adj[x][i];

                if (v != parent[x])
                    ans.push_front(v);
            }

            for (int i = cut; i < (int)adj[x].size(); ++i) {
                int v = adj[x][i];

                if (v != parent[x])
                    ans.push_back(v);
            }

            active[x] = 0;

            for (int v : adj[x]) {
                if (
                    v == parent[x] ||
                    !active[v]
                )
                    continue;

                frontierFlag[v] = 1;
                frontier.insert(make_pair(minChild[v], v));
            }

            int newHead = ans.front();

            if (newHead < oldHead) {
                for (int value = oldHead - 1;
                     value >= newHead;
                     --value) {

                    for (
                        int y = bucketHead[value];
                        y != -1;
                        y = bucketNext[y]
                    )
                        removeRed(y);
                }

                head = newHead;
            }
        }

        return vector<int>(ans.begin(), ans.end());
    }
};

static inline vector<int> solveOneRoot(int root) {
    SingleRootSolver solver(root);
    return solver.run();
}

struct RootSelector {
    vector<int> direction;

    DirectedDSU rootUF;
    Fenwick prefixBIT;
    SuccessorSet candidates;
    SuccessorSet scanSet;

    vector<unsigned char> prefixFlag;
    vector<unsigned char> visited;
    vector<unsigned char> used;

    vector<int> prefixStack;

    FinishedDSU finished;
    NeighborCursor remain;
    RedHistory redHistory;

    int eventCount;

    vector<int> eventKey;
    vector<int> previousEvent;

    vector<int> redTag;
    vector<int> redEvent;

    vector<unsigned char> redOn;

    vector<pair<int,int> > pendingRed;
    vector<int> tempRemovedRed;

    int currentCandidate;
    bool currentCandidateAlive;
    bool candidateSimAlive;

    int lastVertex;

    int candidate;
    int candidateVersion;
    int candidateCut;
    int candidateHead;
    int candidatePos;

    vector<int> parentVersion;
    vector<int> parentCache;

    bool rootBlockFinished;

    int expandedVertex;
    int expandedOldHead;

    vector<pair<int,int> > stackBranch;

    RootSelector()
        : direction(n, -1),
          rootUF(n),
          prefixBIT(n),
          candidates(n),
          scanSet(n),
          prefixFlag(n, 0),
          visited(n, 0),
          used(n, 0),
          finished(n),
          remain(n),
          redHistory(n),
          eventCount(0),
          eventKey(n),
          previousEvent(n, -1),
          redTag(n, -1),
          redEvent(n, -1),
          redOn(n, 0),
          currentCandidate(-1),
          currentCandidateAlive(false),
          candidateSimAlive(false),
          lastVertex(-1),
          candidate(-1),
          candidateVersion(0),
          candidateCut(0),
          candidateHead(-1),
          candidatePos(0),
          parentVersion(n, -1),
          parentCache(n, -1),
          rootBlockFinished(false),
          expandedVertex(-1),
          expandedOldHead(0) {

        iota(eventKey.begin(), eventKey.end(), 0);

        prefixStack.reserve(n);
        pendingRed.reserve(n);
        tempRemovedRed.reserve(n);
        stackBranch.reserve(n);
    }

    int pathPrefixCount(int u, int v) {
        int ans = 0;

        while (treeIndex.top[u] != treeIndex.top[v]) {
            if (
                treeIndex.depth[treeIndex.top[u]] <
                treeIndex.depth[treeIndex.top[v]]
            )
                std::swap(u, v);

            ans +=
                prefixBIT.range(
                    treeIndex.tin[treeIndex.top[u]],
                    treeIndex.tin[u] + 1
                );

            u =
                treeIndex.parent[
                    treeIndex.top[u]
                ];
        }

        if (treeIndex.depth[u] > treeIndex.depth[v])
            std::swap(u, v);

        ans +=
            prefixBIT.range(
                treeIndex.tin[u],
                treeIndex.tin[v] + 1
            );

        return ans;
    }

    void turnRedOn(int tag, int x) {
        if (
            x == -1 ||
            adj[x].empty() ||
            finished.on[x] ||
            redOn[x]
        )
            return;

        int first = adj[x][0];

        int second =
            adj[x].size() >= 2
            ? adj[x][1]
            : INF;

        if (tag != first && tag != second)
            return;

        redHistory.change(x, tag, true);
        redOn[x] = 1;
    }

    void turnRedOff(int x) {
        if (!redOn[x])
            return;

        redHistory.change(
            x,
            redTag[x],
            false
        );

        redOn[x] = 0;
    }

    void registerRedEvent(int eventId, int from, int x) {
        if (
            x == -1 ||
            adj[x].empty()
        )
            return;

        int first = adj[x][0];

        int second =
            adj[x].size() >= 2
            ? adj[x][1]
            : INF;

        if (from != first && from != second)
            return;

        redTag[x] = from;
        redEvent[x] = eventId;

        if (candidateSimAlive)
            pendingRed.push_back(make_pair(from, x));
        else
            turnRedOn(from, x);
    }

    void flushPendingRed() {
        for (int i = 0; i < (int)pendingRed.size(); ++i)
            turnRedOn(
                pendingRed[i].first,
                pendingRed[i].second
            );

        pendingRed.clear();
    }

    void restoreTemporaryRed() {
        for (int x : tempRemovedRed)
            if (!finished.on[x])
                turnRedOn(redTag[x], x);

        tempRemovedRed.clear();
    }

    void addPrefix(int x, int from = -1) {
        if (
            x != -1 &&
            !prefixFlag[x]
        ) {
            prefixFlag[x] = 1;
            prefixStack.push_back(x);

            prefixBIT.add(
                treeIndex.tin[x],
                1
            );

            if (from != -1) {
                int id = eventCount++;

                eventKey[x] = from;
                previousEvent[x] = id;

                registerRedEvent(
                    id,
                    from,
                    x
                );
            }

            return;
        }

        if (
            x != -1 &&
            from != -1 &&
            eventKey[x] < from &&
            from < x &&
            !(
                currentCandidateAlive &&
                from == currentCandidate
            ) &&
            !prefixFlag[from] &&
            previousEvent[x] != -1
        ) {
            int oldKey = eventKey[x];
            int id = previousEvent[x];

            prefixFlag[from] = 1;
            prefixStack.push_back(from);

            prefixBIT.add(
                treeIndex.tin[from],
                1
            );

            eventKey[from] = oldKey;
            eventKey[x] = from;
            previousEvent[from] = id;
        }
    }

    void directBranch(int start, int toward) {
        stackBranch.clear();
        stackBranch.push_back(make_pair(start, toward));

        while (!stackBranch.empty()) {
            int x = stackBranch.back().first;
            int from = stackBranch.back().second;

            stackBranch.pop_back();

            if (!candidates.contains(x))
                continue;

            direction[x] = from;

            rootUF.link(x, from);
            candidates.erase(x);

            for (int y : adj[x]) {
                if (
                    y == from ||
                    !candidates.contains(y)
                )
                    continue;

                stackBranch.push_back(make_pair(y, x));
            }
        }
    }

    bool valid(int x, int target = -1) {
        if (candidates.contains(x))
            return true;

        if (direction[x] == -1)
            return false;

        if (direction[direction[x]] == -1)
            return true;

        int cnt =
            pathPrefixCount(
                direction[x],
                rootUF.find(x)
            );

        if (cnt == 0)
            return true;

        if (
            cnt == 1 &&
            prefixFlag[direction[x]] &&
            !prefixStack.empty() &&
            prefixStack.back() == direction[x]
        ) {
            return
                target == -1 ||
                treeIndex.dist(target, x) <= 2;
        }

        return false;
    }

    void markUsed(int x) {
        if (used[x])
            return;

        used[x] = 1;

        for (int y : adj[x])
            remain.erase(y, x);
    }

    int candidateParent(int u) {
        if (u == candidate)
            return -1;

        if (parentVersion[u] != candidateVersion) {
            parentVersion[u] = candidateVersion;

            parentCache[u] =
                treeIndex.toward(
                    u,
                    candidate
                );
        }

        return parentCache[u];
    }

    int candidateMinChild(int u) {
        int p = candidateParent(u);

        if (adj[u].empty())
            return INF;

        if (adj[u][0] != p)
            return adj[u][0];

        if (adj[u].size() >= 2)
            return adj[u][1];

        return INF;
    }

    void finishComponent(int u) {
        if (!finished.on[u]) {
            assert(used[u]);

            for (int v : adj[u])
                assert(used[v]);

            turnRedOff(u);
            finished.activate(u);
        }

        candidateHead =
            min(
                candidateHead,
                finished.low(
                    u,
                    candidate
                )
            );
    }

    bool isOpen(int u) {
        if (
            u == candidate ||
            finished.on[u]
        )
            return false;

        int p = candidateParent(u);

        return
            p != -1 &&
            finished.on[p] &&
            finished.same(p, candidate);
    }

    int firstOpen() {
        return
            finished.nextFrontier(
                candidate,
                candidateHead
            );
    }

    int blockingRed() {
        if (candidateHead <= 0)
            return -1;

        pair<int,int> q =
            redHistory.query(
                candidatePos,
                candidateHead
            );

        int tag = q.first;
        int u = q.second;

        if (u == -1)
            return -1;

        assert(!finished.on[u]);
        assert(redOn[u]);
        assert(redTag[u] == tag);
        assert(redEvent[u] < candidateCut);
        assert(candidateMinChild(u) == tag);

        return u;
    }

    void expandCandidate(int u) {
        assert(isOpen(u));

        int oldHead = candidateHead;

        expandedVertex = u;
        expandedOldHead = oldHead;

        int value = candidateMinChild(u);

        if (value < candidateHead) {
            int eventId = redEvent[u];

            if (
                eventId != -1 &&
                eventId < candidateCut &&
                redTag[u] == value &&
                redOn[u]
            ) {
                turnRedOff(u);
                tempRemovedRed.push_back(u);
            }

            candidateHead = value;
        }
    }

    int peekCandidate() {
        while (true) {
            if (!rootBlockFinished) {
                int x =
                    used[candidate]
                    ? INF
                    : candidate;

                x =
                    min(
                        x,
                        remain.first(candidate)
                    );

                if (x != INF)
                    return x;

                finishComponent(candidate);
                rootBlockFinished = true;
            }

            if (expandedVertex != -1) {
                int x =
                    remain.lower(
                        expandedVertex,
                        expandedOldHead
                    );

                if (x != INF)
                    return x;

                finishComponent(expandedVertex);
                expandedVertex = -1;
            }

            int bad = blockingRed();
            int afterBad = -1;

            if (bad != -1) {
                int p = candidateParent(bad);

                vector<int>::iterator it =
                    lower_bound(
                        adj[bad].begin(),
                        adj[bad].end(),
                        candidateHead
                    );

                while (
                    it != adj[bad].end() &&
                    *it == p
                )
                    ++it;

                if (it != adj[bad].end())
                    afterBad = *it;
            }

            int open = firstOpen();

            int u;

            if (
                open != -1 &&
                (
                    bad == -1 ||
                    !isOpen(bad) ||
                    (
                        afterBad != -1 &&
                        candidateMinChild(open) < afterBad
                    )
                )
            ) {
                u = open;
            } else {
                u = bad;
            }

            if (u == -1)
                return INF;

            expandCandidate(u);
        }
    }

    void startCandidate(int root) {
        if (candidateSimAlive)
            return;

        restoreTemporaryRed();
        flushPendingRed();

        candidate = root;
        currentCandidate = root;

        currentCandidateAlive = true;
        candidateSimAlive = true;

        ++candidateVersion;

        candidatePos = treeIndex.tin[root];
        candidateCut = eventCount;

        rootBlockFinished = false;

        expandedVertex = -1;
        expandedOldHead = 0;

        candidateHead =
            min(
                root,
                adj[root].empty()
                ? INF
                : adj[root][0]
            );
    }

    int compareCandidateWith(int x) {
        if (!candidateSimAlive)
            return -1;

        int y = peekCandidate();

        if (y == INF)
            return -1;

        if (y < x)
            return candidate;

        if (y > x) {
            candidateSimAlive = false;
            currentCandidateAlive = false;

            restoreTemporaryRed();
            flushPendingRed();

            return -1;
        }

        markUsed(y);

        return -1;
    }

    int processAliveVertex(int x) {
        for (int y : adj[x]) {
            if (
                visited[y] &&
                direction[y] == -1
            ) {
                assert(candidates.contains(y));
                assert(lastVertex == y);

                startCandidate(y);
            }
        }

        scanSet.erase(x);

        while (scanSet.size() > 0) {
            int y = scanSet.first();

            if (
                candidates.contains(y) ||
                valid(y, x)
            ) {
                int winner =
                    compareCandidateWith(y);

                if (winner != -1)
                    return winner;

                int d = treeIndex.dist(x, y);

                if (d >= 3) {
                    int u =
                        treeIndex.toward(
                            x,
                            y
                        );

                    int v =
                        treeIndex.toward(
                            u,
                            y
                        );

                    if (!candidates.contains(v)) {
                        scanSet.erase(y);
                        continue;
                    }

                    if (
                        lastVertex != -1 &&
                        u != lastVertex
                    ) {
                        addPrefix(
                            x,
                            lastVertex
                        );

                        lastVertex = -1;
                    }

                    directBranch(u, v);

                    addPrefix(
                        direction[x],
                        x
                    );
                }

                else if (d == 2) {
                    int u =
                        treeIndex.toward(
                            x,
                            y
                        );

                    if (!candidates.contains(u)) {
                        scanSet.erase(y);
                        continue;
                    }

                    if (
                        lastVertex != -1 &&
                        u != lastVertex
                    ) {
                        addPrefix(
                            x,
                            lastVertex
                        );
                    }

                    directBranch(x, u);

                    addPrefix(
                        direction[x],
                        x
                    );
                }

                else {
                    if (
                        lastVertex != -1 &&
                        y != lastVertex
                    ) {
                        addPrefix(
                            x,
                            lastVertex
                        );
                    }

                    for (int v : adj[x]) {
                        if (
                            v != y &&
                            direction[v] != x
                        )
                            directBranch(v, x);
                    }

                    lastVertex = x;
                }

                break;
            }

            scanSet.erase(y);
        }

        return -1;
    }

    vector<int> run() {
        while (candidates.size() > 2) {
            int x = scanSet.first();

            assert(x < n);

            visited[x] = 1;
            markUsed(x);

            if (candidates.contains(x)) {
                int winner =
                    processAliveVertex(x);

                if (winner != -1)
                    return solveOneRoot(winner);

                continue;
            }

            if (!valid(x)) {
                scanSet.erase(x);
                continue;
            }

            addPrefix(
                direction[x],
                x
            );

            scanSet.erase(x);

            if (
                candidates.contains(
                    direction[x]
                )
            ) {
                while (scanSet.size() > 0) {
                    int y = scanSet.first();

                    if (
                        candidates.contains(y) ||
                        valid(y, x)
                    ) {
                        int winner =
                            compareCandidateWith(y);

                        if (winner != -1)
                            return solveOneRoot(winner);

                        if (
                            treeIndex.dist(x, y) >= 3
                        ) {
                            int u =
                                treeIndex.toward(
                                    x,
                                    y
                                );

                            int v =
                                treeIndex.toward(
                                    u,
                                    y
                                );

                            if (
                                u != direction[x]
                            ) {
                                scanSet.erase(y);
                                continue;
                            }

                            directBranch(u, v);
                        }

                        break;
                    }

                    scanSet.erase(y);
                }
            }
        }

        vector<int> best(n, INF);

        for (
            int r = candidates.first();
            r < n;
            r = candidates.next_from(r + 1)
        ) {
            vector<int> cur =
                solveOneRoot(r);

            if (cur < best)
                best.swap(cur);
        }

        if (
            candidateSimAlive &&
            candidate != -1
        ) {
            vector<int> cur =
                solveOneRoot(candidate);

            if (cur < best)
                best.swap(cur);
        }

        return best;
    }
};

static inline vector<int> solveFast() {
    RootSelector solver;
    return solver.run();
}

int main() {
    if (scanf("%d", &n) != 1)
        return 0;

    adj.assign(n, vector<int>());

    for (int i = 1; i < n; ++i) {
        int u, v;

        scanf("%d%d", &u, &v);

        --u;
        --v;

        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    for (int i = 0; i < n; ++i)
        sort(adj[i].begin(), adj[i].end());

    treeIndex = TreeIndex(n);
    treeIndex.build(0);

    vector<int> ans = solveFast();

    for (int i = 0; i < n; ++i) {
        if (i)
            putchar(' ');

        printf("%d", ans[i] + 1);
    }

    putchar('\n');

    return 0;
}