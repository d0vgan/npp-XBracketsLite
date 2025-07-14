#ifndef _xbrackets_npp_plugin_menu_h_
#define _xbrackets_npp_plugin_menu_h_
//---------------------------------------------------------------------------
#include "core/NppPluginMenu.h"

class CXBracketsMenu : public CNppPluginMenu
{
    public:
        enum NMenuItems {
            N_AUTOCOMPLETE = 0,
            N_SETTINGS,
            N_SEPARATOR1,
            N_GOTOMATCHINGBR,
            N_GOTONEARESTBR,
            N_SELTOMATCHINGBR,
            N_SELTONEARESTBR,
            N_SEPARATOR2,
            N_ABOUT,
            
            N_NBFUNCITEMS
        };
        static FuncItem arrFuncItems[N_NBFUNCITEMS];

    public:    
        static INT_PTR PluginDialogBox(WORD idDlg, DLGPROC lpDlgFunc);
        static void    UpdateMenuState();
        static void    AllowAutocomplete(bool bAllow);
        static void    funcAutocomplete();
        static void    funcSettings();
        static void    funcGoToMatchingBracket();
        static void    funcGoToNearestBracket();
        static void    funcSelToMatchingBracket();
        static void    funcSelToNearestBrackets();
        static void    funcAbout();
        
};

//---------------------------------------------------------------------------
#endif
