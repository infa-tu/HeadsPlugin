//-----------------------------------------------------------------------------
// ConeDlg.h : 锥体参数输入对话框
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "ConeGeometry.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// 锥体参数输入对话框（根据锥体类型启用/禁用相应输入项）
//-----------------------------------------------------------------------------
class CConeDlg : public CDialog {
public:
    CConeDlg(ConeType type, CWnd* pParent = nullptr);

    ConeParams m_params;     // 参数（输入/输出）
    BOOL       m_autoAdjust; // 是否勾选"自动调整"

    enum { IDD = IDD_CONE_DLG };

protected:
    ConeType m_type;

    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
};
