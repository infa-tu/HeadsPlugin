//-----------------------------------------------------------------------------
// ConeDlg.cpp : 锥体参数输入对话框实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "ConeDlg.h"

//-----------------------------------------------------------------------------
// 构造函数
//-----------------------------------------------------------------------------
CConeDlg::CConeDlg(ConeType type, CWnd* pParent)
    : CDialog(IDD_CONE_DLG, pParent)
    , m_autoAdjust(FALSE)
    , m_type(type) {
    m_params.type = type;
}

//-----------------------------------------------------------------------------
// 对话框控件 <-> 成员变量 数据交换
//-----------------------------------------------------------------------------
void CConeDlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_EDIT_C_DI, m_params.Di);
    DDV_MinMaxDouble(pDX, m_params.Di, 1.0, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_DIS, m_params.Dis);
    DDV_MinMaxDouble(pDX, m_params.Dis, 1.0, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_ALPHA, m_params.alphaDeg);
    DDV_MinMaxDouble(pDX, m_params.alphaDeg, 0.1, 84.9);

    DDX_Text(pDX, IDC_EDIT_C_DN, m_params.dn);
    DDV_MinMaxDouble(pDX, m_params.dn, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_HS, m_params.hs);
    DDV_MinMaxDouble(pDX, m_params.hs, 0.0, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_H, m_params.h);
    DDV_MinMaxDouble(pDX, m_params.h, 0.0, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_RI, m_params.ri);
    DDV_MinMaxDouble(pDX, m_params.ri, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_C_RS, m_params.rs);
    DDV_MinMaxDouble(pDX, m_params.rs, 0.1, 1.0e9);

    DDX_Check(pDX, IDC_CHECK_C_AUTO, m_autoAdjust);
}

//-----------------------------------------------------------------------------
// 初始化：根据锥体类型启用/禁用输入项
//-----------------------------------------------------------------------------
BOOL CConeDlg::OnInitDialog() {
    CDialog::OnInitDialog();

    // 各类型需要的字段
    bool needH  = (m_type == CONE_CSA || m_type == CONE_CDA);
    bool needRi = (m_type == CONE_CSA || m_type == CONE_CDA);
    bool needRs = (m_type == CONE_CHD || m_type == CONE_CDA);
    bool needHs = (m_type == CONE_CHD || m_type == CONE_CDA);

    GetDlgItem(IDC_EDIT_C_H)->EnableWindow(needH);
    GetDlgItem(IDC_EDIT_C_RI)->EnableWindow(needRi);
    GetDlgItem(IDC_EDIT_C_RS)->EnableWindow(needRs);
    GetDlgItem(IDC_EDIT_C_HS)->EnableWindow(needHs);

    return TRUE;
}

//-----------------------------------------------------------------------------
// 点击"确定"
//-----------------------------------------------------------------------------
void CConeDlg::OnOK() {
    if (!UpdateData(TRUE))
        return;

    CString err;
    if (!ValidateCone(m_params, err)) {
        if (m_autoAdjust) {
            AutoAdjustCone(m_params);
            UpdateData(FALSE);
        } else {
            AfxMessageBox(err, MB_ICONWARNING);
            return;
        }
    }

    CDialog::OnOK();
}
