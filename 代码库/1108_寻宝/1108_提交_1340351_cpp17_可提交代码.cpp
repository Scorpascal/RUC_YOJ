#include <bits/stdc++.h>
using namespace std;

struct FastScanner {
	static constexpr size_t BUFSIZE = 1 << 20;
	char buf[BUFSIZE];
	size_t idx = 0, size = 0;

	inline char readChar() {
		if (idx >= size) {
			size = fread(buf, 1, BUFSIZE, stdin);
			idx = 0;
			if (size == 0) return 0;
		}
		return buf[idx++];
	}

	template <class T>
	bool readInt(T &out) {
		char c;
		do {
			c = readChar();
			if (!c) return false;
		} while (c <= ' ');
		bool neg = false;
		if (c == '-') {
			neg = true;
			c = readChar();
		}
		T val = 0;
		while (c > ' ') {
			val = val * 10 + (c - '0');
			c = readChar();
		}
		out = neg ? -val : val;
		return true;
	}
};

int main() {
	FastScanner fs;
	int T;
	if (!fs.readInt(T)) return 0;

	const long long NEG_INF = -(1LL << 60);
	while (T--) {
		int N, M;
		fs.readInt(N);
		fs.readInt(M);

		vector<long long> prevD(M + 1, NEG_INF), prevL(M + 1, 0);
		vector<long long> curD(M + 1, NEG_INF), curL(M + 1, NEG_INF);
		vector<long long> row(M + 1, 0);

		for (int i = 1; i <= N; i++) {
			for (int j = 1; j <= M; j++) {
				long long x;
				fs.readInt(x);
				row[j] = x;
			}

			for (int j = 0; j <= M; j++) {
				curD[j] = max(prevD[j], prevL[j]);
			}

			curL[M] = NEG_INF;
			for (int j = M - 1; j >= 0; j--) {
				long long stayLeft = curL[j + 1];
				long long turnDownToLeft = curD[j + 1] + row[j + 1];
				curL[j] = max(stayLeft, turnDownToLeft);
			}

			prevD.swap(curD);
			prevL.swap(curL);
		}

		long long ans = max(prevD[0], prevL[0]);
		printf("%lld\n", ans);
	}
	return 0;
}