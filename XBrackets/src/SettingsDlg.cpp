#include "SettingsDlg.h"
#include "resource.h"

/*
#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"


BOOL AnyWindow_CenterWindow(HWND hWnd, HWND hParentWnd, BOOL bRepaint);
bool CheckBox_IsChecked(HWND hDlg, UINT idCheckBox);
bool CheckBox_SetCheck(HWND hDlg, UINT idCheckBox, bool bChecked);
BOOL DlgItem_EnableWindow(HWND hDlg, UINT idDlgItem, bool bEnable);
BOOL DlgItem_SetText(HWND hDlg, UINT idDlgItem, const TCHAR* pszText);

bool SettingsDlg_OnOK(HWND hDlg);
void SettingsDlg_OnChBracketsAutoCompleteClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoDoubleQuoteClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoSingleQuoteClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoSingleQuoteIfClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoTagClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoTagIfClicked(HWND hDlg);
void SettingsDlg_OnChBracketsDoSkipEscapedClicked(HWND hDlg);
void SettingsDlg_OnStPluginStateDblClicked(HWND hDlg);
void SettingsDlg_OnInitDialog(HWND hDlg);

void SettingsDlg_SetAutocompleteBracketsText(HWND hDlg);

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
            case IDC_CH_BRACKETS_DODOUBLEQUOTE:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoDoubleQuoteClicked(hDlg);
                }
                break;

            case IDC_CH_BRACKETS_DOSINGLEQUOTE:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoSingleQuoteClicked(hDlg);
                }
                break;

            case IDC_CH_BRACKETS_DOSINGLEQUOTEIF:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoSingleQuoteIfClicked(hDlg);
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

            case IDC_CH_BRACKETS_SKIPESCAPED:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsDoSkipEscapedClicked(hDlg);
                }
                break;

            case IDC_CH_BRACKETS_AUTOCOMPLETE:
                if ( HIWORD(wParam) == BN_CLICKED )
                {
                    SettingsDlg_OnChBracketsAutoCompleteClicked(hDlg);
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
    TCHAR szFileExts[CXBracketsOptions::STR_FILEEXTS_SIZE];

    HWND hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOSINGLEQUOTEIF);
    if ( hEd )
    {
        szFileExts[0] = 0;
        GetWindowText(hEd, szFileExts, CXBracketsOptions::STR_FILEEXTS_SIZE - 1);
        CharLower(szFileExts);
        GetOptions().setSglQuoteFileExts(szFileExts);
    }

    hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOTAGIF);
    if ( hEd )
    {
        szFileExts[0] = 0;
        GetWindowText(hEd, szFileExts, CXBracketsOptions::STR_FILEEXTS_SIZE - 1);
        CharLower(szFileExts);
        GetOptions().setHtmlFileExts(szFileExts);
    }

    hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_SKIPESCAPED);
    if ( hEd )
    {
        szFileExts[0] = 0;
        GetWindowText(hEd, szFileExts, CXBracketsOptions::STR_FILEEXTS_SIZE - 1);
        CharLower(szFileExts);
        GetOptions().setEscapedFileExts(szFileExts);
    }

    GetOptions().setBracketsAutoComplete( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE) );
    GetOptions().setBracketsRightExistsOK( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_RIGHTEXISTS_OK) );
    GetOptions().setBracketsDoDoubleQuote( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DODOUBLEQUOTE) );
    GetOptions().setBracketsDoSingleQuote( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTE) );
    GetOptions().setBracketsDoSingleQuoteIf( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTEIF) );
    GetOptions().setBracketsDoTag( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG) );
    GetOptions().setBracketsDoTag2( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG2) );
    GetOptions().setBracketsDoTagIf( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAGIF) );
    GetOptions().setBracketsSkipEscaped( CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_SKIPESCAPED) );

    return TRUE;
}

void showPluginStatus(HWND hDlg, bool bActive)
{
    DlgItem_SetText( hDlg, IDC_ST_PLUGINSTATE,
      bActive ? _T("Status: Auto-completion is active") :
        _T("Status: Auto-completion is NOT active") );
}

void SettingsDlg_OnChBracketsAutoCompleteClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_RIGHTEXISTS_OK, bEnable);
    showPluginStatus(hDlg, bEnable);
}

void SettingsDlg_OnChBracketsDoDoubleQuoteClicked(HWND hDlg)
{
    SettingsDlg_SetAutocompleteBracketsText(hDlg);
}

void SettingsDlg_OnChBracketsDoSingleQuoteClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTE);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTEIF, bEnable);
    if ( bEnable )
    {
        bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTEIF);
    }
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOSINGLEQUOTEIF, bEnable);
    SettingsDlg_SetAutocompleteBracketsText(hDlg);
}

void SettingsDlg_OnChBracketsDoSingleQuoteIfClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTEIF);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOSINGLEQUOTEIF, bEnable);
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
    SettingsDlg_SetAutocompleteBracketsText(hDlg);
}

void SettingsDlg_OnChBracketsDoTagIfClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAGIF);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOTAGIF, bEnable);
}

void SettingsDlg_OnChBracketsDoSkipEscapedClicked(HWND hDlg)
{
    bool bEnable = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_SKIPESCAPED);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_SKIPESCAPED, bEnable);
    DlgItem_EnableWindow(hDlg, IDC_ST_BRACKETS_SKIPESCAPED, bEnable);
}

void SettingsDlg_OnStPluginStateDblClicked(HWND hDlg)
{
    bool bActive = !CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    CheckBox_SetCheck(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE, bActive);
    showPluginStatus(hDlg, bActive);
}

void SettingsDlg_OnInitDialog(HWND hDlg)
{
    AnyWindow_CenterWindow(hDlg, GetPlugin().getNppWnd(), FALSE);

    CheckBox_SetCheck(hDlg,
      IDC_CH_BRACKETS_DODOUBLEQUOTE, GetOptions().getBracketsDoDoubleQuote());
    CheckBox_SetCheck(hDlg,
      IDC_CH_BRACKETS_DOSINGLEQUOTE, GetOptions().getBracketsDoSingleQuote());
    CheckBox_SetCheck(hDlg,
        IDC_CH_BRACKETS_DOSINGLEQUOTEIF, GetOptions().getBracketsDoSingleQuoteIf());
    CheckBox_SetCheck(hDlg,
        IDC_CH_BRACKETS_DOTAG, GetOptions().getBracketsDoTag());
    CheckBox_SetCheck(hDlg,
        IDC_CH_BRACKETS_DOTAG2, GetOptions().getBracketsDoTag2());
    CheckBox_SetCheck(hDlg,
        IDC_CH_BRACKETS_DOTAGIF, GetOptions().getBracketsDoTagIf());
    CheckBox_SetCheck(hDlg,
        IDC_CH_BRACKETS_SKIPESCAPED, GetOptions().getBracketsSkipEscaped());
    CheckBox_SetCheck(hDlg,
      IDC_CH_BRACKETS_AUTOCOMPLETE, GetOptions().getBracketsAutoComplete());
    CheckBox_SetCheck(hDlg,
      IDC_CH_BRACKETS_RIGHTEXISTS_OK, GetOptions().getBracketsRightExistsOK());

    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTEIF, GetOptions().getBracketsDoSingleQuote());
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOSINGLEQUOTEIF,
        GetOptions().getBracketsDoSingleQuote() ? GetOptions().getBracketsDoSingleQuoteIf() : false);
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAG2, GetOptions().getBracketsDoTag());
    DlgItem_EnableWindow(hDlg, IDC_CH_BRACKETS_DOTAGIF, GetOptions().getBracketsDoTag());
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_DOTAGIF,
      GetOptions().getBracketsDoTag() ? GetOptions().getBracketsDoTagIf() : false);
    DlgItem_EnableWindow(hDlg, IDC_ED_BRACKETS_SKIPESCAPED, GetOptions().getBracketsSkipEscaped());
    DlgItem_EnableWindow(hDlg, IDC_ST_BRACKETS_SKIPESCAPED, GetOptions().getBracketsSkipEscaped());
    DlgItem_EnableWindow(hDlg,
        IDC_CH_BRACKETS_RIGHTEXISTS_OK, GetOptions().getBracketsAutoComplete());

    showPluginStatus(hDlg, GetOptions().getBracketsAutoComplete());

    HWND hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOSINGLEQUOTEIF);
    if ( hEd )
    {
        SetWindowText(hEd, GetOptions().getSglQuoteFileExts().c_str());
    }

    hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_DOTAGIF);
    if ( hEd )
    {
        SetWindowText(hEd, GetOptions().getHtmlFileExts().c_str());
    }

    hEd = GetDlgItem(hDlg, IDC_ED_BRACKETS_SKIPESCAPED);
    if ( hEd )
    {
        SetWindowText(hEd, GetOptions().getEscapedFileExts().c_str());
    }

    SettingsDlg_SetAutocompleteBracketsText(hDlg);
}

void SettingsDlg_SetAutocompleteBracketsText(HWND hDlg)
{
    TCHAR szCurrText[64];
    TCHAR szNewText[64];

    HWND hCh = GetDlgItem(hDlg, IDC_CH_BRACKETS_AUTOCOMPLETE);
    if ( !hCh )
        return;

    szCurrText[0] = 0;
    GetWindowText(hCh, szCurrText, 128 - 1);

    const TCHAR* pCurr = szCurrText;
    TCHAR* pNew = szNewText;

    for ( ; ; )
    {
        // copying the leading part of the string, including '{'
        *(pNew++) = *pCurr;
        if ( *pCurr == 0 || *pCurr == _T('{') )
            break;

        ++pCurr;
    }

    if ( *pCurr != _T('{') )
        return; // '{' not found

    for ( ; ; )
    {
        // skipping until '}'
        ++pCurr;

        if ( *pCurr == 0 || *pCurr == _T('}') )
            break;
    }

    if ( *pCurr != _T('}') )
        return; // '}' not found

    bool bTag = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOTAG);
    bool bDblQuote = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DODOUBLEQUOTE);
    bool bSglQuote = CheckBox_IsChecked(hDlg, IDC_CH_BRACKETS_DOSINGLEQUOTE);

    // adding left brackets
    if ( bTag )
        *(pNew++) = _T('<');
    if ( bDblQuote )
        *(pNew++) = _T('"');
    if ( bSglQuote )
        *(pNew++) = _T('\'');

    // adding right brackets
    if ( bSglQuote )
        *(pNew++) = _T('\'');
    if ( bDblQuote )
        *(pNew++) = _T('"');
    if ( bTag )
        *(pNew++) = _T('>');

    // copying the remaining part of the string, starting from '}'
    lstrcpy(pNew, pCurr);

    SetWindowText(hCh, szNewText);
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

*/
