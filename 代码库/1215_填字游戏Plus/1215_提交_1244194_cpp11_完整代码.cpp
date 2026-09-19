#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

static inline bool okCell(char cell, char w) {
    return cell == '0' || cell == '*' || cell == w;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int m, n;
    if(!(cin >> m >> n)) return 0;

    vector<vector<char>> A(m, vector<char>(n));
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            cin >> A[i][j];
        }
    }
    string word;
    cin >> word;
    const int L = (int)word.size();
    const int INF = 0x3f3f3f3f;

    int bestZeros = INF;
    int sR = -1, sC = -1, eR = -1, eC = -1;

    auto sameCoverageH = [&](int r1, int c1, int r2, int c2,
                             int r3, int c3, int r4, int c4) -> bool {
        if (r1 != r2 || r3 != r4 || r1 != r3) return false;
        int cl1 = min(c1, c2), cr1 = max(c1, c2);
        int cl2 = min(c3, c4), cr2 = max(c3, c4);
        return cl1 == cl2 && cr1 == cr2;
    };
    auto sameCoverageV = [&](int r1, int c1, int r2, int c2,
                             int r3, int c3, int r4, int c4) -> bool {
        if (c1 != c2 || c3 != c4 || c1 != c3) return false;
        int rt1 = min(r1, r2), rb1 = max(r1, r2);
        int rt2 = min(r3, r4), rb2 = max(r3, r4);
        return rt1 == rt2 && rb1 == rb2;
    };

    auto update = [&](int r1, int c1, int r2, int c2, int zeros, bool preferLRorUD) {
        if (zeros < bestZeros) {
            bestZeros = zeros;
            sR = r1; sC = c1; eR = r2; eC = c2;
            return;
        }
        if (zeros == bestZeros && sR != -1) {
            bool sameH = sameCoverageH(r1, c1, r2, c2, sR, sC, eR, eC);
            bool sameV = sameCoverageV(r1, c1, r2, c2, sR, sC, eR, eC);
            if ((sameH || sameV) && preferLRorUD) {
                if (r1 != sR || c1 != sC) {
                    sR = r1; sC = c1; eR = r2; eC = c2;
                }
            }
        }
    };

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            if (!okCell(A[i][j], word[0])) continue;

            if (L <= j + 1) {
                bool leftOK  = (j - L < 0) || (A[i][j - L] == '1');
                bool rightOK = (j + 1 >= n) || (A[i][j + 1] == '1');
                if (leftOK && rightOK) {
                    int cnt = 0; bool ok = true;
                    for (int k = 0; k < L; ++k) {
                        int c = j - k;
                        if (!okCell(A[i][c], word[k])) { ok = false; break; }
                        if (A[i][c] == '0' && ++cnt > bestZeros) { ok = false; break; }
                    }
                    if (ok) update(i, j, i, j - L + 1, cnt, false);
                }
            }

            if (j + L - 1 < n) {
                bool leftOK  = (j - 1 < 0) || (A[i][j - 1] == '1');
                bool rightOK = (j + L >= n) || (A[i][j + L] == '1');
                if (leftOK && rightOK) {
                    int cnt = 0; bool ok = true;
                    for (int k = 0; k < L; ++k) {
                        int c = j + k;
                        if (!okCell(A[i][c], word[k])) { ok = false; break; }
                        if (A[i][c] == '0' && ++cnt > bestZeros) { ok = false; break; }
                    }
                    if (ok) update(i, j, i, j + L - 1, cnt, true);
                }
            }
        }
    }

    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            if (!okCell(A[i][j], word[0])) continue;

            if (L <= i + 1) {
                bool upOK   = (i - L < 0) || (A[i - L][j] == '1');
                bool downOK = (i + 1 >= m) || (A[i + 1][j] == '1');
                if (upOK && downOK) {
                    int cnt = 0; bool ok = true;
                    for (int k = 0; k < L; ++k) {
                        int r = i - k;
                        if (!okCell(A[r][j], word[k])) { ok = false; break; }
                        if (A[r][j] == '0' && ++cnt > bestZeros) { ok = false; break; }
                    }
                    if (ok) update(i, j, i - L + 1, j, cnt, false);
                }
            }

            if (i + L - 1 < m) {
                bool upOK   = (i - 1 < 0) || (A[i - 1][j] == '1');
                bool downOK = (i + L >= m) || (A[i + L][j] == '1');
                if (upOK && downOK) {
                    int cnt = 0; bool ok = true;
                    for (int k = 0; k < L; ++k) {
                        int r = i + k;
                        if (!okCell(A[r][j], word[k])) { ok = false; break; }
                        if (A[r][j] == '0' && ++cnt > bestZeros) { ok = false; break; }
                    }
                    if (ok) update(i, j, i + L - 1, j, cnt, true);
                }
            }
        }
    }

    if (bestZeros == INF) cout << "No";
    else cout << sR << ' ' << sC << '\n' << eR << ' ' << eC;
    return 0;
}