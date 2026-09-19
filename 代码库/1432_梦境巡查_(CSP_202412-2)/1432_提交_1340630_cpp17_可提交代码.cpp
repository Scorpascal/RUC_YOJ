#include <bits/stdc++.h>
using namespace std;

struct Func {
    long long P; // constant part
    long long C; // linear shift: max(P, x + C)
};

// composition: f ∘ g
static inline Func compose(const Func& f, const Func& g) {
    Func r;
    r.C = f.C + g.C;
    r.P = max(f.P, g.P + f.C);
    return r;
}

// NOTE: avoid name conflict with std::apply
static inline long long evalFunc(const Func& f, long long x) {
    return max(f.P, x + f.C);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    vector<long long> a(n + 1);
    for (int i = 0; i <= n; i++) cin >> a[i];

    vector<long long> b(n + 1, 0); // b[1..n]
    for (int i = 1; i <= n; i++) cin >> b[i];

    vector<Func> f(n);
    for (int k = 0; k < n; k++) {
        f[k].P = a[k];
        f[k].C = a[k] - b[k + 1];
    }

    const long long NEG = -(1LL << 60);
    Func ID{NEG, 0};

    vector<Func> pref(n);
    if (n > 0) {
        pref[0] = f[0];
        for (int k = 1; k < n; k++) pref[k] = compose(pref[k - 1], f[k]);
    }

    vector<Func> suf(n + 1);
    suf[n] = ID;
    for (int k = n - 1; k >= 0; k--) suf[k] = compose(f[k], suf[k + 1]);

    long long base = a[n];

    for (int i = 1; i <= n; i++) {
        int idx = i - 1;

        Func left = (idx - 1 >= 0) ? pref[idx - 1] : ID;
        Func right = (idx + 1 <= n) ? suf[idx + 1] : ID;

        Func mid{a[idx], a[idx]};

        Func total = compose(left, compose(mid, right));
        long long ans = evalFunc(total, base);

        if (i > 1) cout << ' ';
        cout << ans;
    }
    cout << '\n';
    return 0;
}