#include <stdio.h>
#include <string.h>
#include <stdlib.h>

____qcodep____ 

void realse(YellowPage *head)
{
    YellowPage *p = head, *pre;
    while (p != NULL)
    {
        pre = p, p = p->next;
        pre->next = NULL;
        free(pre);
    }
}

void display(YellowPage *head)
{
    YellowPage *node = head;
    printf("display data \n");
    while (node != NULL)
    {
        printf("%s %d \n", node->name, node->telNum);
        node = node->next;
    }
    realse(head);
}
int main()
{
    display(create());
    return 0;
}