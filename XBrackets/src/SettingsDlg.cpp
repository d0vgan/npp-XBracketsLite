#include "SettingsDlg.h"
#include "resource.h"
#include "XBrackets.h"
#include "XBracketsOptions.h"


extern CXBrackets thePlugin;
extern CXBracketsOptions g_opt;


BOOL AnyWindow_CenterWindow(HWND hWnd, HWND hParentWnd, BOOL bRepaint);
bool CheckBox_IsChecked(HWND hDlg, UINT idCheckBox);
bool CheckBox_SetCheck(HWND hDlg, UINT idCheckBox, bool bChecked);
BOOL DlgItem_EnableWindow(HWND hDlg, UINT idDlgItem, bool bEnable);
BOOL DlgItem_SetText(HWND hDlg, UINT idDlgItem, const TCHAR* pszText);

bool SettingsDlg_OnOK(HWND hDlg);
void SettingsDlg_OnChBracketsAutoCompleteClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoTagClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoTagIfClicked(HWND hDlg);
void SettingsDlg_OnStPluginStateDblClicked(HWND hDlg);
void SettingsDlg_OnInitDialog(HWND hDlg);


INT_PTR CALLBACK SettingsDlgProc(
  HWND   hDlg, 
  UINT   uMessage, 
  WPARAM wParam, 
  LPARAM lParam)
{
    if ( uMessage == WM_COMMAND )
    {
        switch ( LOWORD(wParam) ) 
        {
            case IDC_CH_BRACKETS_AUTOCOMPLETE:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsAutoCompleteClicked(hDlg);
                }
                break;

            case IDC_CH_BRACKETS_DOTAG:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoTagClicked(hDlg);
                }
                break;

            case IDC_CH_BRACKETS_DOTAGIF:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoTagIfClicked(hDlg);
                }
                break;

            case IDC_ST_PLUGINSTATE:
                if ( HIWORD(wParam) == STN_DBLCLK )
                {
                    SettingsDlg_OnStPluginStateDblClicked(hDlg);
                }
                break;

            case IDC_BT_OK: // Run
            case IDOK:
                if ( SettingsDlg_OnOK(hDlg) )
                {
                    EndDialog(hDlg, 1); // OK - returns 1
                }
                return 1;

            case IDC_BT_CANCEL: // Cancel
            case IDCANCEL:
                EndDialog(hDlg, 0); // Cancel - returns 0
                return 1;

            default:
                break;
        }
    }

    else if ( uMessage == WM_NOTIFY )
    {
    
    }

    else if ( uMessage == WM_INITDIALOG )
    {
        SettingsDlg_OnInitDialog(hDlg);
    }

    return 0;
}

bool SettingsDlg_OnOK(HWND hDlg)
{
    HWND hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOTAGIF);
    if ( hEd )
    {
        GetWindowText(hEd, g_opt.m_szHtmlFileExts, CXBracketsOptions::STR_FILEEXTS_SIZE - 1);
    }
  
    g_opt.m_bBracketsAutoComplete = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    g_opt.m_bBracketsRightExistsOK = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_RIGHTEXISTS_OK);
    g_opt.m_bBracketsDoSingleQuote = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTE);
    g_opt.m_bBracketsDoTag = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG);
    g_opt.m_bBracketsDoTag2 = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG2);
    g_opt.m_bBracketsDoTagIf = 
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAGIF);
    g_opt.m_bBracketsSkipEscaped =
        CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_SKIPESCAPED);
  
    return TRUE;
}

void showPluginStatus(HWND hDlg, bool bActive)
{
    DlgItem_SetText( hDlg, IDC_ST_PLUGINSTATE,
      bActive ? _T("Status: the plugin is active") : 
        _T("Status: the plugin is NOT active") );
}

void SettingsDlg_OnChBracketsAutoCompleteClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_RIGHTEXISTS_OK, bEnable);
    showPluginStatus(hDlg, bEnable);
}

void SettingsDlg_OnChBracketsDoTagClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAG2, bEnable);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAGIF, bEnable);
    if ( bEnable )
    {
        bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAGIF);
    }
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOTAGIF, bEnable);
}

void SettingsDlg_OnChBracketsDoTagIfClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAGIF);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOTAGIF, bEnable);
}

void SettingsDlg_OnStPluginStateDblClicked(HWND hDlg)
{
    bool bActive = !CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    CheckBox_SetCheck(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE, bActive);
    showPluginStatus(hDlg, bActive);
}

void SettingsDlg_OnInitDialog(HWND hDlg)
{
    HWND hEd;

    AnyWindow_CenterWindow(hDlg, thePlugin.getNppWnd(), FALSE);

    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_AUTOCOMPLETE, g_opt.m_bBracketsAutoComplete);
    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_RIGHTEXISTS_OK, g_opt.m_bBracketsRightExistsOK);
    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_DOSINGLEQUOTE, g_opt.m_bBracketsDoSingleQuote);
    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_DOTAG, g_opt.m_bBracketsDoTag);
    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_DOTAG2, g_opt.m_bBracketsDoTag2);
    CheckBox_SetCheck(hDlg, 
      IDC_CH_BRACKETS_DOTAGIF, g_opt.m_bBracketsDoTagIf);
    CheckBox_SetCheck(hDlg,
      IDC_CH_BRACKETS_SKIPESCAPED, g_opt.m_bBracketsSkipEscaped);
    DlgItem_EnableWindow(hDlg, 
      IDC_CH_BRACKETS_RIGHTEXISTS_OK, g_opt.m_bBracketsAutoComplete);

    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAG2, g_opt.m_bBracketsDoTag);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAGIF, g_opt.m_bBracketsDoTag);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOTAGIF, 
      g_opt.m_bBracketsDoTag ? g_opt.m_bBracketsDoTagIf : FALSE);

    showPluginStatus(hDlg, g_opt.m_bBracketsAutoComplete);
  
    hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOTAGIF);
    if ( hEd )
    {
        SetWindowText(hEd, g_opt.m_szHtmlFileExts);
    }

}

//---------------------------------------------------------------------------

BOOL AnyWindow_CenterWindow(HWND hWnd, HWND hParentWnd, BOOL bRepaint)
{
    RECT rectParent;
    RECT rect;
    INT  height, width;
    INT  x, y;

    GetWindowRect(hParentWnd, &rectParent);
    GetWindowRect(hWnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    x = ((rectParent.right - rectParent.left) - width) / 2;
    x += rectParent.left;
    y = ((rectParent.bottom - rectParent.top) - height) / 2;
    y += rectParent.top;
    return MoveWindow(hWnd, x, y, width, height, bRepaint);
}

bool CheckBox_IsChecked(HWND hDlg, UINT idCheckBox)
{
    HWND hCheckBox = GetDlgItem(hDlg, idCheckBox);
    if ( hCheckBox )
    {
        return (SendMessage(hCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED) ? true : false;
    }
    return false;
}

bool CheckBox_SetCheck(HWND hDlg, UINT idCheckBox, bool bChecked)
{
    HWND hCheckBox = GetDlgItem(hDlg, idCheckBox);
    if ( hCheckBox )
    {
        SendMessage(hCheckBox, BM_SETCHECK,
          (WPARAM) (bChecked ? BST_CHECKED : BST_UNCHECKED), 0);
        return true;
    }
    return false;
}

BOOL DlgItem_EnableWindow(HWND hDlg, UINT idDlgItem, bool bEnable)
{
    HWND hDlgItem = GetDlgItem(hDlg, idDlgItem);
    if ( hDlgItem )
    {
        return EnableWindow(hDlgItem, bEnable);
    }
    return FALSE;
}

BOOL DlgItem_SetText(HWND hDlg, UINT idDlgItem, const TCHAR* pszText)
{
    HWND hDlgItem = GetDlgItem(hDlg, idDlgItem);
    if ( hDlgItem )
    {
        return SetWindowText(hDlgItem, pszText);
    }
    return FALSE;
}
