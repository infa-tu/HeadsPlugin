//-----------------------------------------------------------------------------
// HeadTypeDlg.cpp : 封头选型对话框实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "HeadTypeDlg.h"

//-----------------------------------------------------------------------------
// 构造函数
//-----------------------------------------------------------------------------
CHeadTypeDlg::CHeadTypeDlg(CWnd* pParent)
    : CDialog(IDD_HEAD_TYPE_DLG, pParent)
    , m_type(0) {
}

//-----------------------------------------------------------------------------
// 对话框控件 <-> 成员变量 数据交换
//-----------------------------------------------------------------------------
void CHeadTypeDlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);
    DDX_CBIndex(pDX, IDC_COMBO_TYPE, m_type);
}

//-----------------------------------------------------------------------------
// 初始化：向下拉框添加封头类型
//-----------------------------------------------------------------------------
BOOL CHeadTypeDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    CComboBox* pCb = (CComboBox*)GetDlgItem(IDC_COMBO_TYPE);
    if (pCb) {
        pCb->AddString(_T("碟形封头 (Dish Head)"));
        pCb->AddString(_T("1:2 椭圆封头 (Elliptical Head)"));
        pCb->AddString(_T("CNA 无折边锥壳"));
        pCb->AddString(_T("CSA 带折边锥壳"));
        pCb->AddString(_T("CHD 简单锥形"));
        pCb->AddString(_T("CDA 带顶部圆角锥壳"));
        pCb->SetCurSel(0);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// 点击"确定"
//-----------------------------------------------------------------------------
void CHeadTypeDlg::OnOK() {
    UpdateData(TRUE);
    CDialog::OnOK();
}
