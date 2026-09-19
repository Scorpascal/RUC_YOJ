#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if (!(cin >> n)) return 0;

    while (n--) {
        string s;
        cin >> s;

        bool hasLetter = false, hasDigit = false, hasSpecial = false;
        int cnt[128] = {0};
        int maxCnt = 0;

        for (unsigned char ch : s) {
            if (isalpha(ch)) hasLetter = true;
            else if (isdigit(ch)) hasDigit = true;
            else if (ch == '*' || ch == '#') hasSpecial = true;

            if (ch < 128) {
                maxCnt = max(maxCnt, ++cnt[ch]);
            }
        }

        if (!(hasLetter && hasDigit && hasSpecial)) {
            cout << 0 << "\n";
        } else if (maxCnt <= 2) {
            cout << 2 << "\n";
        } else {
            cout << 1 << "\n";
        }
    }
    return 0;
}