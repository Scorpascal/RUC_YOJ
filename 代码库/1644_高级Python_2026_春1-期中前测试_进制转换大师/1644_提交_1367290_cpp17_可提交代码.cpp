#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	long long n;
	int b;
	if (!(cin >> n >> b)) {
		return 0;
	}

	const string digits = "0123456789ABCDEF";
	if (n == 0) {
		cout << '0';
		return 0;
	}

	string result;
	while (n > 0) {
		result.push_back(digits[n % b]);
		n /= b;
	}
	reverse(result.begin(), result.end());
	cout << result;
	return 0;
}
