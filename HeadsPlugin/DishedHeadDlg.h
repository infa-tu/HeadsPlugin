//-----------------------------------------------------------------------------
// DishedHeadDlg.h : 碟形封头参数输入对话框
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "DishedHead.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// 碟形封头参数输入对话框
//-----------------------------------------------------------------------------
class CDishedHeadDlg : public CDialog {
public:
    CDishedHeadDlg(CWnd* pParent = nullptr);

    // 对话框数据
    DishedHeadParams m_params;      // 参数（输入/输出）
    BOOL             m_autoAdjust;  // 是否勾选"自动调整到相切"(DDX_Check 要求 int)

    enum { IDD = IDD_DISHED_HEAD_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
};
