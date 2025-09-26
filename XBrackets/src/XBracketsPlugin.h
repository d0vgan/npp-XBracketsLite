#ifndef _xbrackets_npp_plugin_h_
#define _xbrackets_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "XBracketsLogic.h"
#include "XBracketsMenu.h"
#include "XBracketsOptions.h"
#include "CFileModificationWatcher.h"
#include <chrono>

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
        virtual ~CXBracketsPlugin();

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
        void OnSciUpdateUI(SCNotification* pscn);
        void OnSciTextChange(SCNotification* pscn);

        // custom functions
        void GoToMatchingBracket();
        void GoToNearestBracket();
        void SelToMatchingBracket();
        void SelToNearestBrackets();

        void ReadOptions();
        void SaveOptions();
        void OnConfigFileChanged(const tstr& configFilePath);
        void OnSettings();
        void OnHighlight();
        void OnHelp();

        void PluginMessageBox(const TCHAR* szMessageText, UINT uType);

    public:
        static const TCHAR* PLUGIN_NAME;

    private:
        enum eMacroState {
            MACRO_TOGGLE = -1, // invert
            MACRO_STOP   = 0,  // false
            MACRO_START  = 1   // true
        };

        struct tHighlightBrPair {
            Sci_Position nLeftBrPos{-1};
            Sci_Position nRightBrPos{-1};
            Sci_Position nLeftBrLen{0};
            Sci_Position nRightBrLen{0};
        };

        CXBracketsMenu m_PluginMenu;
        CXBracketsLogic m_BracketsLogic;
        CFileModificationWatcher m_FileWatcher;
        CConfigFileChangeListener m_ConfigFileChangeListener;
        tstr m_sIniFilePath;
        tstr m_sStaticConfigFilePath;
        tstr m_sUserConfigFilePath;
        tstr m_sCfgFileUpd;
        tHighlightBrPair m_hlBrPair[2];
        int m_nHlSciIdx;
        UINT_PTR m_nHlTimerId;
        Sci_Position m_nTextLength;
        int m_nHlSciStyleInd;
        int m_nHlSciStyleIndByNpp;
        bool m_isCfgUpdInProgress;
        CommunicationInfo m_ciCfgUpd;
        std::chrono::time_point<std::chrono::system_clock> m_lastTextChangedTimePoint;
        XBrackets::CCriticalSection m_csHl;
        XBrackets::CCriticalSection m_csCfgUpd;

        static bool    isNppMacroStarted;
        static bool    isNppWndUnicode;
        static bool    isNppReady;
        static WNDPROC nppOriginalWndProc;
        static WNDPROC sciOriginalWndProc1;
        static WNDPROC sciOriginalWndProc2;
        static LRESULT nppCallWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT sciCallWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK nppNewWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK sciNewWndProc(HWND, UINT, WPARAM, LPARAM);
        static void CALLBACK HlTimerProc(HWND, UINT, UINT_PTR, DWORD);

        void onConfigFileUpdated();
        void onConfigFileError(const tstr& configFilePath, const tstr& err);
        void onConfigFileHasBeenRead();
        void onUpdateHighlight();
        bool updateFileInfo(unsigned int uInvalidateAndUpdateFlags);
        void clearActiveBrackets(int nSciIdx = 2); // 2 - main & second
        void highlightActiveBrackets(Sci_Position pos);
        bool isHighlightEnabled() const;
};

CXBracketsPlugin& GetPlugin();

//---------------------------------------------------------------------------
#endif
