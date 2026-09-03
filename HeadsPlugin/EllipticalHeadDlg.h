//-----------------------------------------------------------------------------
// EllipticalHeadDlg.h : 椭圆封头参数输入对话框
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "EllipticalHead.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// 椭圆封头参数输入对话框
//-----------------------------------------------------------------------------
class CEllipticalHeadDlg : public CDialog {
public:
    CEllipticalHeadDlg(CWnd* pParent = nullptr);

    EllipticalHeadParams m_params;   // 参数（输入/输出）

    enum { IDD = IDD_ELLIPTICAL_HEAD_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
};
