//-----------------------------------------------------------------------------
// HeadDrawer.cpp : 绘图模块实现（整个剖面 / 全剖视图）
// 负责：创建图层、画轮廓实体(直线/圆弧)、画中心线、填充剖面线、标注尺寸
// 说明：几何计算只算右半边，绘图时把右半边镜像到左半边，拼成完整剖面
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "HeadDrawer.h"
#include "LayerManager.h"
#include <math.h>

//-----------------------------------------------------------------------------
// 图层名常量（按规格：轮廓白7 / 中心红1点划线 / 剖面绿3 / 尺寸青4）
//-----------------------------------------------------------------------------
static const TCHAR* LAYER_CONTOUR = _T("HEAD_CONTOUR");
static const TCHAR* LAYER_CENTER  = _T("HEAD_CENTER");
static const TCHAR* LAYER_HATCH   = _T("HEAD_HATCH");
static const TCHAR* LAYER_DIM     = _T("HEAD_DIM");

//-----------------------------------------------------------------------------
// 2D 点 + 插入点偏移 -> 3D 点
//-----------------------------------------------------------------------------
static AcGePoint3d P3(const AcGePoint2d& p, const AcGePoint3d& base) {
    return AcGePoint3d(base.x + p.x, base.y + p.y, 0.0);
}

//-----------------------------------------------------------------------------
// 数值格式化：保留到小数点后三位，并去掉末尾多余的 0
//-----------------------------------------------------------------------------
static CString FormatValue(double v) {
    CString s;
    s.Format(_T("%.3f"), v);
    while (!s.IsEmpty() && s[s.GetLength() - 1] == _T('0'))
        s = s.Left(s.GetLength() - 1);
    if (!s.IsEmpty() && s[s.GetLength() - 1] == _T('.'))
        s = s.Left(s.GetLength() - 1);
    return s;
}

//-----------------------------------------------------------------------------
// 把实体追加到模型空间
//-----------------------------------------------------------------------------
Acad::ErrorStatus AppendToModelSpace(AcDbEntity* pEnt, AcDbObjectId& outId) {
    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();

    AcDbBlockTable* pBt = nullptr;
    Acad::ErrorStatus es = pDb->getBlockTable(pBt, AcDb::kForRead);
    if (es != Acad::eOk) { delete pEnt; return es; }

    AcDbBlockTableRecord* pSpace = nullptr;
    es = pBt->getAt(ACDB_MODEL_SPACE, pSpace, AcDb::kForWrite);
    pBt->close();
    if (es != Acad::eOk) { delete pEnt; return es; }

    es = pSpace->appendAcDbEntity(outId, pEnt);
    pSpace->close();
    if (es != Acad::eOk) { delete pEnt; return es; }

    pEnt->close();
    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 画实心三角箭头（尖端在 tip，方向沿 from -> tip）
//-----------------------------------------------------------------------------
static void DrawArrowHead(const AcGePoint3d& tip, const AcGePoint3d& from, double size) {
    AcGeVector3d dir = tip - from;
    if (dir.length() < 1e-9) dir = AcGeVector3d(1.0, 0.0, 0.0);
    dir.normalize();
    AcGeVector3d perp(-dir.y, dir.x, 0.0);   // XY 平面内垂直于 dir
    AcGePoint3d b1 = tip - dir * size + perp * (size * 0.30);
    AcGePoint3d b2 = tip - dir * size - perp * (size * 0.30);
    AcDbSolid* pSolid = new AcDbSolid();
    pSolid->setPointAt(0, tip);
    pSolid->setPointAt(1, b1);
    pSolid->setPointAt(2, b2);
    pSolid->setPointAt(3, b2);
    pSolid->setLayer(LAYER_DIM);
    AcDbObjectId id;
    AppendToModelSpace(pSolid, id);
}

//-----------------------------------------------------------------------------
// 画文字（MText，左中对齐）
//-----------------------------------------------------------------------------
static void DrawMText(const AcGePoint3d& pos, const CString& txt, const TCHAR* layer) {
    AcDbMText* pMt = new AcDbMText();
    pMt->setLocation(pos);
    pMt->setContents((LPCTSTR)txt);
    pMt->setTextHeight(2.5);
    pMt->setAttachment(AcDbMText::kMiddleLeft);
    pMt->setLayer(layer);
    AcDbObjectId id;
    AppendToModelSpace(pMt, id);
}

//-----------------------------------------------------------------------------
// 径向标注：含箭头的直线指向圆弧突出部分（无尺寸界线）
//-----------------------------------------------------------------------------
static void DrawRadiusLeader(const AcGePoint2d& center, const AcGePoint2d& arcPt,
                             const CString& txt, const AcGePoint3d& base) {
    AcGeVector2d radial(arcPt.x - center.x, arcPt.y - center.y);
    if (radial.length() < 1e-9) radial = AcGeVector2d(1.0, 0.0);
    radial.normalize();
    double lead = 20.0;

    AcGePoint3d tip = P3(arcPt, base);
    AcGePoint3d anchor(base.x + arcPt.x + radial.x * lead,
                       base.y + arcPt.y + radial.y * lead, 0.0);

    AcDbLine* pLine = new AcDbLine(anchor, tip);
    pLine->setLayer(LAYER_DIM);
    AcDbObjectId id;
    AppendToModelSpace(pLine, id);

    DrawArrowHead(tip, anchor, 3.0);
    DrawMText(anchor, txt, LAYER_DIM);
}

//-----------------------------------------------------------------------------
// 画一条直线段（mirror=true 时画其关于中心线 x=0 的镜像）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawLine(const LineData& seg, const TCHAR* layer,
                                  const AcGePoint3d& base, bool mirror = false) {
    AcGePoint2d s = seg.start, e = seg.end;
    if (mirror) { s.x = -s.x; e.x = -e.x; }

    AcDbLine* pLine = new AcDbLine(P3(s, base), P3(e, base));
    pLine->setLayer(layer);
    AcDbObjectId id;
    return AppendToModelSpace(pLine, id);
}

//-----------------------------------------------------------------------------
// 画一段圆弧（mirror=true 时画其关于中心线 x=0 的镜像）
// 镜像规则：圆心 x 取负；起始角/终止角变为 (π-终止角, π-起始角)
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawArc(const ArcData& arc, const TCHAR* layer,
                                 const AcGePoint3d& base, bool mirror = false) {
    double cx = arc.center.x;
    double sa = arc.startAngle;
    double ea = arc.endAngle;
    if (mirror) {
        cx = -cx;
        sa = kPi - arc.endAngle;
        ea = kPi - arc.startAngle;
    }

    AcDbArc* pArc = new AcDbArc(P3(AcGePoint2d(cx, arc.center.y), base),
                                AcGeVector3d::kZAxis, arc.radius, sa, ea);
    pArc->setLayer(layer);
    AcDbObjectId id;
    return AppendToModelSpace(pArc, id);
}

//-----------------------------------------------------------------------------
// 计算一段圆弧（圆心 c、半径 r，从 p1 到 p2 的短弧）的凸度
// 凸度 = tan(圆心角/4)，逆时针为正
//-----------------------------------------------------------------------------
static double Bulge(const AcGePoint2d& c, double /*r*/,
                    const AcGePoint2d& p1, const AcGePoint2d& p2) {
    double a1 = atan2(p1.y - c.y, p1.x - c.x);
    double a2 = atan2(p2.y - c.y, p2.x - c.x);
    double sweep = a2 - a1;
    while (sweep >  kPi) sweep -= 2.0 * kPi;
    while (sweep < -kPi) sweep += 2.0 * kPi;
    return tan(sweep / 4.0);
}

//-----------------------------------------------------------------------------
// 填充剖面线（整个壁厚区域：一个 U 形闭合环，绕左外->外顶->右外->右内->内顶->左内）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawHatch(const DishedHeadGeom& g, const AcGePoint3d& base) {
    double R   = g.bottomInner.x;           // 内半径 Di/2
    double T   = g.bottomOuter.x - R;       // 壁厚
    double kcx = g.innerKnuckle.center.x;   // 折边圆心 x（右半边）
    double kcy = g.innerKnuckle.center.y;   // 折边圆心 y（= L）
    double yc  = g.innerCrown.center.y;     // 球冠圆心 y
    double rkT = g.outerKnuckle.radius;     // 外折边半径 ri+T
    double rcT = g.outerCrown.radius;       // 外球冠半径 Ri+T
    double rk  = g.innerKnuckle.radius;     // 内折边半径 ri
    double rc  = g.innerCrown.radius;       // 内球冠半径 Ri

    // 14 个顶点，绕壁厚区一圈
    AcGePoint2d v[14];
    v[0]  = AcGePoint2d(-R, 0.0);                          // 左内底面
    v[1]  = AcGePoint2d(-(R + T), 0.0);                    // 左外底面
    v[2]  = AcGePoint2d(-(R + T), g.outerFlange.end.y);    // 左外直边顶
    v[3]  = AcGePoint2d(-g.tangencyOuter.x, g.tangencyOuter.y); // 左外切点
    v[4]  = g.outerApex;                                   // 外顶点
    v[5]  = g.tangencyOuter;                               // 右外切点
    v[6]  = AcGePoint2d(R + T, g.outerFlange.end.y);       // 右外直边顶
    v[7]  = AcGePoint2d(R + T, 0.0);                       // 右外底面
    v[8]  = AcGePoint2d(R, 0.0);                           // 右内底面
    v[9]  = AcGePoint2d(R, g.innerFlange.end.y);           // 右内直边顶
    v[10] = g.tangencyInner;                               // 右内切点
    v[11] = g.innerApex;                                   // 内顶点
    v[12] = AcGePoint2d(-g.tangencyInner.x, g.tangencyInner.y); // 左内切点
    v[13] = AcGePoint2d(-R, g.innerFlange.end.y);          // 左内直边顶

    // 各边凸度（直线为 0）
    double b[14];
    b[0]  = 0.0;
    b[1]  = 0.0;
    b[2]  = Bulge(AcGePoint2d(-kcx, kcy), rkT, v[2], v[3]);
    b[3]  = Bulge(AcGePoint2d(0.0, yc), rcT, v[3], v[4]);
    b[4]  = Bulge(AcGePoint2d(0.0, yc), rcT, v[4], v[5]);
    b[5]  = Bulge(AcGePoint2d(kcx, kcy),  rkT, v[5], v[6]);
    b[6]  = 0.0;
    b[7]  = 0.0;
    b[8]  = 0.0;
    b[9]  = Bulge(AcGePoint2d(kcx, kcy),  rk, v[9], v[10]);
    b[10] = Bulge(AcGePoint2d(0.0, yc), rc, v[10], v[11]);
    b[11] = Bulge(AcGePoint2d(0.0, yc), rc, v[11], v[12]);
    b[12] = Bulge(AcGePoint2d(-kcx, kcy), rk, v[12], v[13]);
    b[13] = 0.0;

    // 1) 构建闭合多段线作为剖面线边界（隐藏，仅用于关联填充）
    AcDbPolyline* pPoly = new AcDbPolyline(14);
    for (int i = 0; i < 14; ++i)
        pPoly->addVertexAt(i, AcGePoint2d(v[i].x + base.x, v[i].y + base.y), b[i], 0.0, 0.0);
    pPoly->setClosed(true);
    pPoly->setVisibility(AcDb::kInvisible);   // 隐藏边界线
    pPoly->setLayer(LAYER_HATCH);

    AcDbObjectId polyId;
    Acad::ErrorStatus es = AppendToModelSpace(pPoly, polyId);
    if (es != Acad::eOk) return es;

    // 2) 创建关联剖面线，关联到上面的闭合边界
    AcDbHatch* pHatch = new AcDbHatch();
    pHatch->setAssociative(true);
    pHatch->setNormal(AcGeVector3d::kZAxis);
    pHatch->setLayer(LAYER_HATCH);

    // 剖面线比例随封头尺寸自动调整
    double scale = (g.totalHeight > 0.0) ? (g.totalHeight / 40.0) : 1.0;
    if (scale < 1.0) scale = 1.0;
    pHatch->setPatternScale(scale);
    pHatch->setPatternAngle(0.0);

    AcDbObjectIdArray ids;
    ids.append(polyId);
    es = pHatch->appendLoop(AcDbHatch::kExternal, ids);
    if (es != Acad::eOk) { delete pHatch; return es; }

    // 图案名最后设置，确保比例/角度生效
    pHatch->setPattern(AcDbHatch::kPreDefined, _T("ANSI31"));

    es = pHatch->evaluateHatch();
    if (es != Acad::eOk) { delete pHatch; return es; }

    AcDbObjectId id;
    return AppendToModelSpace(pHatch, id);
}

//-----------------------------------------------------------------------------
// 画中心线（红色 CENTER 点划线，上下各延伸一段）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawCenterLine(double totalHeight, const AcGePoint3d& base) {
    double ext = 15.0;   // 中心线超出量(mm)
    AcDbLine* pLine = new AcDbLine(
        AcGePoint3d(base.x, base.y - ext, 0.0),
        AcGePoint3d(base.x, base.y + totalHeight + ext, 0.0));
    pLine->setLayer(LAYER_CENTER);
    AcDbObjectId id;
    return AppendToModelSpace(pLine, id);
}

//-----------------------------------------------------------------------------
// 标注尺寸（ØDi 全直径；T/L/Ri/ri 标在右侧）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawDimensions(const DishedHeadGeom& g,
                                        const DishedHeadParams& p,
                                        const AcGePoint3d& base) {
    double R = p.Di / 2.0;
    AcDbObjectId id;
    CString txt;

    // 1) 内径 ØDi（全直径标注：左内壁 -> 右内壁）
    txt.Format(_T("\u00D8Di = %s"), (LPCTSTR)FormatValue(p.Di));
    AcDbRotatedDimension* pDimDi = new AcDbRotatedDimension(
        0.0,
        P3(AcGePoint2d(-R, 0.0), base),
        P3(AcGePoint2d(R, 0.0), base),
        P3(AcGePoint2d(0.0, -40.0), base),
        txt, AcDbObjectId::kNull);
    pDimDi->setLayer(LAYER_DIM);
    AppendToModelSpace(pDimDi, id);

    // 2) 壁厚 T（右侧水平标注：内直边 -> 外直边）
    txt.Format(_T("T = %s"), (LPCTSTR)FormatValue(p.T));
    AcDbRotatedDimension* pDimT = new AcDbRotatedDimension(
        0.0,
        P3(AcGePoint2d(R, 0.0), base),
        P3(AcGePoint2d(R + p.T, 0.0), base),
        P3(AcGePoint2d(R + p.T / 2.0, -15.0), base),
        txt, AcDbObjectId::kNull);
    pDimT->setLayer(LAYER_DIM);
    AppendToModelSpace(pDimT, id);

    // 3) 直边高度 L（右侧垂直标注）
    txt.Format(_T("L = %s"), (LPCTSTR)FormatValue(p.L));
    AcDbRotatedDimension* pDimL = new AcDbRotatedDimension(
        kPi / 2.0,
        P3(AcGePoint2d(R + p.T, 0.0), base),
        P3(AcGePoint2d(R + p.T, p.L), base),
        P3(AcGePoint2d(R + p.T + 35.0, p.L / 2.0), base),
        txt, AcDbObjectId::kNull);
    pDimL->setLayer(LAYER_DIM);
    AppendToModelSpace(pDimL, id);

    // 4) H（最高内壁点到底部，左侧）
    double H = g.innerApex.y;
    txt.Format(_T("H = %s"), (LPCTSTR)FormatValue(H));
    AcDbRotatedDimension* pDimH = new AcDbRotatedDimension(
        kPi / 2.0,
        P3(AcGePoint2d(-(R + p.T), 0.0), base),
        P3(AcGePoint2d(0.0, H), base),
        P3(AcGePoint2d(-(R + p.T + 65.0), H / 2.0), base),
        txt, AcDbObjectId::kNull);
    pDimH->setLayer(LAYER_DIM);
    AppendToModelSpace(pDimH, id);

    // 5) 球冠半径 RRi（箭头直线指向突出部分，无尺寸界线）
    double midCrown = (g.innerCrown.startAngle + kPi / 2.0) / 2.0;
    AcGePoint2d crownPt(g.innerCrown.center.x + g.innerCrown.radius * cos(midCrown),
                        g.innerCrown.center.y + g.innerCrown.radius * sin(midCrown));
    txt.Format(_T("Ri = %s"), (LPCTSTR)FormatValue(p.Ri));
    DrawRadiusLeader(g.innerCrown.center, crownPt, txt, base);

    // 6) 过渡半径 Rri（箭头直线指向突出部分，无尺寸界线）
    double midKn = g.innerKnuckle.endAngle / 2.0;
    AcGePoint2d knPt(g.innerKnuckle.center.x + g.innerKnuckle.radius * cos(midKn),
                     g.innerKnuckle.center.y + g.innerKnuckle.radius * sin(midKn));
    txt.Format(_T("ri = %s"), (LPCTSTR)FormatValue(p.ri));
    DrawRadiusLeader(g.innerKnuckle.center, knPt, txt, base);

    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 绘制碟形封头主函数（整个剖面）
//-----------------------------------------------------------------------------
Acad::ErrorStatus DrawDishedHead(const DishedHeadGeom& g,
                                 const DishedHeadParams& p,
                                 const AcGePoint3d& base) {
    // 1) 准备图层
    EnsureLayer(LAYER_CONTOUR, 7, _T("Continuous"), AcDb::kLnWt050);
    EnsureLayer(LAYER_CENTER,  1, _T("CENTER"));
    EnsureLayer(LAYER_HATCH,   3, _T("Continuous"));
    EnsureLayer(LAYER_DIM,     4, _T("Continuous"));

    // 2) 画外轮廓（右半边 + 左半边镜像）
    DrawLine(g.outerFlange,  LAYER_CONTOUR, base, false);
    DrawLine(g.outerFlange,  LAYER_CONTOUR, base, true);
    DrawArc (g.outerKnuckle, LAYER_CONTOUR, base, false);
    DrawArc (g.outerKnuckle, LAYER_CONTOUR, base, true);
    DrawArc (g.outerCrown,   LAYER_CONTOUR, base, false);
    DrawArc (g.outerCrown,   LAYER_CONTOUR, base, true);

    // 3) 画内轮廓（右半边 + 左半边镜像）
    DrawLine(g.innerFlange,  LAYER_CONTOUR, base, false);
    DrawLine(g.innerFlange,  LAYER_CONTOUR, base, true);
    DrawArc (g.innerKnuckle, LAYER_CONTOUR, base, false);
    DrawArc (g.innerKnuckle, LAYER_CONTOUR, base, true);
    DrawArc (g.innerCrown,   LAYER_CONTOUR, base, false);
    DrawArc (g.innerCrown,   LAYER_CONTOUR, base, true);

    // 4) 底端面（右半边 + 左半边镜像）
    LineData bottomFace = { g.bottomInner, g.bottomOuter };
    DrawLine(bottomFace, LAYER_CONTOUR, base, false);
    DrawLine(bottomFace, LAYER_CONTOUR, base, true);

    // 5) 中心线
    DrawCenterLine(g.totalHeight, base);

    // 6) 剖面线（整个 U 形壁厚区）
    DrawHatch(g, base);

    // 7) 尺寸标注
    DrawDimensions(g, p, base);

    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 画上半椭圆（AcDbEllipse：从右赤道经顶点到左赤道）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawHalfEllipse(const AcGePoint3d& center,
                                         double semiMajor, double semiMinor,
                                         const TCHAR* layer) {
    AcDbEllipse* pEll = new AcDbEllipse(center, AcGeVector3d::kZAxis,
                                        AcGeVector3d(semiMajor, 0.0, 0.0),
                                        semiMinor / semiMajor,
                                        0.0, kPi);
    pEll->setLayer(layer);
    AcDbObjectId id;
    return AppendToModelSpace(pEll, id);
}

//-----------------------------------------------------------------------------
// 椭圆封头剖面线：用多段线逼近椭圆边界（64 段/半椭圆），关联填充
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawEllipticalHatch(const EllipticalHeadGeom& g,
                                             double T, double L,
                                             const AcGePoint3d& base) {
    const int N = 64;                 // 半椭圆分段数
    const int nVerts = 2 * N + 6;     // 顶点总数
    double a = g.a, b = g.b;

    AcDbPolyline* pPoly = new AcDbPolyline(nVerts);
    int idx = 0;

    // 左内底面 -> 左外底面 -> 左外直边顶
    pPoly->addVertexAt(idx++, AcGePoint2d(-a + base.x, 0.0 + base.y));
    pPoly->addVertexAt(idx++, AcGePoint2d(-(a + T) + base.x, 0.0 + base.y));
    pPoly->addVertexAt(idx++, AcGePoint2d(-(a + T) + base.x, L + base.y));

    // 外椭圆上半（左赤道 -> 顶点 -> 右赤道）
    for (int i = 1; i < N; ++i) {
        double th = kPi - kPi * i / N;
        pPoly->addVertexAt(idx++, AcGePoint2d((a + T) * cos(th) + base.x,
                                              L + (b + T) * sin(th) + base.y));
    }

    // 右外直边顶 -> 右外底面 -> 右内底面 -> 右内直边顶
    pPoly->addVertexAt(idx++, AcGePoint2d(a + T + base.x, L + base.y));
    pPoly->addVertexAt(idx++, AcGePoint2d(a + T + base.x, 0.0 + base.y));
    pPoly->addVertexAt(idx++, AcGePoint2d(a + base.x, 0.0 + base.y));
    pPoly->addVertexAt(idx++, AcGePoint2d(a + base.x, L + base.y));

    // 内椭圆上半（右赤道 -> 顶点 -> 左赤道）
    for (int i = 1; i < N; ++i) {
        double th = kPi * i / N;
        pPoly->addVertexAt(idx++, AcGePoint2d(a * cos(th) + base.x,
                                              L + b * sin(th) + base.y));
    }

    // 左内直边顶（闭合）
    pPoly->addVertexAt(idx++, AcGePoint2d(-a + base.x, L + base.y));

    pPoly->setClosed(true);
    pPoly->setVisibility(AcDb::kInvisible);   // 隐藏边界
    pPoly->setLayer(LAYER_HATCH);

    AcDbObjectId polyId;
    Acad::ErrorStatus es = AppendToModelSpace(pPoly, polyId);
    if (es != Acad::eOk) return es;

    // 关联填充
    AcDbHatch* pHatch = new AcDbHatch();
    pHatch->setAssociative(true);
    pHatch->setNormal(AcGeVector3d::kZAxis);
    pHatch->setLayer(LAYER_HATCH);

    double scale = (g.totalHeight > 0.0) ? (g.totalHeight / 40.0) : 1.0;
    if (scale < 1.0) scale = 1.0;
    pHatch->setPatternScale(scale);
    pHatch->setPatternAngle(0.0);

    AcDbObjectIdArray ids;
    ids.append(polyId);
    es = pHatch->appendLoop(AcDbHatch::kExternal, ids);
    if (es != Acad::eOk) { delete pHatch; return es; }

    pHatch->setPattern(AcDbHatch::kPreDefined, _T("ANSI31"));
    es = pHatch->evaluateHatch();
    if (es != Acad::eOk) { delete pHatch; return es; }

    AcDbObjectId id;
    return AppendToModelSpace(pHatch, id);
}

//-----------------------------------------------------------------------------
// 椭圆封头尺寸标注
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawEllipticalDimensions(const EllipticalHeadGeom& g,
                                                  const EllipticalHeadParams& p,
                                                  const AcGePoint3d& base) {
    double a = g.a, T = p.T, L = p.L;
    AcDbObjectId id;
    CString txt;

    // 1) 内径 ØDi（全直径）
    txt.Format(_T("\u00D8Di = %s"), (LPCTSTR)FormatValue(p.Di));
    AcDbRotatedDimension* d1 = new AcDbRotatedDimension(
        0.0, P3(AcGePoint2d(-a, 0.0), base), P3(AcGePoint2d(a, 0.0), base),
        P3(AcGePoint2d(0.0, -40.0), base), txt, AcDbObjectId::kNull);
    d1->setLayer(LAYER_DIM);
    AppendToModelSpace(d1, id);

    // 2) 壁厚 T
    txt.Format(_T("T = %s"), (LPCTSTR)FormatValue(T));
    AcDbRotatedDimension* d2 = new AcDbRotatedDimension(
        0.0, P3(AcGePoint2d(a, 0.0), base), P3(AcGePoint2d(a + T, 0.0), base),
        P3(AcGePoint2d(a + T / 2.0, -15.0), base), txt, AcDbObjectId::kNull);
    d2->setLayer(LAYER_DIM);
    AppendToModelSpace(d2, id);

    // 3) 直边高度 L
    txt.Format(_T("L = %s"), (LPCTSTR)FormatValue(L));
    AcDbRotatedDimension* d3 = new AcDbRotatedDimension(
        kPi / 2.0, P3(AcGePoint2d(a + T, 0.0), base), P3(AcGePoint2d(a + T, L), base),
        P3(AcGePoint2d(a + T + 35.0, L / 2.0), base), txt, AcDbObjectId::kNull);
    d3->setLayer(LAYER_DIM);
    AppendToModelSpace(d3, id);

    // 4) H（最高内壁点到底部，左侧）
    double H = L + g.b;
    txt.Format(_T("H = %s"), (LPCTSTR)FormatValue(H));
    AcDbRotatedDimension* d4 = new AcDbRotatedDimension(
        kPi / 2.0,
        P3(AcGePoint2d(-(a + T), 0.0), base),
        P3(AcGePoint2d(0.0, H), base),
        P3(AcGePoint2d(-(a + T + 65.0), H / 2.0), base),
        txt, AcDbObjectId::kNull);
    d4->setLayer(LAYER_DIM);
    AppendToModelSpace(d4, id);

    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 绘制椭圆封头主函数（整个剖面）
//-----------------------------------------------------------------------------
Acad::ErrorStatus DrawEllipticalHead(const EllipticalHeadGeom& g,
                                     const EllipticalHeadParams& p,
                                     const AcGePoint3d& base) {
    double a = g.a, b = g.b, T = p.T, L = p.L;

    // 1) 准备图层
    EnsureLayer(LAYER_CONTOUR, 7, _T("Continuous"), AcDb::kLnWt050);
    EnsureLayer(LAYER_CENTER,  1, _T("CENTER"));
    EnsureLayer(LAYER_HATCH,   3, _T("Continuous"));
    EnsureLayer(LAYER_DIM,     4, _T("Continuous"));

    // 2) 内外椭圆（上半，左右对称）
    DrawHalfEllipse(AcGePoint3d(base.x, base.y + L, 0.0), a + T, b + T, LAYER_CONTOUR);
    DrawHalfEllipse(AcGePoint3d(base.x, base.y + L, 0.0), a, b, LAYER_CONTOUR);

    // 3) 内外直边（右 + 左镜像）
    LineData innerFlange = { AcGePoint2d(a, 0.0), AcGePoint2d(a, L) };
    LineData outerFlange = { AcGePoint2d(a + T, 0.0), AcGePoint2d(a + T, L) };
    DrawLine(innerFlange, LAYER_CONTOUR, base, false);
    DrawLine(innerFlange, LAYER_CONTOUR, base, true);
    DrawLine(outerFlange, LAYER_CONTOUR, base, false);
    DrawLine(outerFlange, LAYER_CONTOUR, base, true);

    // 4) 底端面（右 + 左镜像）
    LineData bottomFace = { g.bottomInner, g.bottomOuter };
    DrawLine(bottomFace, LAYER_CONTOUR, base, false);
    DrawLine(bottomFace, LAYER_CONTOUR, base, true);

    // 5) 中心线
    DrawCenterLine(g.totalHeight, base);

    // 6) 剖面线
    DrawEllipticalHatch(g, T, L, base);

    // 7) 尺寸标注
    DrawEllipticalDimensions(g, p, base);

    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 画轮廓（多段线 + 可选镜像）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawProfile(const ProfileData& pr, const TCHAR* layer,
                                     const AcGePoint3d& base, bool mirror) {
    int n = pr.pts.length();
    AcDbPolyline* pPoly = new AcDbPolyline(n);
    for (int i = 0; i < n; ++i) {
        double x = mirror ? -pr.pts[i].x : pr.pts[i].x;
        double b = (i < n - 1) ? pr.bulges[i] : 0.0;
        if (mirror) b = -b;
        pPoly->addVertexAt(i, AcGePoint2d(x + base.x, pr.pts[i].y + base.y), b, 0.0, 0.0);
    }
    pPoly->setLayer(layer);
    AcDbObjectId id;
    return AppendToModelSpace(pPoly, id);
}

//-----------------------------------------------------------------------------
// 构建锥体一侧壁厚区闭合边界（隐藏多段线，用于关联填充）
// 顺序：内轮廓(底→顶) -> 顶面 -> 外轮廓(顶→底反向) -> 底面闭合
//-----------------------------------------------------------------------------
static Acad::ErrorStatus BuildStripPolyline(const ConeGeom& g, const AcGePoint3d& base,
                                            bool mirror, AcDbObjectId& outId) {
    int m = g.innerRight.pts.length();
    int n = g.outerRight.pts.length();

    AcGePoint2dArray loop;
    AcGeDoubleArray bulges;

    auto tr = [&](const AcGePoint2d& p) { return AcGePoint2d(mirror ? -p.x : p.x, p.y); };
    auto tb = [&](double b) { return mirror ? -b : b; };

    // 内轮廓(底→顶)
    loop.append(tr(g.innerRight.pts[0]));
    for (int i = 1; i < m; ++i) {
        loop.append(tr(g.innerRight.pts[i]));
        bulges.append(tb(g.innerRight.bulges[i - 1]));
    }
    // 顶面 innerTop -> outerTop
    loop.append(tr(g.outerRight.pts[n - 1]));
    bulges.append(0.0);
    // 外轮廓(顶→底反向)
    for (int i = n - 2; i >= 0; --i) {
        loop.append(tr(g.outerRight.pts[i]));
        bulges.append(tb(-g.outerRight.bulges[i]));
    }
    // 底面闭合段（直线）
    bulges.append(0.0);

    int cnt = loop.length();
    AcDbPolyline* pPoly = new AcDbPolyline(cnt);
    for (int i = 0; i < cnt; ++i) {
        pPoly->addVertexAt(i, AcGePoint2d(loop[i].x + base.x, loop[i].y + base.y), bulges[i], 0.0, 0.0);
    }
    pPoly->setClosed(true);
    pPoly->setVisibility(AcDb::kInvisible);
    pPoly->setLayer(LAYER_HATCH);
    return AppendToModelSpace(pPoly, outId);
}

//-----------------------------------------------------------------------------
// 锥体剖面线（左右两个壁厚区，关联填充）
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawConeHatch(const ConeGeom& g, const AcGePoint3d& base) {
    // 左右两个壁厚区分别创建关联填充（各一个闭合边界）
    for (int side = 0; side < 2; ++side) {
        bool mirror = (side == 1);
        AcDbObjectId polyId;
        Acad::ErrorStatus es = BuildStripPolyline(g, base, mirror, polyId);
        if (es != Acad::eOk) return es;

        AcDbHatch* pHatch = new AcDbHatch();
        pHatch->setAssociative(true);
        pHatch->setNormal(AcGeVector3d::kZAxis);
        pHatch->setLayer(LAYER_HATCH);

        double scale = (g.totalHeight > 0.0) ? (g.totalHeight / 40.0) : 1.0;
        if (scale < 1.0) scale = 1.0;
        pHatch->setPatternScale(scale);
        pHatch->setPatternAngle(0.0);

        AcDbObjectIdArray ids;
        ids.append(polyId);
        es = pHatch->appendLoop(AcDbHatch::kExternal, ids);
        if (es != Acad::eOk) { delete pHatch; return es; }

        pHatch->setPattern(AcDbHatch::kPreDefined, _T("ANSI31"));
        es = pHatch->evaluateHatch();
        if (es != Acad::eOk) { delete pHatch; return es; }

        AcDbObjectId id;
        es = AppendToModelSpace(pHatch, id);
        if (es != Acad::eOk) return es;
    }
    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 锥体尺寸标注
//-----------------------------------------------------------------------------
static Acad::ErrorStatus DrawConeDimensions(const ConeGeom& g, const ConeParams& p,
                                            const AcGePoint3d& base) {
    double Rtop = p.Di / 2.0, Rbot = p.Dis / 2.0;
    double H = g.totalHeight;
    double a = p.alphaDeg * kPi / 180.0;
    double ca = cos(a), sa = sin(a), ta = tan(a);
    AcDbObjectId id;
    CString txt;

    // 1) ØDi（大端内径）
    txt.Format(_T("\u00D8Di = %s"), (LPCTSTR)FormatValue(p.Di));
    AcDbRotatedDimension* dD = new AcDbRotatedDimension(
        0.0, P3(AcGePoint2d(-Rtop, H), base), P3(AcGePoint2d(Rtop, H), base),
        P3(AcGePoint2d(0.0, H + 40.0), base), txt, AcDbObjectId::kNull);
    dD->setLayer(LAYER_DIM); AppendToModelSpace(dD, id);

    // 2) ØDis（小端内径）
    txt.Format(_T("\u00D8Dis = %s"), (LPCTSTR)FormatValue(p.Dis));
    AcDbRotatedDimension* dDs = new AcDbRotatedDimension(
        0.0, P3(AcGePoint2d(-Rbot, 0.0), base), P3(AcGePoint2d(Rbot, 0.0), base),
        P3(AcGePoint2d(0.0, -40.0), base), txt, AcDbObjectId::kNull);
    dDs->setLayer(LAYER_DIM); AppendToModelSpace(dDs, id);

    // 3) δn（壁厚，底部右侧，沿端面法向测量）
    txt.Format(_T("\u03B4n = %s"), (LPCTSTR)FormatValue(p.dn));
    {
        AcGePoint2d pb1 = g.innerBottom, pb2 = g.outerBottom;
        double rot = atan2(pb2.y - pb1.y, pb2.x - pb1.x);
        double fx = pb2.x - pb1.x, fy = pb2.y - pb1.y;
        double flen = sqrt(fx * fx + fy * fy);
        double nx = fy / flen, ny = -fx / flen;   // 端面法线（朝下/外侧）
        AcGePoint2d mid((pb1.x + pb2.x) / 2.0, (pb1.y + pb2.y) / 2.0);
        AcGePoint2d dPt(mid.x + 15.0 * nx, mid.y + 15.0 * ny);
        AcDbRotatedDimension* dDn = new AcDbRotatedDimension(
            rot, P3(pb1, base), P3(pb2, base), P3(dPt, base), txt, AcDbObjectId::kNull);
        dDn->setLayer(LAYER_DIM); AppendToModelSpace(dDn, id);
    }

    // 4) hs（底部直边高，仅 CHD/CDA 标注）
    if (g.hs > 1e-6) {
        txt.Format(_T("hs = %s"), (LPCTSTR)FormatValue(g.hs));
        AcDbRotatedDimension* dHs = new AcDbRotatedDimension(
            kPi / 2.0, P3(AcGePoint2d(Rbot, 0.0), base), P3(AcGePoint2d(Rbot, g.hs), base),
            P3(AcGePoint2d(Rbot + 65.0, g.hs / 2.0), base), txt, AcDbObjectId::kNull);
        dHs->setLayer(LAYER_DIM); AppendToModelSpace(dHs, id);
    }

    // 5) H0（总高，左侧）
    txt.Format(_T("H\u2080 = %s"), (LPCTSTR)FormatValue(H));
    AcDbRotatedDimension* dH = new AcDbRotatedDimension(
        kPi / 2.0, P3(AcGePoint2d(-Rbot, 0.0), base), P3(AcGePoint2d(-Rtop, H), base),
        P3(AcGePoint2d(-(Rtop + 65.0), H / 2.0), base), txt, AcDbObjectId::kNull);
    dH->setLayer(LAYER_DIM); AppendToModelSpace(dH, id);

    // 6) h（顶部直边/翻边高，CSA/CDA）
    if (p.type == CONE_CSA || p.type == CONE_CDA) {
        double yf = g.innerRight.pts[g.innerRight.pts.length() - 2].y;
        txt.Format(_T("h = %s"), (LPCTSTR)FormatValue(p.h));
        AcDbRotatedDimension* dHt = new AcDbRotatedDimension(
            kPi / 2.0, P3(AcGePoint2d(Rtop, yf), base), P3(AcGePoint2d(Rtop, H), base),
            P3(AcGePoint2d(Rtop + 65.0, (yf + H) / 2.0), base), txt, AcDbObjectId::kNull);
        dHt->setLayer(LAYER_DIM); AppendToModelSpace(dHt, id);
    }

    // 7) α（半顶角，角标注：一端中心线、一端内壁，弧线落在内部空白区）
    txt.Format(_T("\u03B1 = %s\u00B0"), (LPCTSTR)FormatValue(p.alphaDeg));
    {
        // 斜壁（内壁）起点
        double Tbx, Tby;
        if (p.type == CONE_CHD || p.type == CONE_CDA) {
            double rs = p.rs;
            Tbx = Rbot + rs - rs * ca;
            Tby = g.hs + rs * sa;
        } else {
            Tbx = Rbot;
            Tby = 0.0;   // CNA/CSA：无底直边
        }
        // 虚拟顶点（中心线与斜壁交点，在底部下方）
        double apexY = Tby - Tbx / ta;
        // 弧线半径 = 顶点到斜壁底角的距离，使弧线落在封头内部
        double Rarc = Tbx / sa;
        AcDb2LineAngularDimension* dA = new AcDb2LineAngularDimension(
            P3(AcGePoint2d(0.0, apexY), base), P3(AcGePoint2d(0.0, apexY + Rarc + 250.0), base),   // 中心线
            P3(AcGePoint2d(0.0, apexY), base), P3(AcGePoint2d(Tbx + 250.0 * sa, Tby + 250.0 * ca), base),   // 内壁
            P3(AcGePoint2d(Rarc * sin(a / 2.0), apexY + Rarc * cos(a / 2.0)), base),   // 弧点（内部）
            txt, AcDbObjectId::kNull);
        dA->setLayer(LAYER_DIM); AppendToModelSpace(dA, id);
    }

    // 8) 半径标注（Rri / Rrs，箭头直线指向突出部分，无尺寸界线）
    for (size_t i = 0; i < g.dimArcs.size(); ++i) {
        const ArcData& arc = g.dimArcs[i];
        double mid = (arc.startAngle + arc.endAngle) / 2.0;
        AcGePoint2d pt(arc.center.x + arc.radius * cos(mid),
                       arc.center.y + arc.radius * sin(mid));
        CString name;
        if (p.type == CONE_CSA)      name = _T("ri = ");
        else if (p.type == CONE_CHD) name = _T("rs = ");
        else                         name = (i == 0) ? _T("rs = ") : _T("ri = ");
        txt.Format(_T("%s%s"), (LPCTSTR)name, (LPCTSTR)FormatValue(arc.radius));
        DrawRadiusLeader(arc.center, pt, txt, base);
    }

    return Acad::eOk;
}

//-----------------------------------------------------------------------------
// 绘制锥体主函数（整个剖面）
//-----------------------------------------------------------------------------
Acad::ErrorStatus DrawCone(const ConeGeom& g, const ConeParams& p, const AcGePoint3d& base) {
    // 1) 准备图层
    EnsureLayer(LAYER_CONTOUR, 7, _T("Continuous"), AcDb::kLnWt050);
    EnsureLayer(LAYER_CENTER,  1, _T("CENTER"));
    EnsureLayer(LAYER_HATCH,   3, _T("Continuous"));
    EnsureLayer(LAYER_DIM,     4, _T("Continuous"));

    // 2) 外/内轮廓（右 + 左镜像）
    DrawProfile(g.outerRight, LAYER_CONTOUR, base, false);
    DrawProfile(g.outerRight, LAYER_CONTOUR, base, true);
    DrawProfile(g.innerRight, LAYER_CONTOUR, base, false);
    DrawProfile(g.innerRight, LAYER_CONTOUR, base, true);

    // 3) 顶面 + 底面（右 + 左镜像）
    LineData topFace = { g.innerTop, g.outerTop };
    LineData botFace = { g.innerBottom, g.outerBottom };
    DrawLine(topFace, LAYER_CONTOUR, base, false);
    DrawLine(topFace, LAYER_CONTOUR, base, true);
    DrawLine(botFace, LAYER_CONTOUR, base, false);
    DrawLine(botFace, LAYER_CONTOUR, base, true);

    // 4) 中心线
    DrawCenterLine(g.totalHeight, base);

    // 5) 剖面线（左右壁厚区）
    DrawConeHatch(g, base);

    // 6) 尺寸标注
    DrawConeDimensions(g, p, base);

    return Acad::eOk;
}
