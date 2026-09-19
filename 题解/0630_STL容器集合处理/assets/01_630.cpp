#include <iostream>
#include <string>
#include <set>

using namespace std;

// add any code (or leave it emptry) you need below
____qcodep____

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
	____qcodep____
	Print(s1);
	Print(s2);
	set<int> s3;
	// add any code you need below: s3 = s1 - s2
	____qcodep____
	Print(s3);

	set<int> s4;
	// add any code you need below: s4 = s1 /\ s2
	____qcodep____
	Print(s4);

	set<int> s5;
	// add any code you need below: s5 = s1 \/ s2
	____qcodep____
	Print(s5);

	return 0;
}

