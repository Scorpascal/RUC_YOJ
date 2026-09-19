#include <bits/stdc++.h>
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