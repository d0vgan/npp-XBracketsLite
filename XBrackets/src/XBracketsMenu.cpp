#include "XBracketsMenu.h"
#include "XBrackets.h"
#include "XBracketsOptions.h"
#include "SettingsDlg.h"
#include "resource.h"


extern CXBrackets        thePlugin;
extern CXBracketsOptions g_opt;


FuncItem CXBracketsMenu::arrFuncItems[N_NBFUNCITEMS] = {
    { _T("Autocomplete brackets"), funcAutocomplete, 0, false, NULL },
    { _T("Settings..."),           funcSettings,     0, false, NULL },
    { _T(""),                      NULL,             0, false, NULL }, // separator
    { _T("About"),                 funcAbout,        0, false, NULL }
};

void CXBracketsMenu::funcAutocomplete()
{
    g_opt.m_bBracketsAutoComplete = !g_opt.m_bBracketsAutoComplete;
    UpdateMenuState();
}

void CXBracketsMenu::funcSettings()
{
    if ( PluginDialogBox(IDD_SETTINGS, SettingsDlgProc) == 1 )
    {
        UpdateMenuState();
        thePlugin.SaveOptions();
    }
}

void CXBracketsMenu::funcAbout()
{
    ::MessageBox( 
        m_nppMsgr.getNppWnd(),
        _T("XBrackets Lite ver. 1.3.0\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Jan 2009 - Feb 2018\r\n") \
        _T("(C) Vitaliy Dovgan aka DV, Oct 2006 (original idea)"),
        _T("XBrackets plugin for Notepad++"),
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
        MF_BYCOMMAND | (g_opt.m_bBracketsAutoComplete ? MF_CHECKED : MF_UNCHECKED));

    if ( g_opt.m_bBracketsAutoComplete )
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
