#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
using namespace std;

struct Node {
    bool leaf;
    char ch;
    Node *left, *right;

    Node() : leaf(false), ch(0), left(nullptr), right(nullptr) {}

    Node(char c) : leaf(true), ch(c), left(nullptr), right(nullptr) {}
};

int S, D;

vector<pair<string, string>> staticTable;
deque<pair<string, string>> dynamicTable;

Node* root;

// 前序字符串建 Huffman 树
Node* buildTree(const string& s, int& pos) {
    if (s[pos] == '1') {
        ++pos;

        char ch = s[pos];
        ++pos;

        return new Node(ch);
    }

    // s[pos] == '0'
    ++pos;

    Node* node = new Node();

    node->left = buildTree(s, pos);
    node->right = buildTree(s, pos);

    return node;
}

// 十六进制字符 -> 数值
int hexValue(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';

    return c - 'a' + 10;
}

// 两位十六进制 -> 一个字节
int hexByte(char a, char b) {
    return hexValue(a) * 16 + hexValue(b);
}

// Huffman 解码
string decodeHuffman(const string& s) {
    // s 形如 H898007
    // 去掉最开始的 H

    int hexLen = (int)s.size() - 1;

    vector<int> bytes;

    for (int i = 1; i < (int)s.size(); i += 2) {
        bytes.push_back(hexByte(s[i], s[i + 1]));
    }

    // 最后一个字节保存补零数量 p
    int p = bytes.back();
    bytes.pop_back();

    int totalBits = (int)bytes.size() * 8 - p;

    string ans;

    Node* cur = root;

    for (int bitIndex = 0; bitIndex < totalBits; ++bitIndex) {
        int byteIndex = bitIndex / 8;
        int offset = 7 - bitIndex % 8;

        int bit = (bytes[byteIndex] >> offset) & 1;

        if (bit == 0)
            cur = cur->left;
        else
            cur = cur->right;

        if (cur->leaf) {
            ans.push_back(cur->ch);
            cur = root;
        }
    }

    return ans;
}

// 解码题目中的字符串
string decodeString(const string& s) {
    // 普通字符串
    if (s[0] != 'H')
        return s;

    // HH 转义
    if (s.size() >= 2 && s[1] == 'H')
        return s.substr(1);

    // 剩下的以 H 开头的就是 Huffman
    return decodeHuffman(s);
}

// 根据编号取表项
pair<string, string> getEntry(int index) {
    if (index <= S) {
        return staticTable[index - 1];
    }

    int pos = index - S - 1;

    return dynamicTable[pos];
}

// 插入动态表
void insertDynamic(const string& key, const string& value) {
    dynamicTable.push_front({key, value});

    if ((int)dynamicTable.size() > D) {
        dynamicTable.pop_back();
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cin >> S >> D;

    staticTable.resize(S);

    for (int i = 0; i < S; ++i) {
        cin >> staticTable[i].first >> staticTable[i].second;
    }

    string treeString;
    cin >> treeString;

    int pos = 0;
    root = buildTree(treeString, pos);

    int N;
    cin >> N;

    while (N--) {
        int type, index;

        cin >> type >> index;

        // 类型 1：完整表项引用
        if (type == 1) {
            auto [key, value] = getEntry(index);

            cout << key << ": " << value << '\n';
        }

        // 类型 2 / 3
        else {
            string key;
            string value;

            if (index == 0) {
                // 2 0 k v
                // 或
                // 3 0 k v

                string encodedKey, encodedValue;

                cin >> encodedKey >> encodedValue;

                key = decodeString(encodedKey);
                value = decodeString(encodedValue);
            }

            else {
                // 2 i v
                // 或
                // 3 i v

                string encodedValue;
                cin >> encodedValue;

                // 只引用字段名
                key = getEntry(index).first;

                value = decodeString(encodedValue);
            }

            cout << key << ": " << value << '\n';

            // 只有类型 2 插入动态表
            if (type == 2) {
                insertDynamic(key, value);
            }
        }
    }

    return 0;
}