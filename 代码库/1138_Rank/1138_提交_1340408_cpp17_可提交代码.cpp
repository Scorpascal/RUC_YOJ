#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

// 需要 g++ 的 pbds 扩展
using namespace std;
using namespace __gnu_pbds;

struct Node {
    long long score;   // 分数越大越靠前
    long long t;       // 上传时间戳，越小越靠前（同分时先上传优先）
    string name;       // 角色名（用于输出）
};

struct Cmp {
    bool operator()(const Node& a, const Node& b) const {
        if (a.score != b.score) return a.score > b.score; // 分数降序
        return a.t < b.t;                                 // 时间升序
    }
};

// order statistics tree: 支持 order_of_key / find_by_order
using OST = tree<Node, null_type, Cmp, rb_tree_tag, tree_order_statistics_node_update>;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    OST tr;
    unordered_map<string, Node> cur; // name -> 当前节点（用于删除/查排名）
    cur.reserve((size_t)n * 2);

    long long timer = 0;

    for (int i = 0; i < n; i++) {
        char op;
        cin >> op;

        if (op == '+') {
            string name;
            long long score;
            cin >> name >> score;

            auto it = cur.find(name);
            if (it != cur.end()) {
                tr.erase(it->second); // 删除旧记录
            }

            Node nd{score, timer++, name};
            tr.insert(nd);
            cur[name] = nd;
        } else if (op == '?') {
            string x;
            cin >> x;

            // ?Name 或 ?Index：用首字符是否为数字区分
            if (!x.empty() && isdigit((unsigned char)x[0])) {
                int idx = stoi(x); // 1-based
                auto it = tr.find_by_order(idx - 1);

                bool first = true;
                for (int k = 0; k < 10 && it != tr.end(); k++, ++it) {
                    if (!first) cout << ' ';
                    first = false;
                    cout << it->name;
                }
                cout << "\n";
            } else {
                const string& name = x;
                const Node& nd = cur[name]; // 题目保证已上传
                int rank = (int)tr.order_of_key(nd) + 1; // 1-based
                cout << rank << "\n";
            }
        }
    }

    return 0;
}