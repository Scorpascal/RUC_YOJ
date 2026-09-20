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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, P, M;
    if (!(cin >> N >> P >> M)) return 0;

    vector<vector<long long>> A(N, vector<long long>(P));
    vector<vector<long long>> B(P, vector<long long>(M));
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < P; ++j)
            cin >> A[i][j];

    for (int i = 0; i < P; ++i)
        for (int j = 0; j < M; ++j)
            cin >> B[i][j];

    vector<vector<long long>> C(N, vector<long long>(M, 0));
    for (int i = 0; i < N; ++i) {
        for (int k = 0; k < P; ++k) {
            for (int j = 0; j < M; ++j) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            if (j) cout << ' ';
            cout << C[i][j];
        }
        cout << '\n';
    }
    return 0;
}