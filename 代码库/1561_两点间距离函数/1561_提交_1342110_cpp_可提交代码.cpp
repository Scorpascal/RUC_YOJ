#include<iostream>
#include<math.h>
#include<iomanip>
#include<cmath>
using namespace std;

double compute_dis(double x1,double y1,double z1,double x2,double y2,double z2){
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) + (z1-z2)*(z1-z2));
}

int main()
{
	double x1,y1,z1,x2,y2,z2,ans;
	cin>>x1>>y1>>z1>>x2>>y2>>z2;
	ans=compute_dis(x1,y1,z1,x2,y2,z2);
	cout<< fixed << setprecision(1) <<ans<<' ';
	return 0;
}