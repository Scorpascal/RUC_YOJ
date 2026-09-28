#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace std;

struct Node {
    int left = -1, right = -1;
    char ch = 0;
    bool leaf = false;
};

static vector<Node> tree;

int parseTree(const string &s, int &pos) {
    if (pos >= (int)s.size()) return -1;
    if (s[pos] == '1') {
        ++pos;
        Node u;
        u.leaf = true;
        u.ch = s[pos++];
        tree.push_back(u);
        return (int)tree.size() - 1;
    }
    ++pos;
    Node u;
    tree.push_back(u);
    int id = (int)tree.size() - 1;
    tree[id].left = parseTree(s, pos);
    tree[id].right = parseTree(s, pos);
    return id;
}

int hexValue(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

string decodeString(const string &s, int root) {
    if (s.empty()) return s;
    if (s[0] != 'H') return s;
    if (s.size() >= 2 && s[1] == 'H') return s.substr(1);
    if (s.size() < 3 || ((s.size() - 1) & 1)) return s;
    string hex = s.substr(1);
    int pad = hexValue(hex[(int)hex.size() - 2]) * 16 +
              hexValue(hex[(int)hex.size() - 1]);
    string data = hex.substr(0, hex.size() - 2);
    string out;
    int u = root;
    int totalBits = (int)data.size() * 4 - pad;
    for (int i = 0; i < totalBits; ++i) {
        int byte = hexValue(data[i / 4]);
        int bit = (byte >> (3 - (i & 3))) & 1;
        u = bit ? tree[u].right : tree[u].left;
        if (tree[u].leaf) {
            out.push_back(tree[u].ch);
            u = root;
        }
    }
    return out;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int S, D;
    if (!(cin >> S >> D)) return 0;
    vector<pair<string, string>> stat(S);
    for (auto &e : stat) cin >> e.first >> e.second;
    string treeCode;
    cin >> treeCode;
    int pos = 0;
    tree.reserve(treeCode.size());
    int root = parseTree(treeCode, pos);

    int N;
    cin >> N;
    vector<pair<string, string>> dyn;
    dyn.reserve(D);

    auto getEntry = [&](int id) -> pair<string, string> {
        if (id <= S) return stat[id - 1];
        return dyn[id - S - 1];
    };
    auto addEntry = [&](const pair<string, string> &e) {
        dyn.insert(dyn.begin(), e);
        if ((int)dyn.size() > D) dyn.pop_back();
    };

    for (int q = 0; q < N; ++q) {
        int type, id;
        cin >> type >> id;
        string key, value;
        if (type == 1) {
            pair<string, string> e = getEntry(id);
            cout << e.first << ": " << e.second << '\n';
        } else {
            if (id == 0) {
                cin >> key >> value;
                key = decodeString(key, root);
            } else {
                cin >> value;
                key = getEntry(id).first;
            }
            value = decodeString(value, root);
            cout << key << ": " << value << '\n';
            if (type == 2) addEntry({key, value});
        }
    }
    return 0;
}