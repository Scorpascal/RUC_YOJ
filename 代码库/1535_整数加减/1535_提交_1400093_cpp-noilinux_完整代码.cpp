#include <iostream>
using namespace std;
int main() {
    int a, b, c, d, e;
    if (!(cin >> a >> b >> c >> d >> e)) return 0;
    int result = a + b + c - d - e;
    cout << a << "+" << b << "+" << c << "-" << d << "-" << e << "=" << result;
    return 0;
}