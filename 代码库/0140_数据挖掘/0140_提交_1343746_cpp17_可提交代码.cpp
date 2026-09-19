#include <bits/stdc++.h>
using namespace std;

static inline string trim(string s) {
    auto notSpace = [](unsigned char c) { return !isspace(c); };
    s.erase(s.begin(), find_if(s.begin(), s.end(), notSpace));
    s.erase(find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    getline(cin, line);

    const string kBorn = "born in ";
    const string kMajor = "My major is ";

    string birth, major;

    // 解析生日
    size_t pBorn = line.find(kBorn);
    if (pBorn != string::npos) {
        size_t start = pBorn + kBorn.size();
        size_t end = line.find(',', start);
        if (end == string::npos) end = line.size();
        birth = trim(line.substr(start, end - start));
    }

    // 解析专业
    size_t pMajor = line.find(kMajor);
    if (pMajor != string::npos) {
        size_t start = pMajor + kMajor.size();
        size_t end = line.find('.', start);
        if (end == string::npos) end = line.size();
        major = trim(line.substr(start, end - start));
    }

    cout << birth << ", " << major << "\n";
    return 0;
}