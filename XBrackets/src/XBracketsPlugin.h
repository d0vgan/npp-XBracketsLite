#ifndef _xbrackets_npp_plugin_h_
#define _xbrackets_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "XBracketsLogic.h"
#include "XBracketsMenu.h"

class CXBracketsPlugin : public CNppPlugin
{
    public:
        // standard n++ plugin functions
        virtual void         nppBeNotified(SCNotification* pscn) override;
        virtual FuncItem*    nppGetFuncsArray(int* pnbFuncItems) override;
        virtual const TCHAR* nppGetName() override;

        // common n++ notification
        virtual void OnNppSetInfo(const NppData& nppd) override;

        // custom n++ notifications
        void OnNppBufferActivated();
        void OnNppBufferReload();
        void OnNppFileOpened();
        void OnNppFileReload();
        void OnNppFileSaved();
        void OnNppReady();
        void OnNppShutdown();
        void OnNppMacro(int nMacroState);

        // custom scintilla notifications
        CXBracketsLogic::eCharProcessingResult OnSciChar(const int ch);
        void OnSciModified(SCNotification* pscn);
        void OnSciTextChanged();

        // custom functions
        void GoToMatchingBracket();
        void GoToNearestBracket();
        void SelToMatchingBracket();
        void SelToNearestBrackets();

        void ReadOptions();
        void SaveOptions();

    public:
        static const TCHAR* PLUGIN_NAME;

    private:
        enum eMacroState {
            MACRO_TOGGLE = -1, // invert
            MACRO_STOP   = 0,  // false
            MACRO_START  = 1   // true
        };

        CXBracketsMenu m_PluginMenu;
        CXBracketsLogic m_BracketsLogic;

        static bool    isNppMacroStarted;
        static bool    isNppWndUnicode;
        static WNDPROC nppOriginalWndProc;
        static WNDPROC sciOriginalWndProc1;
        static WNDPROC sciOriginalWndProc2;
        static LRESULT nppCallWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT sciCallWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK nppNewWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK sciNewWndProc(HWND, UINT, WPARAM, LPARAM);
};

//---------------------------------------------------------------------------
#endif
