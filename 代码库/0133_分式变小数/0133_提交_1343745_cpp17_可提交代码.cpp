#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    long long N, D;
    if (!(cin >> N >> D)) return 0;

    long long integerPart = N / D;
    long long rem = N % D;

    string result = to_string(integerPart);
    result.push_back('.');

    if (rem == 0) {
        result.push_back('0');
        cout << result << "\n";
        return 0;
    }

    string fractional;
    unordered_map<long long, int> firstPos; // remainder -> position in fractional

    while (rem != 0) {
        auto it = firstPos.find(rem);
        if (it != firstPos.end()) {
            int pos = it->second;
            fractional.insert(fractional.begin() + pos, '(');
            fractional.push_back(')');
            break;
        }

        firstPos[rem] = static_cast<int>(fractional.size());
        rem *= 10;
        fractional.push_back(static_cast<char>('0' + (rem / D)));
        rem %= D;
    }

    result += fractional;
    cout << result << "\n";
    return 0;
}
