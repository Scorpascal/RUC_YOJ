// ...existing code...
#include <bits/stdc++.h>
using namespace std;

struct Person {
    string id;
    string name;
    char gender;
    int age;
    int cnt;
    int idx;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<Person> v;
    for (int i = 0; i < 3; ++i) {
        Person p;
        if (!(cin >> p.id >> p.name >> p.gender >> p.age >> p.cnt)) return 0;
        p.idx = i;
        v.push_back(p);
    }

    sort(v.begin(), v.end(), [](const Person& a, const Person& b) {
        if (a.cnt != b.cnt) return a.cnt > b.cnt;
        return a.idx < b.idx;
    });

    for (const auto& p : v) {
        cout << p.id << ',' << p.name << ',' << p.gender << ',' << p.age << ',' << p.cnt << '\n';
    }
    return 0;
}
// ...existing code...