#include <iostream>
#include <utility>
#include <vector>

struct Splay {
    struct Node {
        long long key = 0;
        int child[2] = {0, 0};
        int parent = 0, count = 0, size = 0;
    };
    std::vector<Node> nodes;
    int root = 0;

    explicit Splay(int capacity) {
        nodes.reserve(static_cast<size_t>(capacity) + 1);
        nodes.emplace_back();
    }
    int makeNode(long long key, int parent) {
        Node x;
        x.key = key; x.parent = parent; x.count = x.size = 1;
        nodes.push_back(x);
        return static_cast<int>(nodes.size()) - 1;
    }
    void pull(int x) {
        if (x) nodes[x].size = nodes[x].count + nodes[nodes[x].child[0]].size + nodes[nodes[x].child[1]].size;
    }
    void rotate(int x) {
        int p = nodes[x].parent, g = nodes[p].parent;
        int direction = nodes[p].child[1] == x;
        int middle = nodes[x].child[direction ^ 1];
        if (g) nodes[g].child[nodes[g].child[1] == p] = x;
        else root = x;
        nodes[x].parent = g;
        nodes[p].child[direction] = middle;
        if (middle) nodes[middle].parent = p;
        nodes[x].child[direction ^ 1] = p;
        nodes[p].parent = x;
        pull(p); pull(x);
    }
    int splay(int x) {
        int rotations = 0;
        while (nodes[x].parent) {
            int p = nodes[x].parent, g = nodes[p].parent;
            if (!g) {
                rotate(x); ++rotations;
            } else if ((nodes[g].child[1] == p) == (nodes[p].child[1] == x)) {
                rotate(p); rotate(x); rotations += 2;
            } else {
                rotate(x); rotate(x); rotations += 2;
            }
        }
        return rotations;
    }
    int insert(long long key) {
        if (!root) { root = makeNode(key, 0); return 0; }
        int u = root;
        while (true) {
            if (nodes[u].key == key) {
                ++nodes[u].count; pull(u); return splay(u);
            }
            int direction = key > nodes[u].key;
            if (nodes[u].child[direction]) u = nodes[u].child[direction];
            else {
                int x = makeNode(key, u);
                nodes[u].child[direction] = x;
                return splay(x);
            }
        }
    }
    std::pair<int,int> locate(long long key) {
        int u = root, last = 0, found = 0;
        while (u) {
            last = u;
            if (nodes[u].key == key) { found = u; break; }
            u = nodes[u].child[key > nodes[u].key];
        }
        return {found, last ? splay(last) : 0};
    }
    void eraseOne(long long key) {
        if (!locate(key).first) return;
        if (nodes[root].count > 1) { --nodes[root].count; pull(root); return; }
        int left = nodes[root].child[0], right = nodes[root].child[1];
        if (left) nodes[left].parent = 0;
        if (right) nodes[right].parent = 0;
        if (!left) { root = right; return; }
        root = left;
        int x = root;
        while (nodes[x].child[1]) x = nodes[x].child[1];
        splay(x);
        nodes[root].child[1] = right;
        if (right) nodes[right].parent = root;
        pull(root);
    }
    int rankOf(long long key) {
        int u = root, last = 0, result = 1;
        while (u) {
            last = u;
            if (key <= nodes[u].key) u = nodes[u].child[0];
            else {
                result += nodes[nodes[u].child[0]].size + nodes[u].count;
                u = nodes[u].child[1];
            }
        }
        if (last) splay(last);
        return result;
    }
    long long kth(int k) {
        int u = root;
        while (u) {
            int left = nodes[nodes[u].child[0]].size;
            if (k <= left) u = nodes[u].child[0];
            else if (k <= left + nodes[u].count) { splay(u); return nodes[u].key; }
            else { k -= left + nodes[u].count; u = nodes[u].child[1]; }
        }
        return 0;
    }
    std::pair<bool,long long> neighbor(long long key, bool successor) {
        int u = root, last = 0, best = 0;
        while (u) {
            last = u;
            if (successor ? nodes[u].key > key : nodes[u].key < key) {
                best = u; u = nodes[u].child[successor ? 0 : 1];
            } else u = nodes[u].child[successor ? 1 : 0];
        }
        if (best) { long long answer = nodes[best].key; splay(best); return {true, answer}; }
        if (last) splay(last);
        return {false, 0};
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    int q;
    if (!(std::cin >> q)) return 0;
    Splay tree(q);
    while (q--) {
        int op; long long x;
        std::cin >> op >> x;
        if (op == 1) tree.insert(x);
        else if (op == 2) tree.eraseOne(x);
        else if (op == 3) std::cout << tree.rankOf(x) << '\n';
        else if (op == 4) std::cout << tree.kth(static_cast<int>(x)) << '\n';
        else if (op == 5 || op == 6) {
            auto result = tree.neighbor(x, op == 6);
            if (result.first) std::cout << result.second << '\n';
            else std::cout << "NONE\n";
        }
    }
    return 0;
}
