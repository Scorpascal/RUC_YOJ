#include <iostream>
#include <queue>
using namespace std;

class RBTree {
private:
    enum Color {
        RED,
        BLACK
    };

    struct Node {
        int key;
        Color color;
        Node *left;
        Node *right;
        Node *parent;

        Node(int k = 0, Color c = BLACK)
            : key(k), color(c),
              left(nullptr), right(nullptr), parent(nullptr) {}
    };

    Node *root;
    Node *NIL;

private:
    void leftRotate(Node *x) {
        Node *y = x->right;

        x->right = y->left;

        if (y->left != NIL) {
            y->left->parent = x;
        }

        y->parent = x->parent;

        if (x->parent == NIL) {
            root = y;
        }
        else if (x == x->parent->left) {
            x->parent->left = y;
        }
        else {
            x->parent->right = y;
        }

        y->left = x;
        x->parent = y;
    }

    void rightRotate(Node *y) {
        Node *x = y->left;

        y->left = x->right;

        if (x->right != NIL) {
            x->right->parent = y;
        }

        x->parent = y->parent;

        if (y->parent == NIL) {
            root = x;
        }
        else if (y == y->parent->left) {
            y->parent->left = x;
        }
        else {
            y->parent->right = x;
        }

        x->right = y;
        y->parent = x;
    }

    void insertFixup(Node *z) {
        while (z->parent->color == RED) {

            if (z->parent == z->parent->parent->left) {
                Node *uncle = z->parent->parent->right;

                if (uncle->color == RED) {
                    z->parent->color = BLACK;
                    uncle->color = BLACK;
                    z->parent->parent->color = RED;

                    z = z->parent->parent;
                }
                else {
                    if (z == z->parent->right) {
                        z = z->parent;
                        leftRotate(z);
                    }

                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;

                    rightRotate(z->parent->parent);
                }
            }
            else {
                Node *uncle = z->parent->parent->left;

                if (uncle->color == RED) {
                    z->parent->color = BLACK;
                    uncle->color = BLACK;
                    z->parent->parent->color = RED;

                    z = z->parent->parent;
                }
                else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        rightRotate(z);
                    }

                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;

                    leftRotate(z->parent->parent);
                }
            }
        }

        root->color = BLACK;
    }

    Node *search(int key) {
        Node *cur = root;

        while (cur != NIL) {
            if (key == cur->key) {
                return cur;
            }

            if (key < cur->key) {
                cur = cur->left;
            }
            else {
                cur = cur->right;
            }
        }

        return NIL;
    }

    Node *minimum(Node *x) {
        while (x->left != NIL) {
            x = x->left;
        }

        return x;
    }

    void transplant(Node *u, Node *v) {
        if (u->parent == NIL) {
            root = v;
        }
        else if (u == u->parent->left) {
            u->parent->left = v;
        }
        else {
            u->parent->right = v;
        }

        /*
         * 即使 v == NIL，也要更新 parent。
         *
         * 删除修复过程中，如果实际删除的是黑色叶子，
         * x 会等于 NIL，此时必须通过 x->parent 找到父节点。
         */
        v->parent = u->parent;
    }

    /*
     * 删除后的双黑修复。
     *
     * ================================================================
     * 本题评测特殊性：
     *
     * 这道题不是只判断结果是否为“某一棵合法红黑树”，
     * 而是直接比较最终红黑树的层序遍历，因此旋转选择不同，
     * 即使得到的红黑树完全合法，也可能 Wrong Answer。
     *
     * 本题使用的规则与课程/邓俊辉教材的红黑树实现一致：
     *
     * 当兄弟为黑色且存在红孩子时：
     *
     *     若左右孩子均为红色，优先选择左孩子参与重构。
     *
     * 这一点与常见的 CLRS 删除实现略有不同。
     *
     * 对于 x 是左孩子的情况：
     *
     *             p
     *           /   \
     *          x     w
     *               / \
     *             RED RED
     *
     * CLRS 通常直接选择 w 的右孩子（外侧孩子）；
     *
     * 本题必须优先选择 w 的左孩子，因此要先对 w 右旋，
     * 再对 p 左旋。
     *
     * 下面标记为 [OJ SPECIAL] 的判断就是为适配本题评测。
     * ================================================================
     */
    void deleteFixup(Node *x) {
        while (x != root && x->color == BLACK) {

            /*
             * x 是父节点的左孩子。
             */
            if (x == x->parent->left) {
                Node *w = x->parent->right;

                /*
                 * Case 1:
                 * 兄弟 w 为红色。
                 */
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;

                    leftRotate(x->parent);

                    w = x->parent->right;
                }

                /*
                 * Case 2:
                 * 兄弟为黑色，而且两个孩子都是黑色。
                 *
                 * 将兄弟染红，把“双黑”问题上移到父节点。
                 */
                if (w->left->color == BLACK &&
                    w->right->color == BLACK) {

                    w->color = RED;
                    x = x->parent;
                }
                else {

                    /*
                     * =================================================
                     * [OJ SPECIAL]
                     *
                     * 注意这里不能写成常见 CLRS 版本：
                     *
                     *     if (w->right->color == BLACK)
                     *
                     * 因为当：
                     *
                     *     w->left  == RED
                     *     w->right == RED
                     *
                     * 时，CLRS 会直接使用右孩子。
                     *
                     * 但本题要求“左红孩子优先”，因此只要左孩子
                     * 是红色，就先让左孩子参与 3+4 重构。
                     *
                     * 这正是上传的大型失败数据产生 WA 的原因。
                     * =================================================
                     */
                    if (w->left->color == RED) {
                        w->left->color = BLACK;
                        w->color = RED;

                        rightRotate(w);

                        w = x->parent->right;
                    }

                    /*
                     * Case 4:
                     * 完成最终调整。
                     */
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    w->right->color = BLACK;

                    leftRotate(x->parent);

                    x = root;
                }
            }

            /*
             * x 是父节点的右孩子。
             *
             * 这一侧本身就符合“左孩子优先”的规则：
             * 兄弟 w 位于左边，优先使用 w 的左孩子正好就是
             * 标准算法中的外侧红孩子。
             */
            else {
                Node *w = x->parent->left;

                /*
                 * Case 1:
                 * 兄弟为红。
                 */
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;

                    rightRotate(x->parent);

                    w = x->parent->left;
                }

                /*
                 * Case 2:
                 * 兄弟两个孩子均黑。
                 */
                if (w->right->color == BLACK &&
                    w->left->color == BLACK) {

                    w->color = RED;
                    x = x->parent;
                }
                else {

                    /*
                     * 如果左孩子黑、右孩子红，
                     * 先把右侧红孩子旋到合适的位置。
                     */
                    if (w->left->color == BLACK) {
                        w->right->color = BLACK;
                        w->color = RED;

                        leftRotate(w);

                        w = x->parent->left;
                    }

                    /*
                     * 最终调整。
                     */
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    w->left->color = BLACK;

                    rightRotate(x->parent);

                    x = root;
                }
            }
        }

        x->color = BLACK;
    }

public:
    RBTree() {
        /*
         * 使用统一的黑色 NIL 哨兵，
         * 可以大幅简化红黑树删除时的空节点判断。
         */
        NIL = new Node();

        NIL->color = BLACK;
        NIL->left = NIL;
        NIL->right = NIL;
        NIL->parent = NIL;

        root = NIL;
    }

    void insert(int key) {
        /*
         * 题目说明红黑树不存在重复节点。
         * 为保险起见，如果输入重复插入，则直接忽略。
         */
        if (search(key) != NIL) {
            return;
        }

        Node *z = new Node(key, RED);

        z->left = NIL;
        z->right = NIL;

        Node *parent = NIL;
        Node *cur = root;

        /*
         * 普通 BST 插入。
         */
        while (cur != NIL) {
            parent = cur;

            if (key < cur->key) {
                cur = cur->left;
            }
            else {
                cur = cur->right;
            }
        }

        z->parent = parent;

        if (parent == NIL) {
            root = z;
        }
        else if (key < parent->key) {
            parent->left = z;
        }
        else {
            parent->right = z;
        }

        insertFixup(z);
    }

    void erase(int key) {
        Node *z = search(key);

        /*
         * 删除不存在的元素时直接忽略。
         */
        if (z == NIL) {
            return;
        }

        Node *y = z;
        Color originalColor = y->color;

        Node *x;

        /*
         * Case 1:
         * 没有左孩子。
         */
        if (z->left == NIL) {
            x = z->right;

            transplant(z, z->right);
        }

        /*
         * Case 2:
         * 没有右孩子。
         */
        else if (z->right == NIL) {
            x = z->left;

            transplant(z, z->left);
        }

        /*
         * Case 3:
         * 左右孩子都有。
         *
         * 本题课程实现选择“直接后继”，
         * 即右子树中的最小节点。
         */
        else {
            y = minimum(z->right);

            originalColor = y->color;

            x = y->right;

            if (y->parent == z) {
                /*
                 * 即使 x == NIL，也必须记录 parent，
                 * 后面的 deleteFixup 会使用它。
                 */
                x->parent = y;
            }
            else {
                transplant(y, y->right);

                y->right = z->right;
                y->right->parent = y;
            }

            transplant(z, y);

            y->left = z->left;
            y->left->parent = y;

            /*
             * y 移动到 z 原来的位置后，
             * 继承 z 的颜色。
             */
            y->color = z->color;
        }

        delete z;

        /*
         * 只有真正被移除位置上的节点原来为黑色时，
         * 才可能破坏黑高度，需要修复。
         */
        if (originalColor == BLACK) {
            deleteFixup(x);
        }
    }

    void levelOrder() {
        if (root == NIL) {
            cout << '\n';
            return;
        }

        queue<Node *> q;
        q.push(root);

        bool first = true;

        while (!q.empty()) {
            Node *cur = q.front();
            q.pop();

            if (!first) {
                cout << ' ';
            }

            cout << cur->key;
            first = false;

            if (cur->left != NIL) {
                q.push(cur->left);
            }

            if (cur->right != NIL) {
                q.push(cur->right);
            }
        }

        cout << '\n';
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    RBTree tree;

    while (n--) {
        int a, b;
        cin >> a >> b;

        if (a == 1) {
            tree.insert(b);
        }
        else {
            tree.erase(b);
        }
    }

    tree.levelOrder();

    return 0;
}