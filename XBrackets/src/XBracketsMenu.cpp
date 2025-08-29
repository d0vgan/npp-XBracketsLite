#include "XBracketsMenu.h"
#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"
#include "SettingsDlg.h"
#include "resource.h"


FuncItem CXBracketsMenu::arrFuncItems[N_NBFUNCITEMS] = {
    { _T("Autocomplete brackets"),      funcAutocomplete,         0, false, NULL },
    { _T("Settings..."),                funcSettings,             0, false, NULL },
    { _T(""),                           NULL,                     0, false, NULL }, // separator
    { _T("Go To Matching Bracket"),     funcGoToMatchingBracket,  0, false, NULL },
    { _T("Go To Nearest Bracket"),      funcGoToNearestBracket,   0, false, NULL },
    { _T("Select To Matching Bracket"), funcSelToMatchingBracket, 0, false, NULL },
    { _T("Select To Nearest Brackets"), funcSelToNearestBrackets, 0, false, NULL },
    { _T(""),                           NULL,                     0, false, NULL }, // separator
    { _T("Help..."),                    funcHelp,                 0, false, NULL },
    { _T("About"),                      funcAbout,                0, false, NULL }
};

void CXBracketsMenu::funcAutocomplete()
{
    GetOptions().setBracketsAutoComplete( !GetOptions().getBracketsAutoComplete() );
    UpdateMenuState();
}

void CXBracketsMenu::funcSettings()
{
    GetPlugin().OnSettings();
}

void CXBracketsMenu::funcGoToMatchingBracket()
{
    GetPlugin().GoToMatchingBracket();
}

void CXBracketsMenu::funcGoToNearestBracket()
{
    GetPlugin().GoToNearestBracket();
}

void CXBracketsMenu::funcSelToMatchingBracket()
{
    GetPlugin().SelToMatchingBracket();
}

void CXBracketsMenu::funcSelToNearestBrackets()
{
    GetPlugin().SelToNearestBrackets();
}

void CXBracketsMenu::funcHelp()
{
    GetPlugin().OnHelp();
}

void CXBracketsMenu::funcAbout()
{
    GetPlugin().PluginMessageBox(
        _T("XBrackets Lite ver. 2.0.0\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Jan 2009 - Sep 2025\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Oct 2006 (original idea)"),
        MB_OK
      );
}

INT_PTR CXBracketsMenu::PluginDialogBox(WORD idDlg, DLGPROC lpDlgFunc)
{
    // static function uses the static getter 'GetPlugin()'
    return ::DialogBox( (HINSTANCE) GetPlugin().getDllModule(),
               MAKEINTRESOURCE(idDlg), m_nppMsgr.getNppWnd(), lpDlgFunc );
}

void CXBracketsMenu::UpdateMenuState()
{
    HMENU hMenu = ::GetMenu( m_nppMsgr.getNppWnd() );
    ::CheckMenuItem(hMenu, arrFuncItems[N_AUTOCOMPLETE]._cmdID,
        MF_BYCOMMAND | (GetOptions().getBracketsAutoComplete() ? MF_CHECKED : MF_UNCHECKED));

    if ( GetOptions().getBracketsAutoComplete() )
    {
        GetPlugin().OnNppBufferActivated();
    }
}

void CXBracketsMenu::AllowAutocomplete(bool bAllow)
{
    HMENU hMenu = ::GetMenu( m_nppMsgr.getNppWnd() );
    ::EnableMenuItem(hMenu, arrFuncItems[N_AUTOCOMPLETE]._cmdID,
        MF_BYCOMMAND | (bAllow ? MF_ENABLED : MF_GRAYED));
    ::EnableMenuItem(hMenu, arrFuncItems[N_SETTINGS]._cmdID,
        MF_BYCOMMAND | (bAllow ? MF_ENABLED : MF_GRAYED));
}
