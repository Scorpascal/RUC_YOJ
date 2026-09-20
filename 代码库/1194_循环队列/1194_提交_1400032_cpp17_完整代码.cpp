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

class MyCircularQueue {
public:
    explicit MyCircularQueue(int k) : a(k), n(k), head(0), tail(0), cnt(0) {}

    int Front() const {
        if (isEmpty()) return -1;
        return a[head];
    }

    int Rear() const {
        if (isEmpty()) return -1;
        int idx = (tail - 1 + n) % n;
        return a[idx];
    }

    bool enQueue(int value) {
        if (isFull()) return false;
        a[tail] = value;
        tail = (tail + 1) % n;
        ++cnt;
        return true;
    }

    bool deQueue() {
        if (isEmpty()) return false;
        head = (head + 1) % n;
        --cnt;
        return true;
    }

    bool isEmpty() const { return cnt == 0; }
    bool isFull() const { return cnt == n; }

private:
    vector<int> a;
    int n;
    int head, tail;
    int cnt;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    if (!(cin >> n >> m)) return 0;

    MyCircularQueue q(n);

    for (int i = 0; i < m; ++i) {
        int op;
        cin >> op;
        if (op == 0) {
            cout << q.Front() << "\n";
        } else if (op == 1) {
            cout << q.Rear() << "\n";
        } else if (op == 2) {
            int x;
            cin >> x;
            cout << (q.enQueue(x) ? 1 : 0) << "\n";
        } else if (op == 3) {
            cout << (q.deQueue() ? 1 : 0) << "\n";
        } else if (op == 4) {
            cout << (q.isEmpty() ? 1 : 0) << "\n";
        } else if (op == 5) {
            cout << (q.isFull() ? 1 : 0) << "\n";
        }
    }
    return 0;
}