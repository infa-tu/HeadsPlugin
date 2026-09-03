//-----------------------------------------------------------------------------
// EllipticalHeadDlg.cpp : 椭圆封头参数输入对话框实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "EllipticalHeadDlg.h"

//-----------------------------------------------------------------------------
// 构造函数
//-----------------------------------------------------------------------------
CEllipticalHeadDlg::CEllipticalHeadDlg(CWnd* pParent)
    : CDialog(IDD_ELLIPTICAL_HEAD_DLG, pParent) {
}

//-----------------------------------------------------------------------------
// 对话框控件 <-> 成员变量 数据交换
//-----------------------------------------------------------------------------
void CEllipticalHeadDlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_EDIT_E_DI, m_params.Di);
    DDV_MinMaxDouble(pDX, m_params.Di, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_E_T, m_params.T);
    DDV_MinMaxDouble(pDX, m_params.T, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_E_L, m_params.L);
    DDV_MinMaxDouble(pDX, m_params.L, 0.0, 1.0e9);
}
