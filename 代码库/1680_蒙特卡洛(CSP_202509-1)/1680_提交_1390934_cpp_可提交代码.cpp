#include <iostream>
#include <iomanip>
using namespace std;

int main() {
    int n;
    double a;
    cin >> n >> a;

    int m = 0;

    for (int i = 0; i < n; ++i) {
        double x, y;
        cin >> x >> y;

        if (x * x + y * y <= a * a) {
            ++m;
        }
    }

    double ans = 4.0 * m / n;

    cout << fixed << setprecision(10) << ans << '\n';

    return 0;
}//傻逼