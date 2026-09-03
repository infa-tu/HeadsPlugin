//-----------------------------------------------------------------------------
// ConeGeometry.h : 锥体（CNA/CSA/CHD/CDA）参数与几何计算声明
// 约定：Di/Dis 为内径，α 为内斜壁半顶角，ri/rs 为内半径
//       内轮廓由参数确定，外轮廓 = 内轮廓向外偏移 δn
//       H0 自动计算（各段相加），仅标注；hs 为输入（仅 CHD/CDA）
//       CNA/CSA 无底部直边（斜壁直接到底面）
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "HeadTypes.h"

//-----------------------------------------------------------------------------
// 锥体类型
//-----------------------------------------------------------------------------
enum ConeType {
    CONE_CNA = 0,   // 无折边锥壳
    CONE_CSA = 1,   // 带折边锥壳
    CONE_CHD = 2,   // 简单锥形
    CONE_CDA = 3    // 带顶部圆角锥壳
};

//-----------------------------------------------------------------------------
// 锥体输入参数（不同类型使用其中不同的子集）
//-----------------------------------------------------------------------------
struct ConeParams {
    ConeType type;
    double Di;        // 大端内径(mm)
    double Dis;       // 小端内径(mm)
    double alphaDeg;  // 半顶角(度)，内斜壁与中心线夹角
    double dn;        // 壁厚 δn(mm)
    double h;         // 顶部直边/翻边高度(mm)，仅 CSA/CDA
    double ri;        // 顶部过渡内半径(mm)，仅 CSA/CDA
    double rs;        // 底部过渡内半径(mm)，仅 CHD/CDA
    double hs;        // 底部直边高度(mm)，仅 CHD/CDA

    ConeParams()
        : type(CONE_CNA), Di(1000.0), Dis(600.0), alphaDeg(30.0), dn(10.0),
          h(40.0), ri(100.0), rs(60.0), hs(25.0) {}
};

//-----------------------------------------------------------------------------
// 轮廓（顶点 + 凸度，类似多段线；bulges[i] 对应 pts[i] -> pts[i+1]）
//-----------------------------------------------------------------------------
struct ProfileData {
    AcGePoint2dArray pts;
    AcGeDoubleArray bulges;
};

//-----------------------------------------------------------------------------
// 锥体几何计算结果（右半边，底 -> 顶）
//-----------------------------------------------------------------------------
struct ConeGeom {
    bool    valid;         // 是否有效
    CString errorMsg;      // 错误信息

    ProfileData innerRight;  // 内轮廓（右半边）
    ProfileData outerRight;  // 外轮廓（右半边，向外偏移 δn）

    AcGePoint2d innerBottom; // 内底面角点 (Dis/2, 0)
    AcGePoint2d innerTop;    // 内顶面角点
    AcGePoint2d outerBottom; // 外底面角点
    AcGePoint2d outerTop;    // 外顶面角点

    double hs;             // 底部直边高度（CNA/CSA 为 0）
    double totalHeight;    // 总高 H0（自动计算）

    // 需要标注半径的圆弧（0=rs，1=ri，视类型）
    std::vector<ArcData> dimArcs;

    ConeGeom() : valid(false), hs(0.0), totalHeight(0.0) {}
};

// 校验参数
bool ValidateCone(const ConeParams& p, CString& errorMsg);

// 自动调整参数（设为默认值）
bool AutoAdjustCone(ConeParams& p);

// 计算锥体几何
bool ComputeCone(const ConeParams& p, ConeGeom& g);
