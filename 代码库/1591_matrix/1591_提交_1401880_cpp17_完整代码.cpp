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

int board[1010][1010], n;
bool book[1000010];
int main() {
	scanf("%d", &n);
	if (n % 2 == 0) {
		int m = n / 2 + 1;
		int cji = 1, c0 = 6, c1 = 4, c2 = 2;
		board[1][m] = 1;
		board[1][m - 1] = 8;
		book[1] = 1;
		book[8] = 1;
		for (int i = 2; i <= n; i++) {
			while (book[cji]) {
				cji += 2;
			}
			book[cji] = 1;
			board[i][m] = cji;
			if (cji % 3 == 1) {
				while (book[c2]) {
					c2 += 6;
				}
				book[c2] = 1;
				board[i][m - 1] = c2;
			}
			if (cji % 3 == 2) {
				while (book[c1]) {
					c1 += 6;
				}
				book[c1] = 1;
				board[i][m - 1] = c1;
			}
			if (cji % 3 == 0) {
				while (book[c0]) {
					c0 += 6;
				}
				book[c0] = 1;
				board[i][m - 1] = c0;
			}
		}
		bool boo = 1;
		int st = 4;
		for (int j = 1; j <= n && boo; j++) {
			for (int i = 1; i <= n && boo; i++) {
				if (board[i][j]) {
					boo = 0;
					break;
				}
				while (book[st]) {
					st += 2;
				}
				book[st] = 1;
				board[i][j] = st;
			}
		}
		st = 3;
		boo = 1;
		for (int j = n; j >= 1 && boo; j--) {
			for (int i = n; i >= 1 && boo; i--) {
				if (board[i][j]) {
					boo = 0;
					break;
				}
				while (book[st]) {
					st += 2;
				}
				book[st] = 1;
				board[i][j] = st;
			}
		}
	}
	if (n % 2 == 1) {
		int m = n / 2 + 1;
		board[m][m] = 7;
		board[m][m - 1] = 2;
		board[m - 1][m] = 8;
		board[m - 1][m + 1] = 1;
		book[1] = 1;
		book[2] = 1;
		book[7] = 1;
		book[8] = 1;
		int cji = 3, c0 = 6, c1 = 4, c2 = 2;
		for (int i = m + 1; i <= n; i++) {
			while (book[cji]) {
				cji += 2;
			}
			book[cji] = 1;
			board[i][m] = cji;
			if (cji % 3 == 1) {
				while (book[c2]) {
					c2 += 6;
				}
				book[c2] = 1;
				board[i][m - 1] = c2;
			}
			if (cji % 3 == 2) {
				while (book[c1]) {
					c1 += 6;
				}
				book[c1] = 1;
				board[i][m - 1] = c1;
			}
			if (cji % 3 == 0) {
				while (book[c0]) {
					c0 += 6;
				}
				book[c0] = 1;
				board[i][m - 1] = c0;
			}
		}
		for (int i = m - 2; i >= 1; i--) {
			while (book[cji]) {
				cji += 2;
			}
			book[cji] = 1;
			board[i][m + 1] = cji;
			if (cji % 3 == 1) {
				while (book[c2]) {
					c2 += 6;
				}
				book[c2] = 1;
				board[i][m] = c2;
			}
			if (cji % 3 == 2) {
				while (book[c1]) {
					c1 += 6;
				}
				book[c1] = 1;
				board[i][m] = c1;
			}
			if (cji % 3 == 0) {
				while (book[c0]) {
					c0 += 6;
				}
				book[c0] = 1;
				board[i][m] = c0;
			}
		}
		bool boo = 1;
		int st = 4;
		for (int j = 1; j <= n && boo; j++) {
			for (int i = 1; i <= n && boo; i++) {
				if (board[i][j]) {
					boo = 0;
					break;
				}
				while (book[st]) {
					st += 2;
				}
				book[st] = 1;
				board[i][j] = st;
			}
		}
		st = 3;
		boo = 1;
		for (int j = n; j >= 1 && boo; j--) {
			for (int i = n; i >= 1 && boo; i--) {
				if (board[i][j]) {
					boo = 0;
					break;
				}
				while (book[st]) {
					st += 2;
				}
				book[st] = 1;
				board[i][j] = st;
			}
		}
	}
	for (int i = 1; i <= n; i++) {
		for (int j = 1; j <= n; j++) {
			printf("%d ", board[i][j]);
		}
		printf("\n");
	}
	return 0;
}//1591AK