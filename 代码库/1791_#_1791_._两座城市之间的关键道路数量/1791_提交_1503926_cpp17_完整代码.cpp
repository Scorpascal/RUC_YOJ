#include <algorithm>
#include <cstdio>
#include <vector>

class FastInput {
    static constexpr int size = 1 << 16;
    char buffer[size];
    int pos = 0, len = 0;
    int nextChar() {
        if (pos == len) {
            len = static_cast<int>(std::fread(buffer, 1, size, stdin));
            pos = 0;
            if (!len) return EOF;
        }
        return static_cast<unsigned char>(buffer[pos++]);
    }
public:
    bool read(int &value) {
        int c = nextChar();
        while (c != EOF && c <= ' ') c = nextChar();
        if (c == EOF) return false;
        int sign = 1;
        if (c == '-') { sign = -1; c = nextChar(); }
        int v = 0;
        while (c >= '0' && c <= '9') {
            v = v * 10 + c - '0';
            c = nextChar();
        }
        value = v * sign;
        return true;
    }
};

class FastOutput {
    static constexpr int size = 1 << 16;
    char buffer[size];
    int used = 0;
    void put(char c) {
        if (used == size) flush();
        buffer[used++] = c;
    }
public:
    ~FastOutput() { flush(); }
    void flush() {
        if (used) std::fwrite(buffer, 1, used, stdout);
        used = 0;
    }
    void write(int value, char end) {
        if (value < 0) { put('-'); value = -value; }
        char digits[16];
        int count = 0;
        do { digits[count++] = static_cast<char>('0' + value % 10); value /= 10; } while (value);
        while (count) put(digits[--count]);
        put(end);
    }
};

struct BridgeGraph {
    int n, m, inserted = 0;
    std::vector<int> head, to, next, dfn, low, parent, parentEdge, cursor, comp;
    std::vector<unsigned char> bridge;

    BridgeGraph(int vertices, int edges)
        : n(vertices), m(edges), head(n + 1, -1), to(2 * m), next(2 * m),
          dfn(n + 1, 0), low(n + 1, 0), parent(n + 1, 0),
          parentEdge(n + 1, -1), cursor(n + 1, -1), comp(n + 1, 0), bridge(m, 0) {}

    void addEdge(int u, int v) {
        int e = 2 * inserted++;
        to[e] = v; next[e] = head[u]; head[u] = e;
        to[e + 1] = u; next[e + 1] = head[v]; head[v] = e + 1;
    }

    void findBridges() {
        int timestamp = 0;
        std::vector<int> stack;
        stack.reserve(n);
        for (int start = 1; start <= n; ++start) {
            if (dfn[start]) continue;
            dfn[start] = low[start] = ++timestamp;
            cursor[start] = head[start];
            stack.push_back(start);
            while (!stack.empty()) {
                int u = stack.back();
                int &it = cursor[u];
                if (it == -1) {
                    stack.pop_back();
                    if (parentEdge[u] != -1) {
                        int p = parent[u];
                        if (low[u] > dfn[p]) bridge[parentEdge[u]] = 1;
                        low[p] = std::min(low[p], low[u]);
                    }
                    continue;
                }
                int arc = it;
                it = next[arc];
                int edge = arc >> 1, v = to[arc];
                if (edge == parentEdge[u]) continue;
                if (!dfn[v]) {
                    parent[v] = u;
                    parentEdge[v] = edge;
                    dfn[v] = low[v] = ++timestamp;
                    cursor[v] = head[v];
                    stack.push_back(v);
                } else {
                    low[u] = std::min(low[u], dfn[v]);
                }
            }
        }
    }

    int labelComponents() {
        int count = 0;
        std::vector<int> stack;
        stack.reserve(n);
        for (int start = 1; start <= n; ++start) {
            if (comp[start]) continue;
            comp[start] = ++count;
            stack.push_back(start);
            while (!stack.empty()) {
                int u = stack.back();
                stack.pop_back();
                for (int e = head[u]; e != -1; e = next[e]) {
                    int v = to[e];
                    if (!bridge[e >> 1] && !comp[v]) {
                        comp[v] = count;
                        stack.push_back(v);
                    }
                }
            }
        }
        return count;
    }
};

int main() {
    FastInput in;
    int n, m, q;
    if (!in.read(n) || !in.read(m) || !in.read(q)) return 0;
    BridgeGraph graph(n, m);
    for (int i = 0; i < m; ++i) {
        int u, v; in.read(u); in.read(v);
        graph.addEdge(u, v);
    }
    graph.findBridges();
    int count = graph.labelComponents();
    std::vector<int> head(count + 1, -1), to, next;
    to.reserve(2 * count);
    next.reserve(2 * count);
    auto addArc = [&](int u, int v) {
        int e = static_cast<int>(to.size());
        to.push_back(v); next.push_back(head[u]); head[u] = e;
    };
    for (int e = 0; e < m; ++e) if (graph.bridge[e]) {
        int u = graph.comp[graph.to[2 * e]], v = graph.comp[graph.to[2 * e + 1]];
        addArc(u, v); addArc(v, u);
    }
    int logn = 1;
    while ((1LL << logn) <= count) ++logn;
    std::vector<std::vector<int>> up(logn, std::vector<int>(count + 1, 0));
    std::vector<int> depth(count + 1, 0), root(count + 1, 0), stack;
    stack.reserve(count);
    for (int start = 1; start <= count; ++start) {
        if (root[start]) continue;
        root[start] = start;
        stack.push_back(start);
        while (!stack.empty()) {
            int u = stack.back(); stack.pop_back();
            for (int e = head[u]; e != -1; e = next[e]) {
                int v = to[e];
                if (root[v]) continue;
                root[v] = start;
                up[0][v] = u;
                depth[v] = depth[u] + 1;
                stack.push_back(v);
            }
        }
    }
    for (int k = 1; k < logn; ++k)
        for (int u = 1; u <= count; ++u) up[k][u] = up[k - 1][up[k - 1][u]];
    FastOutput out;
    for (int i = 0; i < q; ++i) {
        int u, v; in.read(u); in.read(v);
        u = graph.comp[u]; v = graph.comp[v];
        if (root[u] != root[v]) { out.write(-1, '\n'); continue; }
        int total = depth[u] + depth[v];
        if (depth[u] < depth[v]) std::swap(u, v);
        int diff = depth[u] - depth[v];
        for (int k = 0; diff; ++k, diff >>= 1) if (diff & 1) u = up[k][u];
        if (u != v) {
            for (int k = logn - 1; k >= 0; --k) if (up[k][u] != up[k][v]) {
                u = up[k][u]; v = up[k][v];
            }
            u = up[0][u];
        }
        out.write(total - 2 * depth[u], '\n');
    }
    return 0;
}
