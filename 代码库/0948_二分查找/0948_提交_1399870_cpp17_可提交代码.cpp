#include <iostream>
#include <vector>
using namespace std;
int main() {
    int n, target;
    cin >> n >> target;
    vector<int> a(n);
    for (int& x : a) cin >> x;
    int lo = 0, hi = n - 1, ca = 0;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        ++ca;
        if (target < a[mid]) hi = mid;
        else {
            ++ca;
            if (a[mid] < target) lo = mid + 1;
            else break;
        }
    }
    lo = 0; hi = n - 1;
    int cb = 0;
    while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        ++cb;
        if (target < a[mid]) hi = mid;
        else lo = mid;
    }
    ++cb;
    lo = 0; hi = n - 1;
    int cc = 0;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        ++cc;
        if (target < a[mid]) hi = mid;
        else lo = mid + 1;
    }
    vector<int> fib{1, 1};
    while (fib.back() < n) fib.push_back(fib[fib.size()-1] + fib[fib.size()-2]);
    int k = (int)fib.size() - 1;
    lo = 0; hi = n - 1;
    int cf = 0;
    while (lo < hi) {
        while (fib[k] > hi - lo) --k;
        int mid = lo + fib[k] - 1;
        ++cf;
        if (target < a[mid]) hi = mid;
        else {
            ++cf;
            if (a[mid] < target) lo = mid + 1;
            else break;
        }
    }
    cout << ca << ' ' << cb << ' ' << cc << ' ' << cf << endl;
}
