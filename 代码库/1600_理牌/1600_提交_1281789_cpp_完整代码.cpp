#include <bits/stdc++.h>

using namespace std;

struct card {
	int sp[25];
};
card c[10010];

struct minicard {
	int p[2];
};
minicard tran[10010], mc[10010];
int n, m, l;

int so(int l, int r, int pa) {
	if (l == r)
		return 0;
	int m = (l + r) / 2, cnt = 0;
	int lt = so(l, m, pa), rt = so(m + 1, r, pa), llen = m - l + 1, lp = l, rp = m + 1;
	for (int i = 0; i <= r - l; i++) {

		if (rp >= r + 1) {
			tran[l + i] = mc[lp];
			lp++;
		} else if (lp >= m + 1) {
			tran[l + i] = mc[rp];
			rp++;
		} else if (mc[lp].p[pa] <= mc[rp].p[pa]) {
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
	if (pa)
		return lt + rt + cnt;
	else
		return 0;
}

void check(int s, int e) {
	for (int i = 1; i <= n; i++) {
		mc[i].p[0] = c[i].sp[s];
		mc[i].p[1] = c[i].sp[e];
	}
	so(1, n, 0);
//	for (int i = 1; i <= n; i++) {
//		printf("%d %d\n", mc[i].p[0], mc[i].p[1]);
//
//	}
	printf("%d\n", so(1, n, 1));
}

int main() {
	scanf("%d%d", &n, &m);
	for (int i = 1; i <= n; i++) {
		for (int j = 1; j <= m; j++) {
			scanf("%d", &c[i].sp[j]);
		}
	}
	scanf("%d", &l);
	for (int i = 1; i <= l; i++) {
		int t1, t2;
		scanf("%d%d", &t1, &t2);
		check(t1, t2);
	}
	return 0;
}//1600AK