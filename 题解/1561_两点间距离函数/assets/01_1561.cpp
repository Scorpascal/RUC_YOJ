#include<iostream>
#include<math.h>
#include<iomanip>
#include<cmath>
using namespace std;

____qcodep____

int main()
{
	double x1,y1,z1,x2,y2,z2,ans;
	cin>>x1>>y1>>z1>>x2>>y2>>z2;
	ans=compute_dis(x1,y1,z1,x2,y2,z2);
	cout<< fixed << setprecision(1) <<ans<<' ';
	return 0;
}