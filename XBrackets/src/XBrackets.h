#ifndef _xbrackets_npp_plugin_h_
#define _xbrackets_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "XBracketsMenu.h"

class CXBrackets : public CNppPlugin
{
    public:
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
        unsigned int m_uFileType;

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
        eCharProcessingResult OnSciChar(const int ch);

        // custom functions
        void GoToMatchingBracket();
        void GoToNearestBracket();
        void SelToMatchingBracket();
        void SelToNearestBrackets();

        void ReadOptions();
        void SaveOptions();

    protected:
        enum eDupPairDirection
        {
            DP_NONE          = 0x00,
            DP_FORWARD       = 0x01,
            DP_BACKWARD      = 0x02,
            DP_DETECT        = 0x08,
            DP_MAYBEFORWARD  = (DP_DETECT | DP_FORWARD),
            DP_MAYBEBACKWARD = (DP_DETECT | DP_BACKWARD)
        };

        enum eAtBrChar {
            abcNone        = 0x00,
            abcLeftBr      = 0x01, // left:  (
            abcRightBr     = 0x02, // right: )
            abcDetectBr    = 0x04, // can be either left or right
            abcBrIsOnLeft  = 0x10, // on left:  (|  or  )|
            abcBrIsOnRight = 0x20  // on right: |(  or  |)
        };

        enum eGetBracketsAction {
            baGoToMatching = 0,
            baSelToMatching,
            baGoToNearest,
            baSelToNearest
        };

        struct tGetBracketsState
        {
            Sci_Position nSelStart{-1};
            Sci_Position nSelEnd{-1};
            Sci_Position nCharPos{-1};
            Sci_Position nLeftBrPos{-1};
            Sci_Position nRightBrPos{-1};
            TBracketType nLeftBrType{tbtNone};
            TBracketType nRightBrType{tbtNone};
            eDupPairDirection nLeftDupDirection{DP_NONE};
            eDupPairDirection nRightDupDirection{DP_NONE};
        };

        // custom functions
        eCharProcessingResult AutoBracketsFunc(TBracketType nBracketType);
        bool SelAutoBrFunc(TBracketType nBracketType);
        void UpdateFileType();
        bool isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, TBracketType* pnBracketType, bool bInSelection);
        unsigned int getFileType();

        bool isDoubleQuoteSupported() const;
        bool isSingleQuoteSupported() const;
        bool isTagSupported() const;
        bool isTag2Supported() const;
        bool isSkipEscapedSupported() const;

        enum eBracketOptions {
            bofIgnoreMode = 0x01
        };
        TBracketType getLeftBracketType(const int ch, unsigned int uOptions = 0) const;
        TBracketType getRightBracketType(const int ch, unsigned int uOptions = 0) const;

        static int getDirectionIndex(const eDupPairDirection direction);
        static int getDirectionRank(const eDupPairDirection leftDirection, const eDupPairDirection rightDirection);

        bool isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const;
        bool isDuplicatedPair(TBracketType nBracketType) const;
        eDupPairDirection getDuplicatedPairDirection(const CSciMessager& sciMsgr, const Sci_Position nCharPos, const char curr_ch) const;
        unsigned int isAtBracketCharacter(const CSciMessager& sciMsgr, const Sci_Position nCharPos, TBracketType* out_nBrType, eDupPairDirection* out_nDupDirection) const;
        bool findLeftBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state);
        bool findRightBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state);
        void performBracketsAction(eGetBracketsAction nBrAction);

    protected:
        enum eMacroState {
            MACRO_TOGGLE = -1, // invert
            MACRO_STOP   = 0,  // false
            MACRO_START  = 1   // true
        };
        
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
