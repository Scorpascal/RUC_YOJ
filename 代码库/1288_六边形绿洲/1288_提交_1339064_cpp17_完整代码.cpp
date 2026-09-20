#include<iostream>
#include<queue>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string>

using namespace std;

struct point
{
	int bes[7];
	int state;
	int dist;
	int use[7];
	bool con[7];
	bool book;
};
struct answer
{
	int id;
	int cnt;
};
answer ans[2010];
int n,st,ed,length;
point ma[2010][7];
bool book[2010];
void print()
{
	printf("%d\n",length);
	for(int i=1;i<=length;i++)
	{
		printf("%d L %d\n",ans[i].id,ans[i].cnt);
	}
	exit(0);
}
void bfs(int start)
{
	queue<int> q;
	q.push(start);
	book[start]=1;
	ma[start][0].dist=1;
	int cnt=0;
	while(!q.empty())
	{
		int t=q.front();
		q.pop();
		for(int i=0;i<=5;i++)
		{
			if(ma[t][0].bes[i]!=0&&!book[ma[t][0].bes[i]])
			{
				q.push(ma[t][0].bes[i]);
				book[ma[t][0].bes[i]]=1;
				ma[ma[t][0].bes[i]][0].dist=ma[t][0].dist+1;
			}
		}
	}
	return ;
}
void preset(int poi)
{
	bool tbook[7];
	int temp[7];
	temp[6]=100000;
	for(int i=0;i<=5;i++)
	{
		tbook[i]=0;
		if(ma[poi][0].bes[i])
			temp[i]=ma[ma[poi][0].bes[i]][0].dist;
		else
			temp[i]=10000;
	}
	// for(int i=0;i<=5;i++)
	// {
	// 	printf("%6d",temp[i]);
	// }
	// printf("\n");
	for(int i=0;i<=5;i++)
	{
		int index=6;
		for(int j=0;j<=5;j++)
		{
			if(!tbook[j]&&temp[j]<temp[index]) index=j;
		}
		tbook[index]=1;
		ma[poi][0].use[i]=index;
	}
}
void dfs(int poi,int forward)
{
	if(poi==ed) print();
	else
	{
		for(int i=0;i<=5;i++)
		{
			if(ma[poi][forward].con[ma[poi][forward].use[i]]&&book[ma[poi][forward].bes[ma[poi][forward].use[i]]])
			{
				for(int j=0;j<=5;j++)
				{
					if(ma[ma[poi][forward].bes[ma[poi][forward].use[i]]][j].con[(ma[poi][forward].use[i]+3)%6])
					{
						book[ma[poi][forward].bes[ma[poi][forward].use[i]]]=0;
						// printf("     %d %d\n",ma[poi][forward].use[i],j);
						if(j!=0)
						{
							length++;
							ans[length].id=ma[poi][forward].bes[ma[poi][forward].use[i]];
							ans[length].cnt=j;
							dfs(ma[poi][forward].bes[ma[poi][forward].use[i]],j);
							length--;
						}
						else
						{
							dfs(ma[poi][forward].bes[ma[poi][forward].use[i]],j);
						}
						book[ma[poi][forward].bes[ma[poi][forward].use[i]]]=1;
					}
				}
			}
		}
	}
}
int main()
{
    scanf("%d",&n);
    int frost;
    for(int i=1;i<=n;i++)
    {
    	scanf("%d",&ma[i][0].state);
    	if(ma[i][0].state==0) st=i;
    	if(ma[i][0].state==2) ed=i;
    	for(int j=0;j<=5;j++)
    	{
    		scanf("%d",&ma[i][0].bes[j]);
		}
		for(int j=0;j<=5;j++)
    	{
    		scanf("%d",&ma[i][0].con[j]);
		}
	}
	bfs(ed);
	for(int i=1;i<=n;i++)
	{
		preset(i);
	}
	for(int i=1;i<=n;i++)
	{
		for(int j=1;j<=5;j++)
		{
			ma[i][j]=ma[i][0];
			for(int k=0;k<=5;k++)
			{
				ma[i][j].con[k]=ma[i][0].con[(k+j)%6];
			}
		}
	}
	book[st]=0;
	// for(int i=1;i<=n;i++)
	// {
	// 	for(int k=0;k<=5;k++)
	// 	{
	// 		printf("%d %d\n",i,ma[i][k].dist);
	// 		for(int j=0;j<=5;j++)
	// 		{
	// 			printf("%d",ma[i][k].con[j]);
	// 		}
	// 		printf("\n");
	// 	}
	// }
	dfs(st,0);

	for(int i=1;i<=5;i++)
	{
		length++;
		ans[length].id=st;
		ans[length].cnt=i;
		dfs(st,i);
		length--;
	}
    return 0;
}
/*
全逆时针转 L
7
1
2 3 4 5 6 7
1 1 1 0 1 0
0
0 0 3 1 7 0
0 1 0 0 0 0
1
0 0 0 4 1 2
1 1 0 0 0 0
1
3 0 0 0 5 1
0 1 0 0 0 0
1
1 4 0 0 0 6
0 0 1 0 1 0
2
7 1 5 0 0 0
0 0 0 0 0 1
1
0 2 1 6 0 0
1 0 0 0 0 1
*/