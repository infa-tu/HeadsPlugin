//-----------------------------------------------------------------------------
// EllipticalHead.cpp : 1:2 椭圆封头几何计算实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "EllipticalHead.h"

//-----------------------------------------------------------------------------
// 校验参数
//-----------------------------------------------------------------------------
bool ValidateEllipticalHead(const EllipticalHeadParams& p, CString& errorMsg) {
    if (p.Di <= 0.0) { errorMsg = _T("内径 Di 必须大于 0");  return false; }
    if (p.T  <= 0.0) { errorMsg = _T("壁厚 T 必须大于 0");   return false; }
    if (p.L  <  0.0) { errorMsg = _T("直边高度 L 不能为负"); return false; }
    return true;
}

//-----------------------------------------------------------------------------
// 计算几何
//-----------------------------------------------------------------------------
bool ComputeEllipticalHead(const EllipticalHeadParams& p, EllipticalHeadGeom& g) {
    g = EllipticalHeadGeom();   // 重置

    CString err;
    if (!ValidateEllipticalHead(p, err)) {
        g.errorMsg = err;
        return false;
    }

    // 内椭圆半长轴 / 半短轴
    g.a = p.Di / 2.0;
    g.b = p.Di / 4.0;

    // 关键点
    g.innerApex      = AcGePoint2d(0.0, p.L + g.b);
    g.outerApex      = AcGePoint2d(0.0, p.L + g.b + p.T);
    g.bottomInner    = AcGePoint2d(g.a, 0.0);
    g.bottomOuter    = AcGePoint2d(g.a + p.T, 0.0);
    g.innerFlangeTop = AcGePoint2d(g.a, p.L);
    g.outerFlangeTop = AcGePoint2d(g.a + p.T, p.L);

    g.totalHeight = p.L + g.b + p.T;

    g.valid = true;
    return true;
}
