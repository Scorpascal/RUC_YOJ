#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdio>
using namespace std;
#define MaxRows 10005
int minIndexForRow[MaxRows];
class Matrix{private:int m,n;int *matrix;int accessElementCount,accessElementCountLimit;public:Matrix(){scanf("%d %d",&m,&n);matrix=new int[m*n];for(int i=0;i<m*n;i++)scanf("%d",&matrix[i]);accessElementCount=0;accessElementCountLimit=0.4*m*n;}~Matrix(){delete matrix;}int getElement(int row,int col){accessElementCount++;return matrix[row*n+col];}bool passAccessElementCountLimit(){return accessElementCount<=accessElementCountLimit;}int getRowNumber(){return m;}int getColNumber(){return n;}};
void solve(Matrix &matrix){int m=matrix.getRowNumber(),n=matrix.getColNumber();vector<int> ans(m,-1),rows(m),cols(n);for(int i=0;i<m;i++)rows[i]=i;for(int j=0;j<n;j++)cols[j]=j;function<void(const vector<int>&,const vector<int>&)> smawk=[&](const vector<int>& rs,const vector<int>& cs){if(rs.empty())return;vector<int> red;for(int c:cs){while(!red.empty()){int r=rs[red.size()-1];if(matrix.getElement(r,c)<matrix.getElement(r,red.back()))red.pop_back();else break;}if(red.size()<rs.size())red.push_back(c);}vector<int> odd;for(size_t i=1;i<rs.size();i+=2)odd.push_back(rs[i]);smawk(odd,red);int start=0;for(size_t i=0;i<rs.size();i+=2){int end=(i+1<rs.size()?0:(int)red.size()-1);if(i+1<rs.size()){int target=ans[rs[i+1]];end=(int)(find(red.begin(),red.end(),target)-red.begin());}int best=start;for(int j=start+1;j<=end;j++)if(matrix.getElement(rs[i],red[j])<matrix.getElement(rs[i],red[best]))best=j;ans[rs[i]]=red[best];start=best;}};smawk(rows,cols);for(int i=0;i<m;i++)minIndexForRow[i]=ans[i];}
int main(){Matrix matrix{};solve(matrix);if(matrix.passAccessElementCountLimit()){int m=matrix.getRowNumber();for(int i=0;i<m-1;++i)printf("%d ",minIndexForRow[i]);printf("%d",minIndexForRow[m-1]);}return 0;}