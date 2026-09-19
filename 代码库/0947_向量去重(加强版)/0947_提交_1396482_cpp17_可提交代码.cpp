#include <iostream>
using namespace std;

template <typename T>
class Vector {
private:
    T* elem;       // 存放元素
    int _size;     // 当前元素个数
    int capacity;  // 数组容量

    // 归并排序
    void mergeSort(int left, int right, T* temp) {
        if (left >= right) return;

        int mid = (left + right) / 2;

        mergeSort(left, mid, temp);
        mergeSort(mid + 1, right, temp);

        int i = left;
        int j = mid + 1;
        int k = left;

        // 合并两个有序区间
        while (i <= mid && j <= right) {
            if (elem[i] <= elem[j])
                temp[k++] = elem[i++];
            else
                temp[k++] = elem[j++];
        }

        while (i <= mid)
            temp[k++] = elem[i++];

        while (j <= right)
            temp[k++] = elem[j++];

        // 放回原数组
        for (int p = left; p <= right; p++)
            elem[p] = temp[p];
    }

public:
    // 构造函数
    Vector(int n) {
        capacity = n;
        _size = n;
        elem = new T[capacity];
    }

    // 析构函数
    ~Vector() {
        delete[] elem;
    }

    // 下标访问
    T& operator[](int index) {
        return elem[index];
    }

    // 返回大小
    int size() {
        return _size;
    }

    // 排序
    void sort() {
        T* temp = new T[_size];

        mergeSort(0, _size - 1, temp);

        delete[] temp;
    }

    // 对已经排好序的向量去重
    // 返回删除的元素个数
    int uniquify() {
        if (_size <= 1)
            return 0;

        int oldSize = _size;

        // i：最后一个不重复元素的位置
        // j：当前检查的位置
        int i = 0;

        for (int j = 1; j < _size; j++) {
            if (elem[j] != elem[i]) {
                elem[++i] = elem[j];
            }
        }

        _size = i + 1;

        return oldSize - _size;
    }
};


int main() {
    int n;
    cin >> n;

    Vector<int> v(n);

    for (int i = 0; i < n; i++) {
        cin >> v[i];
    }

    v.sort();

    int removed = v.uniquify();

    cout << removed << endl;

    return 0;
}