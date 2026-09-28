#include <iomanip>
#include <iostream>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    double a;
    if (!(cin >> n >> a)) return 0;
    const double r2 = a * a;
    int inside = 0;
    for (int i = 0; i < n; ++i) {
        double x, y;
        cin >> x >> y;
        if (x * x + y * y <= r2 + 1e-12) ++inside;
    }
    cout << fixed << setprecision(6) << (4.0 * inside / n) << '\n';
    return 0;
}