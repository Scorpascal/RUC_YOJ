#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
using namespace std;

typedef int DataType;

typedef struct LinkNode {
    DataType data;
    struct LinkNode *next;
} LinkNode;

typedef struct
{
    LinkNode *front, *rear;
    int cnt;
} LinkQueue;

LinkQueue queue;

LinkNode *CreateNode(DataType data) //创建结点
{
    LinkNode *node;

    node = (LinkNode *)malloc(sizeof(LinkNode));
    node->data = data;
    node->next = NULL;

    return node;
}

void DestroyNode(LinkNode *node) //销毁结点
{
    free(node);
}

void InsertAfterPtr(LinkNode *ptr, LinkNode *node) //在链表中ptr结点的后面插入node结点
{
    LinkNode *next = ptr->next;
    node->next = next;
    ptr->next = node;
}

void DeleteNextPtr(LinkNode *ptr) //从链表中删除ptr结点的下一个结点
{

LinkNode*temp=ptr->next;
if(temp!=NULL){
    ptr->next=temp->next;
    DestroyNode(temp);
}
else ptr->next=NULL;

}

int IsEmpty(LinkQueue * queue) //判断队列是否为空
{
    return queue->front==queue->rear;
}

void Enqueue(LinkQueue *queue, DataType data) //将元素data插入队列queue
{

LinkNode *node=CreateNode(data);
queue->rear=queue->rear->next=node;
queue->cnt++;

}

DataType Front(LinkQueue *queue) //获得队列queue的队首元素（不将队首元素出队）
{
    if (IsEmpty(queue)) {
        printf("underflow\n");
        exit(-1);
    }
    else {
        return queue->front->next->data;
    }
}

DataType Dequeue(LinkQueue *queue) //取出队列queue的队首元素（将队首元素出队）
{
    DataType ret = Front(queue);
    if (queue->front->next == queue->rear)
        queue->rear = queue->front; // 临界情况
    DeleteNextPtr(queue->front);
    queue->cnt--;
    return ret;
}

void InitQueue(LinkQueue *queue) //初始化队列
{
    queue->front = CreateNode(0);
    queue->rear = queue->front;
    queue->cnt = 0;
}

int main(){
    string team;
    cin >> team;
    int n = team.size();
    LinkQueue *white, *black;
    white = (LinkQueue*)malloc(sizeof(LinkQueue));
    black = (LinkQueue*)malloc(sizeof(LinkQueue));
    InitQueue(white);
    InitQueue(black);

    for (int i = 0; i < n; i++)
    {   
        if(team[i] == 'W'){
            Enqueue(white, i);
        }else{
            Enqueue(black, i);
        }
    }

    // 参考题目中的解题思路实现
    while(!IsEmpty(white) && !IsEmpty(black)){

    if(Front(white)<Front(black)){
        DataType w=Dequeue(white);
        Dequeue(black);
        Enqueue(white,w+n);

    }
    else{
        DataType b=Dequeue(black);
        Dequeue(white);
        Enqueue(black,b+n);
    }

    }

    if(IsEmpty(white)){
        cout << "Black Hat Win!" << endl;
    }else{
        cout << "White Hat Win!" << endl;
    }
    return 0;
}