//-----------------------------------------------------------------------------
// EllipticalHead.h : 1:2 椭圆封头参数与几何计算声明
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "HeadTypes.h"

//-----------------------------------------------------------------------------
// 椭圆封头输入参数
//-----------------------------------------------------------------------------
struct EllipticalHeadParams {
    double Di;   // 内径(mm)
    double T;    // 壁厚(mm)
    double L;    // 直边高度(mm)

    EllipticalHeadParams()
        : Di(1000.0), T(10.0), L(40.0) {}
};

//-----------------------------------------------------------------------------
// 椭圆封头几何计算结果
// 说明：内轮廓椭圆长轴 = Di（半长轴 a = Di/2），短轴 = Di/2（半短轴 b = Di/4）
//       外轮廓 = 新椭圆，半长轴 a+T、半短轴 b+T（工程制图惯例）
// 坐标系：中心线 x=0，直边底端 y=0，向上为 +y
//-----------------------------------------------------------------------------
struct EllipticalHeadGeom {
    bool    valid;        // 是否有效
    CString errorMsg;     // 错误信息

    double a;             // 内椭圆半长轴 = Di/2
    double b;             // 内椭圆半短轴 = Di/4（即深度）

    AcGePoint2d innerApex;      // 内顶点 (0, L+b)
    AcGePoint2d outerApex;      // 外顶点 (0, L+b+T)
    AcGePoint2d bottomInner;    // 内底面 (a, 0)
    AcGePoint2d bottomOuter;    // 外底面 (a+T, 0)
    AcGePoint2d innerFlangeTop; // 内直边顶 (a, L)
    AcGePoint2d outerFlangeTop; // 外直边顶 (a+T, L)

    double totalHeight;   // 总高度 = L + b + T

    EllipticalHeadGeom() : valid(false), a(0.0), b(0.0), totalHeight(0.0) {}
};

// 校验参数
bool ValidateEllipticalHead(const EllipticalHeadParams& p, CString& errorMsg);

// 计算几何
bool ComputeEllipticalHead(const EllipticalHeadParams& p, EllipticalHeadGeom& g);
