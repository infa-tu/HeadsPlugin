// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"

#include "DishedHead.h"
#include "HeadDrawer.h"
#include "DishedHeadDlg.h"
#include "EllipticalHead.h"
#include "EllipticalHeadDlg.h"
#include "HeadTypeDlg.h"
#include "ConeGeometry.h"
#include "ConeDlg.h"

//-----------------------------------------------------------------------------
#define szRDS _RXST("asdk")

//-----------------------------------------------------------------------------
// 命令实现：DRAW_HEAD —— 主命令：选型 -> 参数 -> 出图
//-----------------------------------------------------------------------------
static void HeadsPluginDrawHead() {
    // 切换到本模块资源，保证 MFC 对话框能正确加载
    CAcModuleResourceOverride resOverride;

    // 1) 封头选型
    CHeadTypeDlg typeDlg(acedGetAcadFrame());
    if (typeDlg.DoModal() != IDOK)
        return;
    int t = typeDlg.m_type;

    // 2) 根据类型弹出参数对话框并计算几何
    DishedHeadParams    dishParams;
    DishedHeadGeom      dishGeom;
    EllipticalHeadParams ellParams;
    EllipticalHeadGeom  ellGeom;
    ConeParams          coneParams;
    ConeGeom            coneGeom;

    if (t == 0) {
        CDishedHeadDlg dlg(acedGetAcadFrame());
        if (dlg.DoModal() != IDOK)
            return;
        dishParams = dlg.m_params;
        if (!ComputeDishedHead(dishParams, dishGeom)) {
            acutPrintf(_T("\n几何计算失败: %s"), (LPCTSTR)dishGeom.errorMsg);
            return;
        }
    } else if (t == 1) {
        CEllipticalHeadDlg dlg(acedGetAcadFrame());
        if (dlg.DoModal() != IDOK)
            return;
        ellParams = dlg.m_params;
        if (!ComputeEllipticalHead(ellParams, ellGeom)) {
            acutPrintf(_T("\n几何计算失败: %s"), (LPCTSTR)ellGeom.errorMsg);
            return;
        }
    } else {
        // 锥体：下拉索引 2..5 -> ConeType 0..3
        CConeDlg dlg((ConeType)(t - 2), acedGetAcadFrame());
        if (dlg.DoModal() != IDOK)
            return;
        coneParams = dlg.m_params;
        if (!ComputeCone(coneParams, coneGeom)) {
            acutPrintf(_T("\n几何计算失败: %s"), (LPCTSTR)coneGeom.errorMsg);
            return;
        }
    }

    // 3) 提示用户拾取插入点（中心线与直边底端交点）
    ads_point pt;
    if (acedGetPoint(nullptr, _T("\n拾取插入点(中心线与直边底端交点): "), pt) != RTNORM) {
        acutPrintf(_T("\n已取消。"));
        return;
    }
    AcGePoint3d base(pt[X], pt[Y], 0.0);

    // 4) 绘制
    if (t == 0)
        DrawDishedHead(dishGeom, dishParams, base);
    else if (t == 1)
        DrawEllipticalHead(ellGeom, ellParams, base);
    else
        DrawCone(coneGeom, coneParams, base);

    acutPrintf(_T("\n封头绘制完成。"));
}

//-----------------------------------------------------------------------------
// 命令注册 / 注销
//-----------------------------------------------------------------------------
static void RegisterCommands() {
    acedRegCmds->addCommand(_T("HEADS_CMDS"),
                            _T("DRAW_HEAD"),
                            _T("DRAW_HEAD"),
                            ACRX_CMD_MODAL,
                            HeadsPluginDrawHead);
}

static void UnregisterCommands() {
    acedRegCmds->removeGroup(_T("HEADS_CMDS"));
}

//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class CHeadsPluginApp : public AcRxArxApp {

public:
	CHeadsPluginApp () : AcRxArxApp () {}

	virtual AcRx::AppRetCode On_kInitAppMsg (void *pkt) {
		// You *must* call On_kInitAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kInitAppMsg (pkt) ;

		// 注册本插件的命令
		RegisterCommands () ;

		return (retCode) ;
	}

	virtual AcRx::AppRetCode On_kUnloadAppMsg (void *pkt) {
		// 注销本插件的命令
		UnregisterCommands () ;

		// You *must* call On_kUnloadAppMsg here
		AcRx::AppRetCode retCode =AcRxArxApp::On_kUnloadAppMsg (pkt) ;

		return (retCode) ;
	}

	virtual void RegisterServerComponents () {
	}

} ;

//-----------------------------------------------------------------------------
IMPLEMENT_ARX_ENTRYPOINT(CHeadsPluginApp)

