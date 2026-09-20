#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
struct Rect {
    int x1,y1,x2,y2;
};
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Rect A,B;
    cin>>A.x1>>A.y1>>A.x2>>A.y2;
    cin>>B.x1>>B.y1>>B.x2>>B.y2;
    // 规范化
    if(A.x1>A.x2) swap(A.x1,A.x2);
    if(A.y1>A.y2) swap(A.y1,A.y2);
    if(B.x1>B.x2) swap(B.x1,B.x2);
    if(B.y1>B.y2) swap(B.y1,B.y2);

    // 完全重合
    if(A.x1==B.x1 && A.y1==B.y1 && A.x2==B.x2 && A.y2==B.y2){
        cout<<12;
        return 0;
    }

    int w = min(A.x2,B.x2) - max(A.x1,B.x1);
    int h = min(A.y2,B.y2) - max(A.y1,B.y1);

    // 无面积交叠或仅点/边接触
    if(w<0 || h<0){
        cout<<0; // 完全不相交
        return 0;
    }
    if(w==0 && h==0){
        cout<<1; // 单点接触
        return 0;
    }
    if(w==0 && h>0){
        cout<<2; // 竖边贴合
        return 0;
    }
    if(h==0 && w>0){
        cout<<2; // 水平边贴合
        return 0;
    }

    // 有面积交叠 w>0 && h>0
    bool A_in_B = (A.x1>=B.x1 && A.x2<=B.x2 && A.y1>=B.y1 && A.y2<=B.y2);
    bool B_in_A = (B.x1>=A.x1 && B.x2<=A.x2 && B.y1>=A.y1 && B.y2<=A.y2);
    if(A_in_B || B_in_A){
        cout<<8; // 完全包含（可贴边）
        return 0;
    }

    bool A_contains_Bx = (A.x1<=B.x1 && A.x2>=B.x2);
    bool B_contains_Ax = (B.x1<=A.x1 && B.x2>=A.x2);
    bool A_contains_By = (A.y1<=B.y1 && A.y2>=B.y2);
    bool B_contains_Ay = (B.y1<=A.y1 && B.y2>=A.y2);

    // 十字交叉：两个轴分别被不同矩形包含
    if( (A_contains_Bx && B_contains_Ay) || (B_contains_Ax && A_contains_By) ){
        cout<<10;
        return 0;
    }

    // 单轴包含（另一轴仅部分重叠） -> 情形6
    bool one_axis_contain = (A_contains_Bx ^ B_contains_Ax) || (A_contains_By ^ B_contains_Ay);
    if(one_axis_contain){
        cout<<6;
        return 0;
    }

    // 剩下即：无轴包含但有面积交 -> 互相仅一角进入 -> 4
    cout<<4;
    return 0;
}