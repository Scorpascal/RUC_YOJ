#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, L;
    cin >> n >> L;

    vector<vector<int>> A(n, vector<int>(n));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            cin >> A[i][j];
        }
    }

    const vector<string> pattern = {
        "111111111",
        "100100101",
        "100111110",
        "100001100",
        "111111100"
    };

    // L + 1 是为了能够访问 diff[r + 1]
    vector<int> diff(L + 1, 0);

    // 枚举所有 5 × 9 子矩阵
    for (int i = 0; i + 4 < n; ++i) {
        for (int j = 0; j + 8 < n; ++j) {

            int minWhite = L;   // 白色位置的最小灰度
            int maxBlack = -1;  // 黑色位置的最大灰度

            for (int x = 0; x < 5; ++x) {
                for (int y = 0; y < 9; ++y) {

                    int value = A[i + x][j + y];

                    if (pattern[x][y] == '1') {
                        minWhite = min(minWhite, value);
                    } else {
                        maxBlack = max(maxBlack, value);
                    }
                }
            }

            int left = maxBlack + 1;
            int right = minWhite;

            // 阈值还必须属于 [0, L-1]
            left = max(left, 0);
            right = min(right, L - 1);

            if (left <= right) {
                ++diff[left];

                if (right + 1 < L) {
                    --diff[right + 1];
                }
            }
        }
    }

    int cur = 0;

    for (int k = 0; k < L; ++k) {
        cur += diff[k];

        if (cur > 0) {
            cout << k << '\n';
        }
    }

    return 0;
}