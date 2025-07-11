#ifndef _xbrackets_npp_plugin_h_
#define _xbrackets_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "XBracketsMenu.h"
#include <utility>
#include <memory>
#include <atomic>

class CXBrackets : public CNppPlugin
{
    public:
        enum TFileType {
            tftNone = 0,
            tftText,
            tftC_Cpp,
            tftH_Hpp,
            tftPas
        };

        enum TFileType2 {
            tfmNone           = 0x0000,
            tfmComment1       = 0x0001,
            tfmHtmlCompatible = 0x0002,
            tfmEscaped1       = 0x0004,
            tfmSingleQuote    = 0x0008,
            tfmIsSupported    = 0x1000
        };

        enum TBracketType {
            tbtNone = 0,
            tbtBracket,  //  (
            tbtSquare,   //  [
            tbtBrace,    //  {
            tbtDblQuote, //  "
            tbtSglQuote, //  '
            tbtTag,      //  <
            tbtTag2,     //  />

            tbtCount
        };

        enum eCharProcessingResult {
            cprNone = 0,
            cprBrAutoCompl,
            cprSelAutoCompl
        };

        enum eConsts {
            MAX_ESCAPED_PREFIX  = 20
        };

        static const TCHAR* PLUGIN_NAME;
        static const char*  strBrackets[tbtCount];

    protected:
        // plugin menu
        CXBracketsMenu m_PluginMenu;
        
        // internal vars
        Sci_Position m_nAutoRightBracketPos;
        std::pair<TFileType, unsigned short> m_nnFileType;
        Sci_Position m_nSelPos;
        Sci_Position m_nSelLen;
        std::unique_ptr<char[]> m_pSelText;
        std::atomic_bool m_isProcessingSelAutoBr;
        UINT_PTR m_nDelTextTimerId;
        CRITICAL_SECTION m_csDelTextTimer;

    public:
        CXBrackets();
        virtual ~CXBrackets();

        // standard n++ plugin functions
        virtual void         nppBeNotified(SCNotification* pscn) override;
        virtual FuncItem*    nppGetFuncsArray(int* pnbFuncItems) override;
        virtual const TCHAR* nppGetName() override;

        // common n++ notification
        virtual void OnNppSetInfo(const NppData& nppd) override;

        // custom n++ notifications
        void OnNppBufferActivated();
        void OnNppFileOpened();
        void OnNppFileSaved();
        void OnNppReady();
        void OnNppShutdown();
        void OnNppMacro(int nMacroState);

        // custom scintilla notifications
        eCharProcessingResult OnSciCharAdded(const int ch);
        bool OnSciBeforeDeleteText(const SCNotification* pscn);

        // timer
        void OnDelTextTimer(UINT_PTR idEvent);

        // custom functions
        void ReadOptions();
        void SaveOptions();

    protected:
        // custom functions
        eCharProcessingResult AutoBracketsFunc(int nBracketType);
        eCharProcessingResult OnSciCharAdded_Internal(const int ch);
        bool PrepareSelAutoBrFunc();
        bool SelAutoBrFunc(int nBracketType);
        void UpdateFileType();
        bool isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, int* pnBracketType, bool bInSelection);
        std::pair<TFileType, unsigned short> getFileType();

        enum eBracketOptions {
            bofIgnoreMode = 0x01
        };
        TBracketType getLeftBracketType(const int ch, unsigned int uOptions = 0) const;
        TBracketType getRightBracketType(const int ch, unsigned int uOptions = 0) const;

    protected:
        enum eMacroState {
            MACRO_TOGGLE = -1, // invert
            MACRO_STOP   = 0,  // false
            MACRO_START  = 1   // true
        };
        
        static bool    isNppMacroStarted;
        static bool    isNppWndUnicode;
        static WNDPROC nppOriginalWndProc;
        static LRESULT nppCallWndProc(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK nppNewWndProc(HWND, UINT, WPARAM, LPARAM);
};

//---------------------------------------------------------------------------
#endif
