#include<iostream>
#include<stdio.h>
#include<stdlib.h>

using namespace std;

typedef int Datatype;

typedef struct LinkNode {
    Datatype data;
    struct LinkNode *next;

} LinkNode;

LinkNode *head;

LinkNode *CreateNode (Datatype data){
    LinkNode *node;
    node =(LinkNode*)malloc(sizeof(LinkNode));
    node->data=data;
    node->next=NULL;
    return node;
}

void DestroyNode(LinkNode *node){
    free(node);
}

void Initialize(LinkNode **head){
    Datatype data=0;
    *head=CreateNode(data);
} 

void Finalize(LinkNode *head){
    LinkNode *iter,*tmp;
    iter=head;
    while(iter!=NULL){
        tmp=iter;
        iter=iter->next;
        DestroyNode(tmp);
    }
}

LinkNode *Idx2Ptr(LinkNode *head, int idx){
    LinkNode *ptr=head;
    if(idx<0) return NULL;
    while(idx>0&&ptr!=NULL){
        ptr=ptr->next;
        idx--;
    }
    return ptr;
}

int Ptr2Idx(LinkNode *head, LinkNode *ptr){
    LinkNode *iter=head;
    int idx=0;
    while(iter!=NULL&&iter!=ptr){
        iter=iter->next;
        idx++;
    }
    if(iter!=NULL) return idx;
    else return -1;
}

void InsertAfterPtr(LinkNode *p, Datatype x){
    LinkNode *s=CreateNode(x);
    s->next=p->next;
    p->next=s;
}

void InsertAfterIdx(LinkNode *head, int idx,Datatype x){
    LinkNode *p=Idx2Ptr(head,idx);
    if(p!=NULL) InsertAfterPtr(p,x);
}

void InsertB4tr(LinkNode *head, LinkNode *ptr, LinkNode *node){
    int idx=Ptr2Idx(head,ptr);//查找结点p的下标
    LinkNode *prev=Idx2Ptr(head,idx-1);//查找结点p的前驱结点
    if(prev!=NULL){
        node->next=ptr;
        prev->next=node;
    }
}

void InsertB4Idx(LinkNode *head, int idx, LinkNode *node){
    LinkNode *ptr= Idx2Ptr(head,idx);
    if(ptr!=NULL) InsertB4tr(head,ptr,node);
}

void DeleteNextPtr(LinkNode *ptr){
    LinkNode *next=ptr->next;
    if(next!=NULL){
        ptr->next=next->next;
        DestroyNode(next);
    }
}

void DeleteNextIdx(LinkNode *head, int idx){
    LinkNode *ptr=Idx2Ptr(head,idx);
    if(ptr!=NULL) DeleteNextPtr(ptr);
}

void DeleteCurrentPtr(LinkNode *head, LinkNode *ptr){
    int idx=Ptr2Idx(head,ptr);
    LinkNode *prev=Idx2Ptr(head,idx-1);
    LinkNode *next=ptr->next;
    if(prev!=NULL){
        prev->next=next;
        DestroyNode(ptr);
    }
}

void DeleteCurrentIdx(LinkNode *head, int idx){
    LinkNode *ptr=Idx2Ptr(head,idx);
    if(ptr!=NULL) DeleteCurrentPtr(head,ptr);
}

void Insert(LinkNode *head, LinkNode *ptr, LinkNode *ptrnext, LinkNode *tail,int pos,int n){
     for(int i=1;i<=pos/2;i++){
            InsertAfterPtr(tail,(ptr->data+ptrnext->data)/2);
            tail=tail->next;
            InsertAfterPtr(tail,(ptr->data-ptrnext->data)/2);
            tail=tail->next;
             ptr=ptr->next;
            ptrnext=ptrnext->next;
            
}
}

int main(){
    Initialize(&head);
    int n;
    LinkNode *ptr;
    ptr=head;
    scanf("%d",&n);
    for(int i=0;i<n;i++){
        int cnt;
        scanf("%d",&cnt);
        InsertAfterPtr(ptr,cnt);
        ptr=ptr->next;
    }
   
    int pos=2;

    while(pos<=n){
        ptr=head->next;
        LinkNode *ptrnext;
        ptrnext=Idx2Ptr(head,pos/2+1);
        LinkNode *tail;
        tail=Idx2Ptr(head,pos);    
         
        Insert(head,ptr,ptrnext,tail,pos,n);
                
        LinkNode *deleteptr;
        deleteptr = head->next;
        tail = Idx2Ptr(head,pos);
        LinkNode *boundary = (tail != NULL) ? tail->next : NULL;
        while(deleteptr != boundary){
            LinkNode *next = deleteptr->next;
            DeleteCurrentPtr(head, deleteptr);
            deleteptr = next;
        } //delete
        pos*=2;
 
}

    ptr=head->next;
    for(int i=0;i<n;i++){
        printf("%d ",ptr->data);
        ptr=ptr->next;
    }
    
    Finalize(head);
return 0;
}