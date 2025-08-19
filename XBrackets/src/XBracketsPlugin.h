#ifndef _xbrackets_npp_plugin_h_
#define _xbrackets_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "XBracketsLogic.h"
#include "XBracketsMenu.h"
#include "XBracketsOptions.h"
#include "CFileModificationWatcher.h"

class CXBracketsPlugin : public CNppPlugin
{
    private:
        class CConfigFileChangeListener : public IFileChangeListener
        {
        public:
            CConfigFileChangeListener(CXBracketsPlugin* pPlugin);

            virtual void HandleFileChange(const FileInfoStruct* pFile) override;

        private:
            CXBracketsPlugin* m_pPlugin;
        };

    public:
        CXBracketsPlugin();

        // standard n++ plugin functions
        virtual void         nppBeNotified(SCNotification* pscn) override;
        virtual FuncItem*    nppGetFuncsArray(int* pnbFuncItems) override;
        virtual const TCHAR* nppGetName() override;
        virtual LRESULT      nppMessageProc(UINT uMessage, WPARAM wParam, LPARAM lParam) override;

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
        LRESULT OnNppMsgToPlugin(CommunicationInfo* pInfo);

        // custom scintilla notifications
        CXBracketsLogic::eCharProcessingResult OnSciChar(const int ch);
        void OnSciModified(SCNotification* pscn);
        void OnSciAutoCompleted(SCNotification* pscn);
        void OnSciTextChanged(SCNotification* pscn);

        // custom functions
        void GoToMatchingBracket();
        void GoToNearestBracket();
        void SelToMatchingBracket();
        void SelToNearestBrackets();

        void ReadOptions();
        void SaveOptions();
        void OnSettings();
        void OnConfigFileChanged(const tstr& configFilePath);

        void PluginMessageBox(const TCHAR* szMessageText, UINT uType);

    private:
        void onConfigFileError(const tstr& configFilePath, const tstr& err);

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
        CFileModificationWatcher m_FileWatcher;
        CConfigFileChangeListener m_ConfigFileChangeListener;
        tstr m_sIniFilePath;
        tstr m_sConfigFilePath;
        tstr m_sUserConfigFilePath;

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

CXBracketsPlugin& GetPlugin();

//---------------------------------------------------------------------------
#endif
