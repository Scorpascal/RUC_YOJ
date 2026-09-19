node *newNode = new node();
    newNode->m = m;
    newNode->next = NULL;
    if (n == 0) {
        newNode->next = head;
        return newNode;
    }
    node *cur = head;
    for (int i = 1; i < n; ++i) cur = cur->next;
    newNode->next = cur->next;
    cur->next = newNode;
    return head;
