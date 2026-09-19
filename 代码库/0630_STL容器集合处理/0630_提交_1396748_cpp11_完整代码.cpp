#include <iostream>
#include <string>
#include <set>

using namespace std;

// add any code (or leave it emptry) you need below
#include <algorithm>
#include <iterator>

void Print(set<int> &s) {
	int i = 0, l = s.size();
	for (int v : s) {
		cout << v;
		if (i < l -1) cout << " ";
	}
	cout << endl;
}

int main() {
	int m, n, val;
	cin >> m >> n;
	set<int> s1, s2;
	// read data to s1 and s2
	for (int i = 0; i < m; ++i) { cin >> val; s1.insert(val); }
for (int i = 0; i < n; ++i) { cin >> val; s2.insert(val); }
	Print(s1);
	Print(s2);
	set<int> s3;
	// add any code you need below: s3 = s1 - s2
	set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s3, s3.begin()));
	Print(s3);

	set<int> s4;
	// add any code you need below: s4 = s1 /\ s2
	set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s4, s4.begin()));
	Print(s4);

	set<int> s5;
	// add any code you need below: s5 = s1 \/ s2
	set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(s5, s5.begin()));
	Print(s5);

	return 0;
}

