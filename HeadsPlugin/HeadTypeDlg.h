//-----------------------------------------------------------------------------
// HeadTypeDlg.h : 封头选型对话框
//-----------------------------------------------------------------------------
#pragma once
#include "StdAfx.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// 封头选型对话框（下拉框选择封头类型）
//-----------------------------------------------------------------------------
class CHeadTypeDlg : public CDialog {
public:
    CHeadTypeDlg(CWnd* pParent = nullptr);

    int m_type;   // 选中的封头类型：0=碟形封头, 1=1:2椭圆封头（以后扩展锥体）

    enum { IDD = IDD_HEAD_TYPE_DLG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
};
