
#include <bits/stdc++.h>
using namespace std;

class FastScanner {
public:
	static constexpr size_t BUFSIZE = 1 << 20;
	FastScanner() : idx_(0), size_(0) {}

	template <class T>
	bool readInt(T &out) {
		char c = nextChar();
		if (!c) return false;
		while (c <= ' ') {
			c = nextChar();
			if (!c) return false;
		}
		T sign = 1;
		if (c == '-') {
			sign = -1;
			c = nextChar();
		}
		T val = 0;
		while (c > ' ') {
			val = val * 10 + (c - '0');
			c = nextChar();
		}
		out = val * sign;
		return true;
	}

private:
	char nextChar() {
		if (idx_ >= size_) {
			size_ = fread(buf_, 1, BUFSIZE, stdin);
			idx_ = 0;
			if (size_ == 0) return 0;
		}
		return buf_[idx_++];
	}

	char buf_[BUFSIZE];
	size_t idx_;
	size_t size_;
};

int main() {
	FastScanner fs;
	int n = 0, k = 0;
	if (!fs.readInt(n)) return 0;
	fs.readInt(k);

	vector<int> c(n + 1);
	for (int i = 1; i <= n; i++) {
		fs.readInt(c[i]);
	}

	static constexpr long long NEG = LLONG_MIN / 4;
	vector<long long> best(k + 1, NEG);

	long long prefixSum = 0;
	long long dpPrev = 0;

	for (int i = 1; i <= n; i++) {
		long long v = 0;
		fs.readInt(v);
		long long prefixBefore = prefixSum;
		prefixSum += v;

		long long dp = dpPrev;
		long long b = best[c[i]];
		if (b != NEG) {
			dp = max(dp, prefixSum + b);
		}

		best[c[i]] = max(best[c[i]], dpPrev - prefixBefore);
		dpPrev = dp;
	}

	printf("%lld\n", dpPrev);
	return 0;
}

