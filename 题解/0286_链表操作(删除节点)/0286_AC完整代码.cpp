#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node{
	int m;
	node *next;
};

node *insertNode( node *head, int n, int m)
{
	int i = 0;
	node *p, *pre, *p1 = (node *)malloc( sizeof(node) );
	
	p1->m = m;
	p1->next = NULL;
	
	
	p = head;
	pre = head;
	if ( n==0 )
	{
		p1->next = head;
		head = p1;
	}else
	{
		while( i<n && p!=NULL )
		{
			pre = p;
			p = p->next;
			i++;
		}
		pre->next = p1;
		p1->next = p;	
	}
	return head;
}

node * delNode( node *head, int n )
{
if (head == NULL || n <= 0) return head;
if (n == 1) {
    node *p = head;
    head = head->next;
    delete p;
    return head;
}
node *p = head;
for (int i = 1; i < n - 1 && p != NULL; ++i) p = p->next;
if (p != NULL && p->next != NULL) {
    node *q = p->next;
    p->next = q->next;
    delete q;
}
return head;    

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