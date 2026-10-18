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
