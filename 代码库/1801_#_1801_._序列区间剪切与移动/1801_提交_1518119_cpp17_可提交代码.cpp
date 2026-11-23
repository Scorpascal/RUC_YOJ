#include <iostream>
#include <utility>
#include <vector>

struct Sequence {
    struct Node { int child[2] = {0,0}, parent = 0, size = 0; };
    std::vector<Node> nodes;
    int root;
    explicit Sequence(int n) : nodes(n + 1), root(0) { root = build(1,n,0); }
    void pull(int x) { nodes[x].size = 1 + nodes[nodes[x].child[0]].size + nodes[nodes[x].child[1]].size; }
    int build(int l, int r, int parent) {
        if (l > r) return 0;
        int x = l + (r - l) / 2;
        nodes[x].parent = parent;
        nodes[x].child[0] = build(l,x-1,x);
        nodes[x].child[1] = build(x+1,r,x);
        pull(x);
        return x;
    }
    void rotate(int x) {
        int p = nodes[x].parent, g = nodes[p].parent;
        int d = nodes[p].child[1] == x, middle = nodes[x].child[d ^ 1];
        if (g) nodes[g].child[nodes[g].child[1] == p] = x;
        else root = x;
        nodes[x].parent = g;
        nodes[p].child[d] = middle;
        if (middle) nodes[middle].parent = p;
        nodes[x].child[d ^ 1] = p;
        nodes[p].parent = x;
        pull(p); pull(x);
    }
    void splay(int x) {
        while (nodes[x].parent) {
            int p = nodes[x].parent, g = nodes[p].parent;
            if (!g) rotate(x);
            else if ((nodes[g].child[1] == p) == (nodes[p].child[1] == x)) { rotate(p); rotate(x); }
            else { rotate(x); rotate(x); }
        }
    }
    std::pair<int,int> split(int rt, int k) {
        root = rt;
        if (!rt) return {0,0};
        if (k == 0) return {0,rt};
        if (k == nodes[rt].size) return {rt,0};
        int x = rt;
        while (x) {
            int left = nodes[nodes[x].child[0]].size;
            if (k == left + 1) break;
            if (k <= left) x = nodes[x].child[0];
            else { k -= left + 1; x = nodes[x].child[1]; }
        }
        splay(x);
        int b = nodes[x].child[1];
        nodes[x].child[1] = 0;
        if (b) nodes[b].parent = 0;
        pull(x);
        return {x,b};
    }
    int merge(int a, int b) {
        if (!a || !b) { root = a ? a : b; return root; }
        root = a;
        int x = a;
        while (nodes[x].child[1]) x = nodes[x].child[1];
        splay(x);
        nodes[x].child[1] = b; nodes[b].parent = x; pull(x);
        return x;
    }
    void move(int l, int r, int k) {
        auto a_bc = split(root, l-1);
        auto b_c = split(a_bc.second, r-l+1);
        int rest = merge(a_bc.first,b_c.second);
        auto left_right = split(rest,k);
        root = merge(merge(left_right.first,b_c.first),left_right.second);
    }
    void print() {
        std::vector<int> stack;
        int u = root;
        bool first = true;
        while (u || !stack.empty()) {
            while (u) { stack.push_back(u); u = nodes[u].child[0]; }
            u = stack.back(); stack.pop_back();
            if (!first) std::cout << ' ';
            first = false;
            std::cout << u;
            u = nodes[u].child[1];
        }
        std::cout << '\n';
    }
};
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int n,m;
    if (!(std::cin >> n >> m)) return 0;
    Sequence seq(n);
    while (m--) { int l,r,k; std::cin >> l >> r >> k; seq.move(l,r,k); }
    seq.print();
    return 0;
}
