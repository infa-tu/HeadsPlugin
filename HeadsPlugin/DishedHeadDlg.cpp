//-----------------------------------------------------------------------------
// DishedHeadDlg.cpp : 碟形封头参数输入对话框实现
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "DishedHeadDlg.h"

//-----------------------------------------------------------------------------
// 构造函数
//-----------------------------------------------------------------------------
CDishedHeadDlg::CDishedHeadDlg(CWnd* pParent)
    : CDialog(IDD_DISHED_HEAD_DLG, pParent)
    , m_autoAdjust(FALSE) {
}

//-----------------------------------------------------------------------------
// 对话框控件 <-> 成员变量 数据交换
//-----------------------------------------------------------------------------
void CDishedHeadDlg::DoDataExchange(CDataExchange* pDX) {
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_EDIT_DI, m_params.Di);
    DDV_MinMaxDouble(pDX, m_params.Di, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_RI, m_params.Ri);
    DDV_MinMaxDouble(pDX, m_params.Ri, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_RI_TRANS, m_params.ri);
    DDV_MinMaxDouble(pDX, m_params.ri, 0.01, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_T, m_params.T);
    DDV_MinMaxDouble(pDX, m_params.T, 0.1, 1.0e9);

    DDX_Text(pDX, IDC_EDIT_L, m_params.L);
    DDV_MinMaxDouble(pDX, m_params.L, 0.0, 1.0e9);

    DDX_Check(pDX, IDC_CHECK_AUTO, m_autoAdjust);
}

//-----------------------------------------------------------------------------
// 初始化对话框
//-----------------------------------------------------------------------------
BOOL CDishedHeadDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    return TRUE;   // 返回 TRUE 表示把焦点设置到第一个控件
}

//-----------------------------------------------------------------------------
// 点击"确定"
//-----------------------------------------------------------------------------
void CDishedHeadDlg::OnOK() {
    // 从控件读取数据（DDV 校验失败会自动提示并停留）
    if (!UpdateData(TRUE))
        return;

    // 检查相切条件
    CString err;
    if (!ValidateDishedHead(m_params, err)) {
        if (m_autoAdjust) {
            // 勾选了自动调整：修正参数后回填控件
            AutoAdjustDishedHead(m_params);
            UpdateData(FALSE);
        } else {
            AfxMessageBox(err, MB_ICONWARNING);
            return;
        }
    }

    CDialog::OnOK();
}
