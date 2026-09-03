//-----------------------------------------------------------------------------
// LayerManager.h : 图层管理
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"

// 确保图层存在(不存在则创建)，并设置颜色、线型与线宽
//   name         图层名
//   colorIndex   AutoCAD 颜色索引(ACI)
//   linetypeName 线型名，如 _T("Continuous")、_T("CENTER")
//   lw           线宽(AcDb::LineWeight)，默认 kLnWtDefault
Acad::ErrorStatus EnsureLayer(const TCHAR* name, int colorIndex, const TCHAR* linetypeName,
                              AcDb::LineWeight lw = AcDb::kLnWtByLwDefault);
