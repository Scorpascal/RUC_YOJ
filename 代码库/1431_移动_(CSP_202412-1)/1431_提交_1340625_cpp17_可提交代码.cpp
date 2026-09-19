#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, k;
    cin >> n >> k;

    while (k--) {
        int x, y;
        string s;
        cin >> x >> y >> s;

        for (char c : s) {
            int nx = x, ny = y;
            if (c == 'f') ny++;
            else if (c == 'b') ny--;
            else if (c == 'l') nx--;
            else if (c == 'r') nx++;

            if (1 <= nx && nx <= n && 1 <= ny && ny <= n) {
                x = nx; y = ny;
            }
        }
        cout << x << ' ' << y << "\n";
    }
    return 0;
}