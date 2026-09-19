#include<iostream>
#include<math.h>
#include<cmath>
#include<iomanip>
using namespace std;
double all_number[20000];

____qcodep____//请在这里实现compare函数和swap函数

int main()
{
	int num;
	cin>>num;
	for(int i=0;i<num;i++)
	{
		cin>>all_number[i];
	}
	for(int i=0;i<num;i++)
	{
		for(int j=0;j<num-i-1;j++)
		{
			if(compare(all_number[j],all_number[j+1]))
			{
				swap(all_number,j,j+1);
			}
		}

	}
	for(int i=0;i<num;i++)
	{
		cout<< fixed << setprecision(4) <<all_number[i] <<' ';
	}
	return 0;

}