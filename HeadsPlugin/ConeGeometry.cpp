//-----------------------------------------------------------------------------
// ConeGeometry.cpp : 锥体（CNA/CSA/CHD/CDA）几何计算实现
//
// 坐标：中心线 x=0，底面 y=0，向上为 +y，右半边 x>=0
// 内轮廓由 Di/Dis/α/ri/rs/h/hs 确定；外轮廓 = 内轮廓向外偏移 δn
// H0 自动计算（各段相加），仅标注
// CNA/CSA 无底部直边（斜壁直接到底面，高度 0）
// 圆角圆心方向（保证相切平滑）：
//   底部（法兰→斜壁，向右拐）：圆心在外侧
//   顶部（斜壁→法兰，向左拐）：圆心在内侧
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "ConeGeometry.h"
#include <math.h>

//-----------------------------------------------------------------------------
// 工具函数
//-----------------------------------------------------------------------------
static double D2R(double deg) { return deg * kPi / 180.0; }

static void ArcFromBulge(const AcGePoint2d& p1, const AcGePoint2d& p2, double b,
                         AcGePoint2d& c, double& r) {
    double dx = p2.x - p1.x, dy = p2.y - p1.y;
    double chord = sqrt(dx * dx + dy * dy);
    if (fabs(b) < 1e-12 || chord < 1e-12) {
        c = AcGePoint2d((p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0);
        r = 0.0;
        return;
    }
    double theta = 4.0 * atan(b);
    r = chord / (2.0 * fabs(sin(theta / 2.0)));
    double mx = (p1.x + p2.x) / 2.0, my = (p1.y + p2.y) / 2.0;
    double nx = -dy / chord, ny = dx / chord;
    double dist = chord / (2.0 * tan(theta / 2.0));
    c = AcGePoint2d(mx + nx * dist, my + ny * dist);
}

static double ArcBulge(const AcGePoint2d& c, const AcGePoint2d& from, const AcGePoint2d& to) {
    double a1 = atan2(from.y - c.y, from.x - c.x);
    double a2 = atan2(to.y - c.y, to.x - c.x);
    double sweep = a2 - a1;
    while (sweep >  kPi) sweep -= 2.0 * kPi;
    while (sweep < -kPi) sweep += 2.0 * kPi;
    return tan(sweep / 4.0);
}

static void StartProf(ProfileData& pr, double x, double y) {
    pr.pts.append(AcGePoint2d(x, y));
}

static void AddPt(ProfileData& pr, double x, double y, double bulge) {
    pr.pts.append(AcGePoint2d(x, y));
    pr.bulges.append(bulge);
}

//-----------------------------------------------------------------------------
// 将内轮廓向外偏移 δn（材料在右侧），得到外轮廓
// bottomIsFlange / topIsFlange：底/顶部是否为竖直法兰（否则为斜壁切口）
//-----------------------------------------------------------------------------
static ProfileData OffsetOutward(const ProfileData& inner, double dn, double alphaRad,
                                 bool bottomIsFlange, bool topIsFlange) {
    int n = inner.pts.length();
    double ca = cos(alphaRad), sa = sin(alphaRad);
    ProfileData out;

    struct E { bool arc; double px, py, dx, dy, cx, cy, r; };
    std::vector<E> e(n - 1);

    for (int i = 0; i < n - 1; ++i) {
        double b = inner.bulges[i];
        AcGePoint2d p1 = inner.pts[i], p2 = inner.pts[i + 1];
        double vx = p2.x - p1.x, vy = p2.y - p1.y;
        double len = sqrt(vx * vx + vy * vy);
        if (fabs(b) < 1e-12 || len < 1e-12) {
            double dx = vx / len, dy = vy / len;
            double nx = dy, ny = -dx;   // 右法线
            e[i].arc = false;
            e[i].px = p1.x + dn * nx; e[i].py = p1.y + dn * ny;
            e[i].dx = dx; e[i].dy = dy;
        } else {
            AcGePoint2d c; double r;
            ArcFromBulge(p1, p2, b, c, r);
            double r2 = r + (b > 0.0 ? dn : -dn);
            e[i].arc = true;
            e[i].cx = c.x; e[i].cy = c.y; e[i].r = r2;
        }
    }

    // 底部端点：法兰→水平外移 dn；斜壁裸端→垂直于内壁(斜线)外移 dn
    if (bottomIsFlange)
        out.pts.append(AcGePoint2d(inner.pts[0].x + dn, inner.pts[0].y));
    else
        out.pts.append(AcGePoint2d(inner.pts[0].x + dn * ca, inner.pts[0].y - dn * sa));

    // 内部顶点
    for (int i = 1; i < n - 1; ++i) {
        const E& A = e[i - 1];
        const E& B = e[i];
        if (!A.arc && !B.arc) {
            double d = A.dx * B.dy - A.dy * B.dx;
            double t = ((B.px - A.px) * B.dy - (B.py - A.py) * B.dx) / d;
            out.pts.append(AcGePoint2d(A.px + t * A.dx, A.py + t * A.dy));
        } else if (A.arc && !B.arc) {
            // 从圆心 A 向直线 B 作垂足（切点）
            double t = (A.cx - B.px) * B.dx + (A.cy - B.py) * B.dy;
            out.pts.append(AcGePoint2d(B.px + t * B.dx, B.py + t * B.dy));
        } else if (!A.arc && B.arc) {
            // 从圆心 B 向直线 A 作垂足（切点）
            double t = (B.cx - A.px) * A.dx + (B.cy - A.py) * A.dy;
            out.pts.append(AcGePoint2d(A.px + t * A.dx, A.py + t * A.dy));
        } else {
            out.pts.append(AcGePoint2d((A.cx + B.cx) / 2.0, (A.cy + B.cy) / 2.0));
        }
    }

    // 顶部端点：法兰→水平外移 dn；斜壁裸端→垂直于内壁(斜线)外移 dn
    if (topIsFlange)
        out.pts.append(AcGePoint2d(inner.pts[n - 1].x + dn, inner.pts[n - 1].y));
    else
        out.pts.append(AcGePoint2d(inner.pts[n - 1].x + dn * ca, inner.pts[n - 1].y - dn * sa));

    // 凸度
    for (int i = 0; i < n - 1; ++i) {
        if (e[i].arc)
            out.bulges.append(ArcBulge(AcGePoint2d(e[i].cx, e[i].cy), out.pts[i], out.pts[i + 1]));
        else
            out.bulges.append(0.0);
    }

    return out;
}

//-----------------------------------------------------------------------------
// 校验参数
//-----------------------------------------------------------------------------
bool ValidateCone(const ConeParams& p, CString& errorMsg) {
    if (p.Di <= 0.0) { errorMsg = _T("大端内径 Di 必须大于 0"); return false; }
    if (p.Dis <= 0.0) { errorMsg = _T("小端内径 Dis 必须大于 0"); return false; }
    if (p.Di <= p.Dis) { errorMsg = _T("大端内径 Di 必须大于小端内径 Dis"); return false; }
    if (p.alphaDeg <= 0.0 || p.alphaDeg >= 85.0) { errorMsg = _T("半顶角 α 应在 (0, 85) 度之间"); return false; }
    if (p.dn <= 0.0) { errorMsg = _T("壁厚 δn 必须大于 0"); return false; }
    if (p.dn >= p.Dis / 2.0) { errorMsg = _T("壁厚 δn 过厚（应小于 Dis/2）"); return false; }

    switch (p.type) {
    case CONE_CSA:
        if (p.h < 0.0) { errorMsg = _T("顶部直边高度 h 不能为负"); return false; }
        if (p.ri <= 0.0) { errorMsg = _T("顶部过渡内半径 ri 必须大于 0"); return false; }
        if (p.ri > p.Di / 2.0) { errorMsg = _T("顶部过渡内半径 ri 过大（应 ≤ Di/2）"); return false; }
        break;
    case CONE_CHD:
        if (p.rs <= 0.0) { errorMsg = _T("底部过渡内半径 rs 必须大于 0"); return false; }
        if (p.rs >= p.Dis / 2.0) { errorMsg = _T("底部过渡内半径 rs 过大（应 < Dis/2）"); return false; }
        if (p.hs < 0.0) { errorMsg = _T("底部直边高度 hs 不能为负"); return false; }
        break;
    case CONE_CDA:
        if (p.h < 0.0) { errorMsg = _T("顶部翻边高度 h 不能为负"); return false; }
        if (p.ri <= 0.0) { errorMsg = _T("顶部过渡内半径 ri 必须大于 0"); return false; }
        if (p.ri > p.Di / 2.0) { errorMsg = _T("顶部过渡内半径 ri 过大（应 ≤ Di/2）"); return false; }
        if (p.rs <= 0.0) { errorMsg = _T("底部过渡内半径 rs 必须大于 0"); return false; }
        if (p.rs >= p.Dis / 2.0) { errorMsg = _T("底部过渡内半径 rs 过大（应 < Dis/2）"); return false; }
        if (p.hs < 0.0) { errorMsg = _T("底部直边高度 hs 不能为负"); return false; }
        break;
    default:
        break;
    }
    return true;
}

//-----------------------------------------------------------------------------
// 自动调整参数
//-----------------------------------------------------------------------------
bool AutoAdjustCone(ConeParams& p) {
    bool changed = false;
    if (p.Di <= 0.0 || p.Di <= p.Dis) { p.Di = 1000.0; p.Dis = 600.0; changed = true; }
    if (p.Dis <= 0.0) { p.Dis = 600.0; changed = true; }
    if (p.alphaDeg <= 0.0 || p.alphaDeg >= 85.0) { p.alphaDeg = 30.0; changed = true; }
    if (p.dn <= 0.0) { p.dn = 10.0; changed = true; }
    if (p.dn >= p.Dis / 2.0) { p.dn = p.Dis / 10.0; changed = true; }

    switch (p.type) {
    case CONE_CSA:
        if (p.h < 0.0) { p.h = 40.0; changed = true; }
        if (p.ri <= 0.0 || p.ri > p.Di / 2.0) { p.ri = 0.1 * p.Di; changed = true; }
        break;
    case CONE_CHD:
        if (p.rs <= 0.0 || p.rs >= p.Dis / 2.0) { p.rs = 0.1 * p.Dis; changed = true; }
        if (p.hs < 0.0) { p.hs = 25.0; changed = true; }
        break;
    case CONE_CDA:
        if (p.h < 0.0) { p.h = 40.0; changed = true; }
        if (p.ri <= 0.0 || p.ri > p.Di / 2.0) { p.ri = 0.1 * p.Di; changed = true; }
        if (p.rs <= 0.0 || p.rs >= p.Dis / 2.0) { p.rs = 0.1 * p.Dis; changed = true; }
        if (p.hs < 0.0) { p.hs = 25.0; changed = true; }
        break;
    default:
        break;
    }
    return changed;
}

//-----------------------------------------------------------------------------
// CNA：无折边锥壳（水平切口顶 + 斜壁，无底直边，底部尖角）
//-----------------------------------------------------------------------------
static bool ComputeCNA(const ConeParams& p, ConeGeom& g) {
    double Rtop = p.Di / 2.0, Rbot = p.Dis / 2.0;
    double a = D2R(p.alphaDeg), ta = tan(a);

    g.hs = 0.0;                              // 无底部直边
    g.totalHeight = (Rtop - Rbot) / ta;      // H0 = 斜壁垂直高

    StartProf(g.innerRight, Rbot, 0.0);
    AddPt(g.innerRight, Rtop, g.totalHeight, 0.0);   // 斜壁

    g.outerRight = OffsetOutward(g.innerRight, p.dn, a, false, false);

    g.innerBottom = AcGePoint2d(Rbot, 0.0);
    g.innerTop    = AcGePoint2d(Rtop, g.totalHeight);
    g.outerBottom = g.outerRight.pts[0];
    g.outerTop    = g.outerRight.pts[g.outerRight.pts.length() - 1];
    return true;
}

//-----------------------------------------------------------------------------
// CSA：带折边锥壳（顶直边 h + 内凹折边 ri + 斜壁，无底直边）
//-----------------------------------------------------------------------------
static bool ComputeCSA(const ConeParams& p, ConeGeom& g) {
    double Rtop = p.Di / 2.0, Rbot = p.Dis / 2.0;
    double a = D2R(p.alphaDeg), ta = tan(a), ca = cos(a), sa = sin(a);
    double ri = p.ri;

    g.hs = 0.0;
    double slopeH = (Rtop - Rbot - ri * (1.0 - ca)) / ta;  // 斜壁垂直高
    g.totalHeight = p.h + ri * sa + slopeH;                // H0

    double yk = g.totalHeight - p.h;                       // 折边圆心 y
    double Tkx = Rtop - ri * (1.0 - ca);
    double Tky = yk - ri * sa;

    StartProf(g.innerRight, Rbot, 0.0);
    AddPt(g.innerRight, Tkx, Tky, 0.0);            // 斜壁
    AddPt(g.innerRight, Rtop, yk, tan(a / 4.0));   // 折边(逆时针)
    AddPt(g.innerRight, Rtop, g.totalHeight, 0.0); // 顶直边

    g.outerRight = OffsetOutward(g.innerRight, p.dn, a, false, true);

    g.innerBottom = AcGePoint2d(Rbot, 0.0);
    g.innerTop    = AcGePoint2d(Rtop, g.totalHeight);
    g.outerBottom = g.outerRight.pts[0];
    g.outerTop    = g.outerRight.pts[g.outerRight.pts.length() - 1];

    ArcData ad;
    ad.center = AcGePoint2d(Rtop - ri, yk);
    ad.radius = ri;
    ad.startAngle = -a;
    ad.endAngle = 0.0;
    g.dimArcs.push_back(ad);
    return true;
}

//-----------------------------------------------------------------------------
// CHD：简单锥形（水平切口顶 + 斜壁 + 底部外凸圆角 rs + 底直边 hs）
//-----------------------------------------------------------------------------
static bool ComputeCHD(const ConeParams& p, ConeGeom& g) {
    double Rtop = p.Di / 2.0, Rbot = p.Dis / 2.0;
    double a = D2R(p.alphaDeg), ta = tan(a), ca = cos(a), sa = sin(a);
    double rs = p.rs;

    g.hs = p.hs;
    double slopeH = (Rtop - Rbot - rs * (1.0 - ca)) / ta;  // 斜壁垂直高
    g.totalHeight = g.hs + rs * sa + slopeH;               // H0

    double Tbx = Rbot + rs - rs * ca;
    double Tby = g.hs + rs * sa;

    StartProf(g.innerRight, Rbot, 0.0);
    AddPt(g.innerRight, Rbot, g.hs, 0.0);            // 底直边
    AddPt(g.innerRight, Tbx, Tby, tan(-a / 4.0));    // 底部圆角(顺时针)
    AddPt(g.innerRight, Rtop, g.totalHeight, 0.0);   // 斜壁

    g.outerRight = OffsetOutward(g.innerRight, p.dn, a, true, false);

    g.innerBottom = AcGePoint2d(Rbot, 0.0);
    g.innerTop    = AcGePoint2d(Rtop, g.totalHeight);
    g.outerBottom = g.outerRight.pts[0];
    g.outerTop    = g.outerRight.pts[g.outerRight.pts.length() - 1];

    ArcData ad;
    ad.center = AcGePoint2d(Rbot + rs, g.hs);
    ad.radius = rs;
    ad.startAngle = kPi - a;
    ad.endAngle = kPi;
    g.dimArcs.push_back(ad);
    return true;
}

//-----------------------------------------------------------------------------
// CDA：带顶部圆角锥壳（顶翻边 h + 外凸 ri + 斜壁 + 外凸 rs + 底直边 hs）
//-----------------------------------------------------------------------------
static bool ComputeCDA(const ConeParams& p, ConeGeom& g) {
    double Rtop = p.Di / 2.0, Rbot = p.Dis / 2.0;
    double a = D2R(p.alphaDeg), ta = tan(a), ca = cos(a), sa = sin(a);
    double ri = p.ri, rs = p.rs;

    g.hs = p.hs;
    double slopeH = (Rtop - Rbot - (ri + rs) * (1.0 - ca)) / ta;  // 斜壁垂直高
    g.totalHeight = p.h + g.hs + (ri + rs) * sa + slopeH;         // H0

    double y1 = g.totalHeight - p.h;                              // 顶部圆角圆心 y
    double Tbx = Rbot + rs - rs * ca;
    double Tby = g.hs + rs * sa;
    double Ttx = Rtop - ri * (1.0 - ca);
    double Tty = y1 - ri * sa;

    StartProf(g.innerRight, Rbot, 0.0);
    AddPt(g.innerRight, Rbot, g.hs, 0.0);            // 底直边
    AddPt(g.innerRight, Tbx, Tby, tan(-a / 4.0));    // 底部圆角(顺时针)
    AddPt(g.innerRight, Ttx, Tty, 0.0);              // 斜壁
    AddPt(g.innerRight, Rtop, y1, tan(a / 4.0));     // 顶部圆角(逆时针)
    AddPt(g.innerRight, Rtop, g.totalHeight, 0.0);   // 顶翻边

    g.outerRight = OffsetOutward(g.innerRight, p.dn, a, true, true);

    g.innerBottom = AcGePoint2d(Rbot, 0.0);
    g.innerTop    = AcGePoint2d(Rtop, g.totalHeight);
    g.outerBottom = g.outerRight.pts[0];
    g.outerTop    = g.outerRight.pts[g.outerRight.pts.length() - 1];

    ArcData ad1;
    ad1.center = AcGePoint2d(Rbot + rs, g.hs);
    ad1.radius = rs;
    ad1.startAngle = kPi - a;
    ad1.endAngle = kPi;
    g.dimArcs.push_back(ad1);

    ArcData ad2;
    ad2.center = AcGePoint2d(Rtop - ri, y1);
    ad2.radius = ri;
    ad2.startAngle = -a;
    ad2.endAngle = 0.0;
    g.dimArcs.push_back(ad2);
    return true;
}

//-----------------------------------------------------------------------------
// 计算锥体几何（分发）
//-----------------------------------------------------------------------------
bool ComputeCone(const ConeParams& p, ConeGeom& g) {
    g = ConeGeom();

    CString err;
    if (!ValidateCone(p, err)) {
        g.errorMsg = err;
        return false;
    }

    bool ok = false;
    switch (p.type) {
    case CONE_CNA: ok = ComputeCNA(p, g); break;
    case CONE_CSA: ok = ComputeCSA(p, g); break;
    case CONE_CHD: ok = ComputeCHD(p, g); break;
    case CONE_CDA: ok = ComputeCDA(p, g); break;
    default: g.errorMsg = _T("未知的锥体类型"); return false;
    }

    g.valid = ok;
    return ok;
}
