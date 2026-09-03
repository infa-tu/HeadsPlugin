//-----------------------------------------------------------------------------
// HeadDrawer.h : 绘图模块
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "DishedHead.h"
#include "EllipticalHead.h"
#include "ConeGeometry.h"

// 把实体追加到当前图形模型空间
Acad::ErrorStatus AppendToModelSpace(AcDbEntity* pEnt, AcDbObjectId& outId);

// 绘制碟形封头（轮廓 + 中心线 + 剖面线 + 尺寸标注）
//   g     几何计算结果
//   p     输入参数（用于标注文字）
//   base  插入点（中心线与直边底端交点，世界坐标）
Acad::ErrorStatus DrawDishedHead(const DishedHeadGeom& g,
                                 const DishedHeadParams& p,
                                 const AcGePoint3d& base);

// 绘制椭圆封头（整个剖面）
Acad::ErrorStatus DrawEllipticalHead(const EllipticalHeadGeom& g,
                                     const EllipticalHeadParams& p,
                                     const AcGePoint3d& base);

// 绘制锥体（整个剖面）
Acad::ErrorStatus DrawCone(const ConeGeom& g, const ConeParams& p, const AcGePoint3d& base);
