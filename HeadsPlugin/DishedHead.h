//-----------------------------------------------------------------------------
// DishedHead.h : 碟形封头参数与几何计算声明
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "HeadTypes.h"

//-----------------------------------------------------------------------------
// 碟形封头输入参数
//-----------------------------------------------------------------------------
struct DishedHeadParams {
    double Di;   // 内径(mm)
    double Ri;   // 球冠(底部)内半径(mm)
    double ri;   // 过渡圆弧(折边)半径(mm)
    double T;    // 壁厚(mm)
    double L;    // 直边高度(mm)

    DishedHeadParams()
        : Di(1000.0), Ri(1000.0), ri(100.0), T(10.0), L(40.0) {}
};

//-----------------------------------------------------------------------------
// 碟形封头几何计算结果（内外轮廓 + 关键点）
// 坐标系：中心线 x=0，直边底端 y=0，向上为 +y，图形画在 x>=0 一侧
//-----------------------------------------------------------------------------
struct DishedHeadGeom {
    bool    valid;            // 是否满足相切条件
    CString errorMsg;         // 错误信息

    // 外轮廓（= 内轮廓向外等距偏移 T）
    LineData outerFlange;     // 外直边
    ArcData  outerKnuckle;    // 外折边圆弧
    ArcData  outerCrown;      // 外球冠圆弧

    // 内轮廓
    LineData innerFlange;     // 内直边
    ArcData  innerKnuckle;    // 内折边圆弧
    ArcData  innerCrown;      // 内球冠圆弧

    // 关键点
    AcGePoint2d bottomInner;    // 内底面点(R, 0)
    AcGePoint2d bottomOuter;    // 外底面点(R+T, 0)
    AcGePoint2d innerApex;      // 内顶点(球冠顶点)
    AcGePoint2d outerApex;      // 外顶点
    AcGePoint2d tangencyInner;  // 内折边与球冠的切点
    AcGePoint2d tangencyOuter;  // 外折边与球冠的切点

    double totalHeight;       // 总高度(= 外顶点 y 坐标)

    DishedHeadGeom() : valid(false), totalHeight(0.0) {}
};

// 校验参数；成功返回 true，失败时 errorMsg 中给出原因
bool ValidateDishedHead(const DishedHeadParams& p, CString& errorMsg);

// 自动调整参数使其满足相切条件；返回是否发生了调整
bool AutoAdjustDishedHead(DishedHeadParams& p);

// 计算碟形封头几何；成功返回 true
bool ComputeDishedHead(const DishedHeadParams& p, DishedHeadGeom& g);
