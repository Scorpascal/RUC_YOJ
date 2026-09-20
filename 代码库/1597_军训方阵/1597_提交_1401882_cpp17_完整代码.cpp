#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
using namespace std;

int tran[500010], mc[500010];
int n;

long long so(long long l, long long r) {
	if (l == r)
		return 0;
	long long m = (l + r) / 2, cnt = 0;
	long long lt = so(l, m), rt = so(m + 1, r), llen = m - l + 1, lp = l, rp = m + 1;
	for (int i = 0; i <= r - l; i++) {

		if (rp >= r + 1) {
			tran[l + i] = mc[lp];
			lp++;
		} else if (lp >= m + 1) {
			tran[l + i] = mc[rp];
			rp++;
		} else if (mc[lp] <= mc[rp]) {
			tran[l + i] = mc[lp];
			lp++;
			llen--;
		} else {
			tran[l + i] = mc[rp];
			rp++;
			cnt += llen;
		}
	}
	for (int i = l; i <= r; i++) {
		mc[i] = tran[i];
	}
	return lt + rt + cnt;
}

int main() {
	scanf("%d", &n);
	for (int i = 1; i <= n; i++) {
		scanf("%d", &mc[i]);
	}
	printf("%lld", so(1, n));
	return 0;
}//1597AK