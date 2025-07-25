#include "XBracketsMenu.h"
#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"
#include "SettingsDlg.h"
#include "resource.h"


extern CXBracketsPlugin  thePlugin;
extern CXBracketsOptions g_opt;


FuncItem CXBracketsMenu::arrFuncItems[N_NBFUNCITEMS] = {
    { _T("Autocomplete brackets"),      funcAutocomplete,         0, false, NULL },
    { _T("Settings..."),                funcSettings,             0, false, NULL },
    { _T(""),                           NULL,                     0, false, NULL }, // separator
    { _T("Go To Matching Bracket"),     funcGoToMatchingBracket,  0, false, NULL },
    { _T("Go To Nearest Bracket"),      funcGoToNearestBracket,   0, false, NULL },
    { _T("Select To Matching Bracket"), funcSelToMatchingBracket, 0, false, NULL },
    { _T("Select To Nearest Brackets"), funcSelToNearestBrackets, 0, false, NULL },
    { _T(""),                           NULL,                     0, false, NULL }, // separator
    { _T("About"),                      funcAbout,                0, false, NULL }
};

void CXBracketsMenu::funcAutocomplete()
{
    g_opt.setBracketsAutoComplete( !g_opt.getBracketsAutoComplete() );
    UpdateMenuState();
}

void CXBracketsMenu::funcSettings()
{
    thePlugin.OnSettings();
}

void CXBracketsMenu::funcGoToMatchingBracket()
{
    thePlugin.GoToMatchingBracket();
}

void CXBracketsMenu::funcGoToNearestBracket()
{
    thePlugin.GoToNearestBracket();
}

void CXBracketsMenu::funcSelToMatchingBracket()
{
    thePlugin.SelToMatchingBracket();
}

void CXBracketsMenu::funcSelToNearestBrackets()
{
    thePlugin.SelToNearestBrackets();
}

void CXBracketsMenu::funcAbout()
{
    thePlugin.PluginMessageBox(
        _T("XBrackets Lite ver. 1.4.0\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Jan 2009 - Jul 2025\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Oct 2006 (original idea)"),
        MB_OK
      );
}

INT_PTR CXBracketsMenu::PluginDialogBox(WORD idDlg, DLGPROC lpDlgFunc)
{
    // static function uses static (global) variable 'thePlugin'
    return ::DialogBox( (HINSTANCE) thePlugin.getDllModule(),
               MAKEINTRESOURCE(idDlg), m_nppMsgr.getNppWnd(), lpDlgFunc );
}

void CXBracketsMenu::UpdateMenuState()
{
    HMENU hMenu = ::GetMenu( m_nppMsgr.getNppWnd() );
    ::CheckMenuItem(hMenu, arrFuncItems[N_AUTOCOMPLETE]._cmdID,
        MF_BYCOMMAND | (g_opt.getBracketsAutoComplete() ? MF_CHECKED : MF_UNCHECKED));

    if ( g_opt.getBracketsAutoComplete() )
    {
        thePlugin.OnNppBufferActivated();
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
