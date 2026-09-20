#include <iostream>
#include <algorithm>
using namespace std;

struct Node {
    int val;
    int height;
    Node *left, *right;

    Node(int v) : val(v), height(1), left(nullptr), right(nullptr) {}
};

int Height(Node* x) {
    return x ? x->height : 0;
}

void updateHeight(Node* x) {
    if (x) {
        x->height = max(Height(x->left), Height(x->right)) + 1;
    }
}

int balanceFactor(Node* x) {
    return Height(x->left) - Height(x->right);
}


// 3 + 4 重构
//
//       a < b < c
//
// 重构为：
//
//          b
//        /   \
//       a     c
//      / \   / \
//     T0 T1 T2 T3
//
Node* connect34(Node* a, Node* b, Node* c,
                Node* T0, Node* T1, Node* T2, Node* T3) {

    a->left = T0;
    a->right = T1;

    c->left = T2;
    c->right = T3;

    b->left = a;
    b->right = c;

    // 注意：先更新下面两个，再更新根
    updateHeight(a);
    updateHeight(c);
    updateHeight(b);

    return b;
}


// 对失衡节点 g 进行 3+4 重构
Node* rebalance(Node* g) {

    // 左子树过高
    if (balanceFactor(g) > 1) {

        Node* p = g->left;

        // LL
        if (balanceFactor(p) >= 0) {

            Node* v = p->left;

            return connect34(
                v, p, g,
                v->left,
                v->right,
                p->right,
                g->right
            );
        }

        // LR
        else {

            Node* v = p->right;

            return connect34(
                p, v, g,
                p->left,
                v->left,
                v->right,
                g->right
            );
        }
    }

    // 右子树过高
    else if (balanceFactor(g) < -1) {

        Node* p = g->right;

        // RR
        if (balanceFactor(p) <= 0) {

            Node* v = p->right;

            return connect34(
                g, p, v,
                g->left,
                p->left,
                v->left,
                v->right
            );
        }

        // RL
        else {

            Node* v = p->left;

            return connect34(
                g, v, p,
                g->left,
                v->left,
                v->right,
                p->right
            );
        }
    }

    return g;
}


// AVL 插入
Node* insertNode(Node* root, int x) {

    if (root == nullptr) {
        return new Node(x);
    }

    if (x < root->val) {
        root->left = insertNode(root->left, x);
    }
    else if (x > root->val) {
        root->right = insertNode(root->right, x);
    }
    else {
        // 题目说 AVL 中不存在相同节点
        // 如果重复插入，直接忽略
        return root;
    }

    updateHeight(root);

    if (abs(balanceFactor(root)) > 1) {
        root = rebalance(root);
    }

    return root;
}


// 找右子树最小节点
Node* findMin(Node* root) {

    while (root->left != nullptr) {
        root = root->left;
    }

    return root;
}


// AVL 删除
Node* deleteNode(Node* root, int x) {

    if (root == nullptr) {
        return nullptr;
    }

    if (x < root->val) {
        root->left = deleteNode(root->left, x);
    }
    else if (x > root->val) {
        root->right = deleteNode(root->right, x);
    }
    else {

        // 0 个或 1 个孩子
        if (root->left == nullptr || root->right == nullptr) {

            Node* child;

            if (root->left != nullptr)
                child = root->left;
            else
                child = root->right;

            delete root;

            return child;
        }

        // 两个孩子：
        // 用右子树最小值（中序后继）替换
        else {

            Node* successor = findMin(root->right);

            root->val = successor->val;

            root->right =
                deleteNode(root->right, successor->val);
        }
    }

    updateHeight(root);

    // 删除以后也可能失衡
    if (abs(balanceFactor(root)) > 1) {
        root = rebalance(root);
    }

    return root;
}


// 先序遍历
bool firstOutput = true;

void preorder(Node* root) {

    if (root == nullptr) {
        return;
    }

    if (!firstOutput) {
        cout << ' ';
    }

    cout << root->val;
    firstOutput = false;

    preorder(root->left);
    preorder(root->right);
}


int main() {

    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    Node* root = nullptr;

    while (n--) {

        int a, b;
        cin >> a >> b;

        if (a == 1) {
            root = insertNode(root, b);
        }
        else {
            root = deleteNode(root, b);
        }
    }

    preorder(root);
    cout << '\n';

    return 0;
}