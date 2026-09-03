//-----------------------------------------------------------------------------
// DishedHead.cpp : 碟形封头几何计算实现
//
// 几何说明（半剖视图）：
//   内轮廓 = 直边(竖直) + 折边圆弧(半径 ri) + 球冠圆弧(半径 Ri)，三段相切
//   外轮廓 = 内轮廓向外等距偏移 T（即同心圆弧、半径各 +T）
//
// 相切条件推导：
//   球冠圆心 C1 = (0, yc)，折边圆心 C2 = (R-ri, L)，R = Di/2
//   两圆弧内切：|C1 C2| = Ri - ri
//   => (R-ri)^2 + (L-yc)^2 = (Ri-ri)^2
//   => yc = L - sqrt((Ri-R)(Ri+R-2ri))
//   要求：(Ri-R)(Ri+R-2ri) >= 0，即 Ri >= R 且 ri <= R
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "DishedHead.h"
#include <math.h>

//-----------------------------------------------------------------------------
// 校验参数
//-----------------------------------------------------------------------------
bool ValidateDishedHead(const DishedHeadParams& p, CString& errorMsg) {
    double R = p.Di / 2.0;
    if (p.Di <= 0.0) { errorMsg = _T("内径 Di 必须大于 0");       return false; }
    if (p.T  <= 0.0) { errorMsg = _T("壁厚 T 必须大于 0");        return false; }
    if (p.L  <  0.0) { errorMsg = _T("直边高度 L 不能为负");      return false; }
    if (p.Ri <= 0.0) { errorMsg = _T("球冠半径 Ri 必须大于 0");   return false; }
    if (p.ri <= 0.0) { errorMsg = _T("过渡圆弧半径 ri 必须大于 0"); return false; }
    if (p.Ri <  R)   { errorMsg = _T("球冠半径 Ri 不能小于 Di/2，无法相切"); return false; }
    if (p.ri >  R)   { errorMsg = _T("过渡圆弧半径 ri 不能大于 Di/2，无法相切"); return false; }
    return true;
}

//-----------------------------------------------------------------------------
// 自动调整参数使其满足相切
//-----------------------------------------------------------------------------
bool AutoAdjustDishedHead(DishedHeadParams& p) {
    bool changed = false;

    // 1) 折边半径过大(越过中心线)或无效 -> 取标准值 0.1*Di
    if (p.ri <= 0.0 || p.ri > p.Di / 2.0) {
        p.ri = 0.1 * p.Di;
        changed = true;
    }

    // 2) 球冠半径过小(无法相切) -> 取标准值 Di
    if (p.Ri <= 0.0 || p.Ri < p.Di / 2.0) {
        p.Ri = p.Di;
        changed = true;
    }

    return changed;
}

//-----------------------------------------------------------------------------
// 计算几何
//-----------------------------------------------------------------------------
bool ComputeDishedHead(const DishedHeadParams& p, DishedHeadGeom& g) {
    g = DishedHeadGeom();   // 重置

    // 校验参数
    CString err;
    if (!ValidateDishedHead(p, err)) {
        g.errorMsg = err;
        return false;
    }

    double R = p.Di / 2.0;       // 筒体内半径

    // 球冠圆心高度
    double h  = sqrt((p.Ri - R) * (p.Ri + R - 2.0 * p.ri));
    double yc = p.L - h;

    // 折边圆心
    double kcx = R - p.ri;       // x：与直边(竖直)相切
    double kcy = p.L;            // y：= 直边顶

    // 折边与球冠的切点 P（在 C1->C2 连线上，距 C1 为 Ri）
    double d  = p.Ri - p.ri;                     // |C1 C2|
    double px = (p.Ri / d) * (kcx - 0.0);        // 切点 x
    double py = yc + (p.Ri / d) * (kcy - yc);    // 切点 y

    // 切点相对折边圆心的角度（因 C1、C2、P 共线，同时也是球冠圆弧的起始角）
    double angTan = atan2(py - kcy, px - kcx);

    // ---------- 内轮廓 ----------
    g.innerFlange    = { AcGePoint2d(R, 0.0), AcGePoint2d(R, p.L) };
    g.innerKnuckle   = { AcGePoint2d(kcx, kcy), p.ri, 0.0, angTan };
    g.innerCrown     = { AcGePoint2d(0.0, yc), p.Ri, angTan, kPi / 2.0 };
    g.innerApex      = AcGePoint2d(0.0, yc + p.Ri);
    g.tangencyInner  = AcGePoint2d(px, py);

    // ---------- 外轮廓（同心，半径 +T，角度相同）----------
    g.outerFlange    = { AcGePoint2d(R + p.T, 0.0), AcGePoint2d(R + p.T, p.L) };
    g.outerKnuckle   = { AcGePoint2d(kcx, kcy), p.ri + p.T, 0.0, angTan };
    g.outerCrown     = { AcGePoint2d(0.0, yc), p.Ri + p.T, angTan, kPi / 2.0 };
    g.outerApex      = AcGePoint2d(0.0, yc + p.Ri + p.T);

    // 外折边与球冠切点（同角度，位于外折边圆弧上）
    double ux = cos(angTan), uy = sin(angTan);
    g.tangencyOuter = AcGePoint2d(kcx + (p.ri + p.T) * ux, kcy + (p.ri + p.T) * uy);

    // 关键点
    g.bottomInner = AcGePoint2d(R, 0.0);
    g.bottomOuter = AcGePoint2d(R + p.T, 0.0);
    g.totalHeight = g.outerApex.y;

    g.valid = true;
    return true;
}
