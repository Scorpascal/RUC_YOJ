#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node{
	int m;
	node *next;
};

node *insertNode( node *head, int n, int m)
{
____qcodep____
}

node * delNode( node *head, int n )
{
	int i = 0;
	node *p, *pre;
	
	p = pre = head;
		
	while( i<(n-1) && p!=NULL )
	{
		pre = p;
		p = p->next;
		i++;
	}
	if ( p == head )
	{
		pre = head->next;
		free (head);
		return pre;
	}else{
		pre->next = p->next;
		free(p);
		return head;
	}
}


void printNode( node *head, int n )
{
	int i = 0;
	node *p, *pre;

	p = head;
	for ( i=2; i<=n; i++ )
	{
		p = p->next;
	}
	printf("%d\n", p->m);
	return;
}

int main()
{
	int n, m, k, i, j;
	char op;
	
	node *head = NULL;
	
	scanf("%d", &n);	//操作个数
	getchar();	
	for ( i=0; i<n; i++ )
	{
		scanf( "%c", &op );
		switch (op)
		{
		case 'C':
			scanf("%d%d", &k, &m);
			head = insertNode( head, k, m );
			getchar();
			break;
		case 'D':
			scanf("%d", &k );
			head = delNode( head, k );
			getchar();
			break;
		case 'P':
			scanf("%d", &k );
			printNode( head, k );
			getchar();
			break;
		}
	}
	return 0;
}