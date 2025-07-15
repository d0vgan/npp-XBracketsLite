#include "XBrackets.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"
#include <vector>
#include <stack>


// can be _T(x), but _T(x) may be incompatible with ANSI mode
#define _TCH(x)  (x)


extern CXBrackets thePlugin;
CXBracketsOptions g_opt;

const TCHAR* CXBrackets::PLUGIN_NAME = _T("XBrackets Lite");
const char* CXBrackets::strBrackets[tbtCount] = {
    "",     // tbtNone
    "()",   // tbtBracket
    "[]",   // tbtSquare
    "{}",   // tbtBrace
    "\"\"", // tbtDblQuote
    "\'\'", // tbtSglQuote
    "<>",   // tbtTag
    "</>"   // tbtTag2
};

bool CXBrackets::isNppMacroStarted = false;
bool CXBrackets::isNppWndUnicode = true;
WNDPROC CXBrackets::nppOriginalWndProc = NULL;
WNDPROC CXBrackets::sciOriginalWndProc1 = NULL;
WNDPROC CXBrackets::sciOriginalWndProc2 = NULL;

namespace
{
    void getEscapedPrefixPos(const Sci_Position nOffset, Sci_Position* pnPos, int* pnLen)
    {
        if ( nOffset > CXBrackets::MAX_ESCAPED_PREFIX )
        {
            *pnPos = nOffset - CXBrackets::MAX_ESCAPED_PREFIX;
            *pnLen = CXBrackets::MAX_ESCAPED_PREFIX;
        }
        else
        {
            *pnPos = 0;
            *pnLen = static_cast<int>(nOffset);
        }
    }

    bool isEscapedPrefix(const char* str, int len)
    {
        int k = 0;
        while ( (len > 0) && (str[--len] == '\\') )
        {
            ++k;
        }
        return (k % 2) ? true : false;
    }

    bool isWhiteSpaceOrNulChar(const char ch)
    {
        switch ( ch )
        {
        case '\x00':
        case '\x0D':
        case '\x0A':
        case ' ':
        case '\t':
            return true;
        }

        return false;
    }

    bool isSepOrOneOf(const char ch, const char* pChars)
    {
        return ( isWhiteSpaceOrNulChar(ch) || strchr(pChars, ch) != NULL );
    }

    bool isSentenceEndChar(const char ch)
    {
        switch ( ch )
        {
        case '.':
        case '?':
        case '!':
            return true;
        }
        return false;
    }
}

LRESULT CXBrackets::nppCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pCallWindowProc = isNppWndUnicode ? CallWindowProcW : CallWindowProcA;
    return pCallWindowProc(nppOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CXBrackets::sciCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pSciWndProc = thePlugin.getNppMsgr().getSciMainWnd() == hWnd ? sciOriginalWndProc1 : sciOriginalWndProc2;
    auto pCallWindowProc = isNppWndUnicode ? CallWindowProcW : CallWindowProcA;
    return pCallWindowProc(pSciWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CXBrackets::nppNewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_COMMAND )
    {
        const WORD id = LOWORD(wParam);
        switch ( id )
        {
            case IDM_MACRO_STARTRECORDINGMACRO:
                thePlugin.OnNppMacro(MACRO_START);
                break;

            case IDM_MACRO_STOPRECORDINGMACRO:
                thePlugin.OnNppMacro(MACRO_STOP);
                break;

            case IDC_EDIT_TOGGLEMACRORECORDING:
                thePlugin.OnNppMacro(MACRO_TOGGLE);
                break;

            case IDM_MACRO_PLAYBACKRECORDEDMACRO:
                {
                    thePlugin.OnNppMacro(MACRO_START);
                    LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
                    thePlugin.OnNppMacro(MACRO_STOP);
                    return lResult;
                }
                break;

            default:
                if ( (id >= ID_MACRO) && (id < ID_MACRO_LIMIT) )
                {
                    thePlugin.OnNppMacro(MACRO_START);
                    LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
                    thePlugin.OnNppMacro(MACRO_STOP);
                    return lResult;
                }
                break;
        }
    }
    else if ( uMsg == WM_MACRODLGRUNMACRO )
    {
        thePlugin.OnNppMacro(MACRO_START);
        LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
        thePlugin.OnNppMacro(MACRO_STOP);
        return lResult;
    }

    return nppCallWndProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CXBrackets::sciNewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_CHAR )
    {
        // this happens _before_ the character is processed by Scintilla
        if ( thePlugin.OnSciChar(static_cast<int>(wParam)) != cprNone )
        {
            return 0; // processed by XBrackets, don't forward to Scintilla
        }
    }
    else if ( uMsg == WM_KEYDOWN )
    {
        if ( wParam == VK_DELETE || wParam == VK_BACK )
        {
            thePlugin.m_nAutoRightBracketPos = -1;
        }
    }

    return sciCallWndProc(hWnd, uMsg, wParam, lParam);
}

CXBrackets::CXBrackets() :
  m_nAutoRightBracketPos(-1),
  m_uFileType(tfmIsSupported)
{
}

CXBrackets::~CXBrackets()
{
}

FuncItem* CXBrackets::nppGetFuncsArray(int* pnbFuncItems)
{
    *pnbFuncItems = CXBracketsMenu::N_NBFUNCITEMS;
    return CXBracketsMenu::arrFuncItems;
}

const TCHAR* CXBrackets::nppGetName()
{
    return PLUGIN_NAME;
}

void CXBrackets::nppBeNotified(SCNotification* pscn)
{
    if ( pscn->nmhdr.hwndFrom == m_nppMsgr.getNppWnd() )
    {
        // >>> notifications from Notepad++
        switch ( pscn->nmhdr.code )
        {
            case NPPN_BUFFERACTIVATED:
                OnNppBufferActivated();
                break;

            case NPPN_FILEOPENED:
                OnNppFileOpened();
                break;

            case NPPN_FILESAVED:
                OnNppFileSaved();
                break;

            case NPPN_READY:
                OnNppReady();
                break;

            case NPPN_SHUTDOWN:
                OnNppShutdown();
                break;

            default:
                break;
        }
        // <<< notifications from Notepad++
    }
}

void CXBrackets::OnNppSetInfo(const NppData& nppd)
{
    m_PluginMenu.setNppData(nppd);
    isNppWndUnicode = ::IsWindowUnicode(nppd._nppHandle) ? true : false;
}

void CXBrackets::OnNppBufferActivated()
{
    UpdateFileType();
}

void CXBrackets::OnNppFileOpened()
{
    //this handler is not needed because file opening
    //is handled by OnNppBufferActivated()
    // UpdateFileType();
}

void CXBrackets::OnNppFileSaved()
{
    UpdateFileType();
}

void CXBrackets::OnNppReady()
{
    ReadOptions();
    CXBracketsMenu::UpdateMenuState();
    UpdateFileType();

    auto pSetWindowLongPtr = isNppWndUnicode ? SetWindowLongPtrW : SetWindowLongPtrA;

    nppOriginalWndProc = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppNewWndProc );

    sciOriginalWndProc1 = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getSciMainWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );

    sciOriginalWndProc2 = (WNDPROC) pSetWindowLongPtr(
        m_nppMsgr.getSciSecondWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );
}

void CXBrackets::OnNppShutdown()
{
    auto pSetWindowLongPtr = isNppWndUnicode ? SetWindowLongPtrW : SetWindowLongPtrA;

    if ( nppOriginalWndProc )
    {
        pSetWindowLongPtr( m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppOriginalWndProc );
    }

    if ( sciOriginalWndProc1 )
    {
        pSetWindowLongPtr( m_nppMsgr.getSciMainWnd(), GWLP_WNDPROC, (LONG_PTR) sciOriginalWndProc1 );
    }

    if ( sciOriginalWndProc2 )
    {
        pSetWindowLongPtr( m_nppMsgr.getSciSecondWnd(), GWLP_WNDPROC, (LONG_PTR) sciOriginalWndProc2 );
    }

    SaveOptions();
}

void CXBrackets::OnNppMacro(int nMacroState)
{
    static int nPrevAutoComplete = -1; // uninitialized

    switch ( nMacroState )
    {
        case MACRO_START:
            isNppMacroStarted = true;
            break;
        case MACRO_STOP:
            isNppMacroStarted = false;
            break;
        default:
            isNppMacroStarted = !isNppMacroStarted;
            break;
    }

    if ( isNppMacroStarted )
    {
        nPrevAutoComplete = g_opt.getBracketsAutoComplete() ? 1 : 0;
        g_opt.setBracketsAutoComplete(false);
    }
    else
    {
        if ( nPrevAutoComplete < 0 ) // initialize now
            nPrevAutoComplete = g_opt.getBracketsAutoComplete() ? 1 : 0;

        g_opt.setBracketsAutoComplete(nPrevAutoComplete > 0);
        UpdateFileType();
    }

    CXBracketsMenu::AllowAutocomplete(!isNppMacroStarted);
}

bool CXBrackets::isDoubleQuoteSupported() const
{
    return g_opt.getBracketsDoDoubleQuote();
}

bool CXBrackets::isSingleQuoteSupported() const
{
    return ( g_opt.getBracketsDoSingleQuote() &&
             ((m_uFileType & tfmSingleQuote) != 0 || !g_opt.getBracketsDoSingleQuoteIf()) );
}

bool CXBrackets::isTagSupported() const
{
    return ( g_opt.getBracketsDoTag() &&
             ((m_uFileType & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf()) );
}

bool CXBrackets::isTag2Supported() const
{
    return ( g_opt.getBracketsDoTag2() &&
             ((m_uFileType & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf()) );
}

bool CXBrackets::isSkipEscapedSupported() const
{
    return ( g_opt.getBracketsSkipEscaped() && (m_uFileType & tfmEscaped1) != 0 );
}

CXBrackets::TBracketType CXBrackets::getLeftBracketType(const int ch, unsigned int uOptions) const
{
    TBracketType nLeftBracketType = tbtNone;

    // OK for both ANSI and Unicode (ch can be wide character)
    switch ( ch )
    {
    case _TCH('(') :
        nLeftBracketType = tbtBracket;
        break;
    case _TCH('[') :
        nLeftBracketType = tbtSquare;
        break;
    case _TCH('{') :
        nLeftBracketType = tbtBrace;
        break;
    case _TCH('\"') :
        if ( (uOptions & bofIgnoreMode) != 0 || isDoubleQuoteSupported() )
        {
            nLeftBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 || isSingleQuoteSupported() )
        {
            nLeftBracketType = tbtSglQuote;
        }
        break;
    case _TCH('<') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTagSupported() )
        {
            nLeftBracketType = tbtTag;
        }
        break;
    }

    return nLeftBracketType;
}

CXBrackets::TBracketType CXBrackets::getRightBracketType(const int ch, unsigned int uOptions) const
{
    TBracketType nRightBracketType = tbtNone;

    // OK for both ANSI and Unicode (ch can be wide character)
    switch ( ch )
    {
    case _TCH(')') :
        nRightBracketType = tbtBracket;
        break;
    case _TCH(']') :
        nRightBracketType = tbtSquare;
        break;
    case _TCH('}') :
        nRightBracketType = tbtBrace;
        break;
    case _TCH('\"') :
        if ( (uOptions & bofIgnoreMode) != 0 || isDoubleQuoteSupported() )
        {
            nRightBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 || isSingleQuoteSupported() )
        {
            nRightBracketType = tbtSglQuote;
        }
        break;
    case _TCH('>') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTagSupported() )
        {
            nRightBracketType = tbtTag;
        }
        // no break here
    case _TCH('/') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTag2Supported() )
        {
            nRightBracketType = tbtTag2;
        }
        break;
    }

    return nRightBracketType;
}

int CXBrackets::getDirectionIndex(const eDupPairDirection direction)
{
    int i = 0;

    switch (direction)
    {
        case DP_BACKWARD:
            i = 0;
            break;
        case DP_MAYBEBACKWARD:
            i = 1;
            break;
        case DP_DETECT:
            i = 2;
            break;
        case DP_MAYBEFORWARD:
            i = 3;
            break;
        case DP_FORWARD:
            i = 4;
            break;
    }

    return i;
}

int CXBrackets::getDirectionRank(const eDupPairDirection leftDirection, const eDupPairDirection rightDirection)
{
    //  The *exact* numbers in the table below have no special meaning.
    //  The only important thing is relative comparison of the numbers:
    //  which of them are greater than others, and which of them are equal.
    //
    // --> leftDirection --> DP_BACKWARD  DP_MAYBEBACKWARD   DP_DETECT      DP_MAYBEFORWARD   DP_FORWARD
    //    DP_FORWARD         <<( )>>  0   <?( )>>   1        ?(? )>>   2    (?> )>>   3       (>> )>>   4
    //    DP_MAYBEFORWARD    <<( )?>  1   <?( )?>   5        ?(? )?>   9    (?> )?>  13       (>> )?>  17
    //    DP_DETECT          <<( ?)?  2   <?( ?)?   9        ?(? ?)?  16    (?> ?)?  23       (>> ?)?  30
    //    DP_MAYBEBACKWARD   <<( <?)  3   <?( <?)  13        ?(? <?)  23    (?> <?)  33       (>> <?)  43
    //    DP_BACWARD         <<( <<)  4   <?( <<)  17        ?(? <<)  30    (?> <<)  43       (>> <<)  56
    // ^ rightDirection ^
    static const int Ranking[5][5] = {
        { 4, 17, 30, 43, 56 }, // DP_BACKWARD
        { 3, 13, 23, 33, 43 }, // DP_MAYBEBACKWARD
        { 2,  9, 16, 23, 30 }, // DP_DETECT
        { 1,  5,  9, 13, 17 }, // DP_MAYBEFORWARD
        { 0,  1,  2,  3,  4 }  // DP_FORWARD
    };

    int leftIndex = getDirectionIndex(leftDirection);
    int rightIndex = getDirectionIndex(rightDirection);

    return Ranking[rightIndex][leftIndex];
}

bool CXBrackets::isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const
{
    if ( !isSkipEscapedSupported() )
        return false;

    char szPrefix[MAX_ESCAPED_PREFIX + 2];
    Sci_Position pos;
    int len; // len <= MAX_ESCAPED_PREFIX, so 'int' is always enough

    getEscapedPrefixPos(nCharPos, &pos, &len);
    len = static_cast<int>(sciMsgr.getTextRange(pos, pos + len, szPrefix));
    return isEscapedPrefix(szPrefix, len);
}

bool CXBrackets::isDuplicatedPair(TBracketType nBracketType) const
{
    return (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote);
}

CXBrackets::eDupPairDirection CXBrackets::getDuplicatedPairDirection(const CSciMessager& sciMsgr, const Sci_Position nCharPos, const char curr_ch) const
{
    static HWND hLocalEditWnd = NULL;
    static char szWordDelimiters[] = " \t\n'`\"\\|[](){}<>,.;:+-=~!@#$%^&*/?";

    if ( nCharPos <= 0 )
        return DP_FORWARD; // no char before, search forward

    const char prev_ch = sciMsgr.getCharAt(nCharPos - 1);
    if ( prev_ch == curr_ch )
        return DP_BACKWARD; // quoted text, search backward

    int nStyles[4] = {
        -1,  // [0]  nCharPos - 1
        -1,  // [1]  nCharPos
        -1,  // [2]  nCharPos + 1
        -1   // [3]  nCharPos + 2
    };

    if ( nCharPos > 0 )
    {
        nStyles[0] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos - 1);
        nStyles[1] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos);
        nStyles[2] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos + 1);

        if ( nStyles[2] == nStyles[1] && nStyles[1] != nStyles[0] )
            return DP_FORWARD;

        if ( nStyles[0] == nStyles[1] && nStyles[1] != nStyles[2] )
            return DP_BACKWARD;
    }

    const Sci_Position nTextLen = sciMsgr.getTextLength();
    if ( nTextLen <= nCharPos + 1 )
        return DP_BACKWARD; // end of the file

    const char next_ch = sciMsgr.getCharAt(nCharPos + 1);
    if ( next_ch == curr_ch )
        return DP_FORWARD; //  quoted text, search forward

    if ( nTextLen > nCharPos + 2 )
    {
        if ( nStyles[1] == -1 )
            nStyles[1] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos);
        if ( nStyles[2] == -1 )
            nStyles[2] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos + 1);
        nStyles[3] = sciMsgr.SendSciMsg(SCI_GETSTYLEINDEXAT, nCharPos + 2);

        if ( nStyles[3] == nStyles[2] && nStyles[2] != nStyles[1] )
            return DP_FORWARD;

        if ( nStyles[1] == nStyles[2] && nStyles[2] != nStyles[3] )
            return DP_BACKWARD;
    }

    if ( prev_ch == '\r' || prev_ch == '\n' )
    {
        // previous char is EOL, search forward
        return isSepOrOneOf(next_ch, szWordDelimiters) ? DP_MAYBEFORWARD : DP_FORWARD;
    }

    if ( next_ch == '\r' || next_ch == '\n')
    {
        // next char is EOL, search backward
        return isSepOrOneOf(prev_ch, szWordDelimiters) ? DP_MAYBEBACKWARD : DP_BACKWARD;
    }

    if ( isSepOrOneOf(prev_ch, szWordDelimiters) )
    {
        // previous char is a separator
        if ( !isSepOrOneOf(next_ch, szWordDelimiters) )
            return DP_FORWARD; // next char is not a separator, search forward

        if ( isSentenceEndChar(prev_ch) )
        {
            if ( !isSentenceEndChar(next_ch) ) // e.g.  ."-
                return DP_MAYBEBACKWARD;
        }
    }
    else if ( isSepOrOneOf(next_ch, szWordDelimiters) )
    {
        // next char is a separator
        if ( !isSepOrOneOf(prev_ch, szWordDelimiters) )
            return DP_BACKWARD; // previous char is not a separator, search backward

        if ( isSentenceEndChar(prev_ch) )
        {
            if ( !isSentenceEndChar(next_ch) ) // e.g.  ."-
                return DP_MAYBEBACKWARD;
        }
    }

    return DP_DETECT;
}

unsigned int CXBrackets::isAtBracketCharacter(const CSciMessager& sciMsgr, const Sci_Position nCharPos, TBracketType* out_nBrType, eDupPairDirection* out_nDupDirection) const
{
    TBracketType nBrType = tbtNone;
    eDupPairDirection nDupDirCurr = DP_NONE;
    eDupPairDirection nDupDirPrev = DP_NONE;
    TBracketType nDetectBrType = tbtNone;
    unsigned int nDetectBrAtPos = abcNone;

    *out_nBrType = tbtNone;
    *out_nDupDirection = DP_NONE;

    const char prev_ch = (nCharPos != 0) ? sciMsgr.getCharAt(nCharPos - 1) : 0;
    const char curr_ch = sciMsgr.getCharAt(nCharPos);

    if ( prev_ch != 0 )
    {
        nBrType = getLeftBracketType(prev_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos - 1) ) //  (|
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                nDupDirPrev = getDuplicatedPairDirection(sciMsgr, nCharPos - 1, prev_ch);
                if ( nDupDirPrev == DP_FORWARD || nDupDirPrev == DP_MAYBEFORWARD )
                {
                    // left quote
                    *out_nDupDirection = nDupDirPrev;
                }
                else if ( nDupDirPrev == DP_DETECT )
                {
                    nDetectBrAtPos = abcBrIsOnLeft;
                    nDetectBrType = nBrType;
                    nBrType = tbtNone;
                }
                else
                    nBrType = tbtNone;
            }
            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcLeftBr | abcBrIsOnLeft);
            }
        }
    }

    if ( curr_ch != 0 )
    {
        nBrType = getLeftBracketType(curr_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos) ) //  |(
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                nDupDirCurr = getDuplicatedPairDirection(sciMsgr, nCharPos, curr_ch);
                if ( nDupDirCurr == DP_FORWARD || nDupDirCurr == DP_MAYBEFORWARD )
                {
                    // left quote
                    *out_nDupDirection = nDupDirCurr;
                }
                else if ( nDupDirCurr == DP_DETECT )
                {
                    nDetectBrAtPos = abcBrIsOnRight;
                    nDetectBrType = nBrType;
                    nBrType = tbtNone;
                }
                else
                    nBrType = tbtNone;
            }
            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcLeftBr | abcBrIsOnRight);
            }
        }
    }

    if ( curr_ch != 0 )
    {
        nBrType = getRightBracketType(curr_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos) ) //  |)
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                if ( nDupDirCurr == DP_NONE )
                    nDupDirCurr = getDuplicatedPairDirection(sciMsgr, nCharPos, curr_ch);

                if ( nDupDirCurr == DP_BACKWARD || nDupDirCurr == DP_MAYBEBACKWARD )
                    *out_nDupDirection = nDupDirCurr; // right quote
                else
                    nBrType = tbtNone;
            }

            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcRightBr | abcBrIsOnRight);
            }
        }
    }

    if ( prev_ch != 0 )
    {
        nBrType = getRightBracketType(prev_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos - 1) ) //  )|
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                if ( nDupDirPrev == DP_NONE )
                    nDupDirPrev = getDuplicatedPairDirection(sciMsgr, nCharPos - 1, prev_ch);

                if ( nDupDirPrev == DP_BACKWARD || nDupDirPrev == DP_MAYBEBACKWARD )
                    *out_nDupDirection = nDupDirPrev; // right quote
                else
                    nBrType = tbtNone;
            }

            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcRightBr | abcBrIsOnLeft);
            }
        }
    }

    if ( nDetectBrType != tbtNone )
    {
        *out_nBrType = nDetectBrType;
        *out_nDupDirection = DP_DETECT;
        return (abcDetectBr | nDetectBrAtPos);
    }

    return (abcNone);
}

bool CXBrackets::findLeftBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state)
{
    switch ( state->nRightBrType )
    {
        case tbtBracket:
        case tbtSquare:
        case tbtBrace:
        case tbtTag:
        {
            Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nStartPos));
            if ( nMatchBrPos != -1 )
            {
                state->nLeftBrType = state->nRightBrType;
                state->nLeftBrPos = nMatchBrPos + 1;
                state->nLeftDupDirection = DP_NONE;
                return true; // rely on Scintilla
            }
        }
        break;
    }

    std::stack<std::pair<TBracketType, eDupPairDirection>> bracketsStack;
    std::vector<char> vLine;

    const Sci_Position nStartLine = sciMsgr.getLineFromPosition(nStartPos);

    for ( Sci_Position nLine = nStartLine; ; --nLine )
    {
        Sci_Position nLineStartPos = sciMsgr.getPositionFromLine(nLine);
        Sci_Position nLineLen = sciMsgr.getLine(nLine, nullptr);

        vLine.resize(nLineLen + 1);
        nLineLen = sciMsgr.getLine(nLine, vLine.data());

        Sci_Position i = (nLine != nStartLine) ? nLineLen : (nStartPos - nLineStartPos);
        if ( i == 0 )
            continue;

        for ( --i; ; --i )
        {
            const char ch = vLine[i];
            TBracketType nBrType = getLeftBracketType(ch);
            if ( nBrType != tbtNone )
            {
                Sci_Position nBrPos = nLineStartPos + i;
                if ( isEscapedPos(sciMsgr, nBrPos) )
                {
                    nBrType = tbtNone;
                }
                else
                {
                    if ( isDuplicatedPair(nBrType) )
                    {
                        eDupPairDirection nDupPairDirection = getDuplicatedPairDirection(sciMsgr, nBrPos, ch);
                        if ( nDupPairDirection == DP_FORWARD || nDupPairDirection == DP_MAYBEFORWARD )
                        {
                            // left quote found
                            if ( bracketsStack.empty() )
                            {
                                state->nLeftBrType = nBrType;
                                state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                                state->nLeftDupDirection = nDupPairDirection;
                                break; // OK, found
                            }
                            else if ( bracketsStack.top().first == nBrType )
                            {
                                if ( getDirectionRank(nDupPairDirection, bracketsStack.top().second) >= getDirectionRank(DP_FORWARD, DP_DETECT) )
                                    bracketsStack.pop();
                                else
                                    break; // can't be sure if it is a pair of quotes
                            }
                            else
                            {
                                // could be either a left quote inside (another) quotes
                                // or a left quote outside (another) quotes, not sure...
                                break;
                            }
                        }
                        else if ( nDupPairDirection == DP_DETECT )
                        {
                            if ( bracketsStack.empty() )
                            {
                                if ( state->nRightBrType == nBrType &&
                                     state->nRightDupDirection == DP_BACKWARD )
                                {
                                    state->nLeftBrType = nBrType;
                                    state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                                    state->nLeftDupDirection = nDupPairDirection;
                                    break; // OK, found
                                }
                                else
                                {
                                    // treating it as a right quote
                                    bracketsStack.push({nBrType, nDupPairDirection});
                                }
                            }
                            else if ( bracketsStack.top().first == nBrType )
                            {
                                if ( bracketsStack.top().second == DP_BACKWARD )
                                    bracketsStack.pop();
                                else
                                    break; // can't be sure if it's a pair of quotes or not
                            }
                            else if ( state->nRightBrType == nBrType &&
                                      state->nRightDupDirection == DP_BACKWARD )
                            {
                                // could be either a left bracket inside quotes
                                // or a left bracket outside quotes, not sure...
                                break;
                            }
                            else
                            {
                                // treating it as a right quote
                                bracketsStack.push({nBrType, nDupPairDirection});
                            }
                        }
                        else
                        {
                            // right quote found while looking for a left one
                            bracketsStack.push({nBrType, nDupPairDirection});
                        }
                    }
                    else
                    {
                        if ( bracketsStack.empty() )
                        {
                            state->nLeftBrType = nBrType;
                            state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                            state->nLeftDupDirection = DP_NONE;
                            break; // OK, found
                        }
                        else if ( bracketsStack.top().first == nBrType )
                        {
                            bracketsStack.pop();
                        }
                        else
                        {
                            // could be either a left bracket inside quotes
                            // or a left bracket outside quotes, not sure...
                            break;
                        }
                    }
                }
            }
            else
            {
                nBrType = getRightBracketType(ch);
                if ( nBrType != tbtNone )
                {
                    Sci_Position nBrPos = nLineStartPos + i;
                    if ( !isEscapedPos(sciMsgr, nBrPos) )
                    {
                        // found a right bracket instead of a left bracket
                        // this can't be a duplicated pair since they have been handled above

                        // TODO: use SCI_BRACEMATCH
                        bracketsStack.push({nBrType, DP_NONE});
                    }
                }
            }

            if ( i == 0 )
                break;
        }

        if ( nLine == 0 || state->nLeftBrType != tbtNone )
            break;
    }

    return (state->nLeftBrType != tbtNone);
}

bool CXBrackets::findRightBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state)
{
    switch ( state->nLeftBrType )
    {
        case tbtBracket:
        case tbtSquare:
        case tbtBrace:
        case tbtTag:
        {
            Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nStartPos - 1));
            if ( nMatchBrPos != -1 )
            {
                state->nRightBrType = state->nLeftBrType;
                state->nRightBrPos = nMatchBrPos;
                state->nRightDupDirection = DP_NONE;
                return true; // rely on Scintilla
            }
        }
        break;
    }

    std::stack<std::pair<TBracketType, eDupPairDirection>> bracketsStack;
    std::vector<char> vLine;

    const Sci_Position nLinesCount = sciMsgr.getLineCount();
    const Sci_Position nStartLine = sciMsgr.getLineFromPosition(nStartPos);

    for ( Sci_Position nLine = nStartLine; nLine < nLinesCount; ++nLine )
    {
        Sci_Position nLineStartPos = sciMsgr.getPositionFromLine(nLine);
        Sci_Position nLineLen = sciMsgr.getLine(nLine, nullptr);

        vLine.resize(nLineLen + 1);
        nLineLen = sciMsgr.getLine(nLine, vLine.data());

        for ( Sci_Position i = (nLine != nStartLine) ? 0 : (nStartPos - nLineStartPos); i < nLineLen; ++i )
        {
            const char ch = vLine[i];
            TBracketType nBrType = getRightBracketType(ch);
            if ( nBrType != tbtNone )
            {
                Sci_Position nBrPos = nLineStartPos + i;
                if ( isEscapedPos(sciMsgr, nBrPos) )
                {
                    nBrType = tbtNone;
                }
                else
                {
                    if ( isDuplicatedPair(nBrType) )
                    {
                        eDupPairDirection nDupPairDirection = getDuplicatedPairDirection(sciMsgr, nBrPos, ch);
                        if ( nDupPairDirection == DP_BACKWARD || nDupPairDirection == DP_MAYBEBACKWARD )
                        {
                            // right quote found
                            if ( bracketsStack.empty() )
                            {
                                state->nRightBrType = nBrType;
                                state->nRightBrPos = nBrPos;
                                state->nRightDupDirection = nDupPairDirection;
                                break; // OK, found
                            }
                            else if ( bracketsStack.top().first == nBrType )
                            {
                                if ( getDirectionRank(bracketsStack.top().second, nDupPairDirection) >= getDirectionRank(DP_DETECT, DP_BACKWARD) )
                                    bracketsStack.pop();
                                else
                                    break; // can't be sure if it is a pair of quotes
                            }
                            else
                            {
                                // could be either a right quote inside (another) quotes
                                // or a right quote outside (another) quotes, not sure...
                                break;
                            }
                        }
                        else if ( nDupPairDirection == DP_DETECT )
                        {
                            if ( bracketsStack.empty() )
                            {
                                if ( state->nLeftBrType == nBrType &&
                                     state->nLeftDupDirection == DP_FORWARD )
                                {
                                    state->nRightBrType = nBrType;
                                    state->nRightBrPos = nBrPos;
                                    state->nRightDupDirection = nDupPairDirection;
                                    break; // OK, found
                                }
                                else
                                {
                                    // treating it as a left quote
                                    bracketsStack.push({nBrType, nDupPairDirection});
                                }
                            }
                            else if ( bracketsStack.top().first == nBrType )
                            {
                                if ( bracketsStack.top().second == DP_FORWARD )
                                    bracketsStack.pop();
                                else
                                    break; // can't be sure if it's a pair of quotes or not
                            }
                            else if ( state->nLeftBrType == nBrType &&
                                      state->nLeftDupDirection == DP_FORWARD )
                            {
                                // could be either a right quote inside (another) quotes
                                // or a right quote outside (another) quotes, not sure...
                                break;
                            }
                            else
                            {
                                // treating it as a left quote
                                bracketsStack.push({nBrType, nDupPairDirection});
                            }
                        }
                        else
                        {
                            // left quote found while looking for a right one
                            bracketsStack.push({nBrType, nDupPairDirection});
                        }
                    }
                    else
                    {
                        if ( bracketsStack.empty() )
                        {
                            state->nRightBrType = nBrType;
                            state->nRightBrPos = nBrPos;
                            state->nRightDupDirection = DP_NONE;
                            break; // OK, found
                        }
                        else if ( bracketsStack.top().first == nBrType )
                        {
                            bracketsStack.pop();
                        }
                        else
                        {
                            // could be either a right bracket inside quotes
                            // or a right bracket outside quotes, not sure...
                            break;
                        }
                    }
                }
            }
            else
            {
                nBrType = getLeftBracketType(ch);
                if ( nBrType != tbtNone )
                {
                    Sci_Position nBrPos = nLineStartPos + i;
                    if ( !isEscapedPos(sciMsgr, nBrPos) )
                    {
                        // found a left bracket instead of a right bracket
                        // this can't be a duplicated pair since they have been handled above

                        // TODO: use SCI_BRACEMATCH
                        bracketsStack.push({nBrType, DP_NONE});
                    }
                }
            }
        }

        if ( state->nRightBrType != tbtNone )
            break;
    }

    return (state->nRightBrType != tbtNone);
}

CXBrackets::eCharProcessingResult CXBrackets::OnSciChar(const int ch)
{
    if ( !g_opt.getBracketsAutoComplete() )
        return cprNone;

    if ( (m_uFileType & tfmIsSupported) == 0 )
        return cprNone;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return cprNone; // nothing to do with multiple selections

    if ( m_nAutoRightBracketPos >= 0 )
    {
        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        TBracketType nRightBracketType = getRightBracketType(ch);

        if ( nRightBracketType != tbtNone )
        {
            Sci_Position pos = sciMsgr.getCurrentPos();

            if ( pos == m_nAutoRightBracketPos )
            {
                // previous character
                char prev_ch = sciMsgr.getCharAt(pos - 1);
                if ( prev_ch == strBrackets[nRightBracketType][0] )
                {
                    char next_ch = sciMsgr.getCharAt(pos);
                    if ( next_ch == strBrackets[nRightBracketType][1] )
                    {
                        ++pos;
                        if ( nRightBracketType == tbtTag2 )
                            ++pos;
                        sciMsgr.setSel(pos, pos);

                        m_nAutoRightBracketPos = -1;
                        return cprBrAutoCompl;
                    }
                }
            }
        }
    }

    m_nAutoRightBracketPos = -1;

    TBracketType nLeftBracketType = getLeftBracketType(ch);
    if ( nLeftBracketType == tbtNone )
        return cprNone;

    // a typed character is a bracket
    return AutoBracketsFunc(nLeftBracketType);
}

void CXBrackets::performBracketsAction(eGetBracketsAction nBrAction)
{
    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return; // multiple selections

    tGetBracketsState state;
    state.nSelStart = sciMsgr.getSelectionStart();
    state.nSelEnd = sciMsgr.getSelectionEnd();
    if ( state.nSelStart != state.nSelEnd )
    {
        // something is selected
        if ( nBrAction == baSelToMatching || nBrAction == baSelToNearest )
        {
            // inverting the selection positions
            if ( sciMsgr.getCurrentPos() == state.nSelEnd )
                sciMsgr.setSel(state.nSelEnd, state.nSelStart);
            else
                sciMsgr.setSel(state.nSelStart, state.nSelEnd);
        }
        return;
    }

    state.nCharPos = state.nSelStart;

    Sci_Position nStartPos = state.nCharPos;
    TBracketType nBrType = tbtNone;
    eDupPairDirection nDupDir = DP_NONE;
    bool isBrPairFound = false;
    unsigned int nAtBr = isAtBracketCharacter(sciMsgr, nStartPos, &nBrType, &nDupDir);

    if ( nBrType == tbtTag2 )
        nBrType = tbtTag;

    if ( nAtBr & abcLeftBr )
    {
        // at left bracket...
        if ( nAtBr & abcBrIsOnRight )
            ++nStartPos; //  |(  ->  (|

        state.nLeftBrPos = nStartPos; // exclude the bracket itself
        state.nLeftBrType = nBrType;
        state.nLeftDupDirection = nDupDir;

        if ( findRightBracket(sciMsgr, nStartPos, &state) && nBrType == state.nRightBrType )
        {
            if ( nAtBr & abcBrIsOnRight )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
    }
    else if ( nAtBr & abcRightBr )
    {
        // at right bracket...
        if ( nAtBr & abcBrIsOnLeft )
            --nStartPos; //  )|  ->  |)

        state.nRightBrPos = nStartPos;
        state.nRightBrType = nBrType;
        state.nRightDupDirection = nDupDir;

        if ( findLeftBracket(sciMsgr, nStartPos, &state) && nBrType == state.nLeftBrType )
        {
            if ( nAtBr & abcBrIsOnLeft )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
    }
    else if ( nAtBr & abcDetectBr )
    {
        // detect: either at left or at right quote
        unsigned int nAtBrNow = nAtBr;

        // 1. try as a left quote...
        if ( nAtBr & abcBrIsOnRight )
        {
            ++nStartPos; //  |(  ->  (|
            nAtBrNow = abcDetectBr | abcBrIsOnLeft;
        }

        state.nLeftBrPos = nStartPos; // exclude the quote itself
        state.nLeftBrType = nBrType;
        state.nLeftDupDirection = nDupDir;

        if ( findRightBracket(sciMsgr, nStartPos, &state) && nBrType == state.nRightBrType )
        {
            if ( nAtBr & abcBrIsOnRight )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
        else
        {
            state.nLeftBrPos = -1;
            state.nLeftBrType = tbtNone;
            state.nLeftDupDirection = DP_NONE;

            // 2. try as a right quote...
            nAtBrNow = nAtBr;
            if ( (nAtBr & abcBrIsOnLeft) || (nAtBr & abcBrIsOnRight) )
            {
                --nStartPos; //  )|  ->  |)
                nAtBrNow = abcDetectBr | abcBrIsOnRight;
            }

            state.nRightBrPos = nStartPos;
            state.nRightBrType = nBrType;
            state.nRightDupDirection = nDupDir;

            if ( findLeftBracket(sciMsgr, nStartPos, &state) && nBrType == state.nLeftBrType )
            {
                if ( nAtBr & abcBrIsOnLeft )
                {
                    ++state.nRightBrPos; //  |)  ->  )|
                    --state.nLeftBrPos;  //  (|  ->  |(
                }

                isBrPairFound = true;
            }
        }
    }
    else
    {
        // not at a bracket

        // TODO
    }

    if ( isBrPairFound )
    {
        if ( nBrAction == baGoToMatching || nBrAction == baGoToNearest )
        {
            if ( state.nSelStart == state.nLeftBrPos )
                sciMsgr.setSel(state.nRightBrPos, state.nRightBrPos);
            else
                sciMsgr.setSel(state.nLeftBrPos, state.nLeftBrPos);
        }
        else
        {
            if ( state.nSelStart == state.nLeftBrPos )
                sciMsgr.setSel(state.nLeftBrPos, state.nRightBrPos);
            else
                sciMsgr.setSel(state.nRightBrPos, state.nLeftBrPos);
        }
    }
}

void CXBrackets::GoToMatchingBracket()
{
    performBracketsAction(baGoToMatching);
}

void CXBrackets::GoToNearestBracket()
{
    performBracketsAction(baGoToNearest);
}

void CXBrackets::SelToMatchingBracket()
{
    performBracketsAction(baSelToMatching);
}

void CXBrackets::SelToNearestBrackets()
{
    performBracketsAction(baSelToNearest);
}

void CXBrackets::ReadOptions()
{
    TCHAR szPath[2*MAX_PATH + 1];

    m_nppMsgr.getPluginsConfigDir(2*MAX_PATH, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, m_szIniFileName);

    g_opt.ReadOptions(szPath);
}

void CXBrackets::SaveOptions()
{
    if ( g_opt.MustBeSaved() )
    {
        TCHAR szPath[2*MAX_PATH + 1];

        m_nppMsgr.getPluginsConfigDir(2*MAX_PATH, szPath);
        lstrcat(szPath, _T("\\"));
        lstrcat(szPath, m_szIniFileName);

        g_opt.SaveOptions(szPath);
    }
}

void CXBrackets::UpdateFileType() // <-- call it when the plugin becomes active!!!
{
    m_nAutoRightBracketPos = -1;
    m_uFileType = getFileType();
}

CXBrackets::eCharProcessingResult CXBrackets::AutoBracketsFunc(TBracketType nBracketType)
{
    if ( SelAutoBrFunc(nBracketType) )
        return cprSelAutoCompl;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();

    // is something selected?
    if ( nEditEndPos != nEditPos )
    {
        const int nSelectionMode = sciMsgr.getSelectionMode();
        if ( nSelectionMode == SC_SEL_RECTANGLE || nSelectionMode == SC_SEL_THIN )
            return cprNone;

        // removing selection
        sciMsgr.replaceSelText("");
    }

    // Theory:
    // - The character just pressed is a standard bracket symbol
    // - Thus, this character takes 1 byte in both ANSI and UTF-8
    // - Thus, we check next (and previous) byte in Scintilla to
    //   determine whether the bracket autocompletion is allowed
    // - In general, previous byte can be a trailing byte of one
    //   multi-byte UTF-8 character, but we just ignore this :)
    //   (it is safe because non-leading byte in UTF-8
    //    is always >= 0x80 whereas the character codes
    //    of standard Latin symbols are < 0x80)

    bool  bPrevCharOK = true;
    bool  bNextCharOK = false;
    char  next_ch = sciMsgr.getCharAt(nEditPos);

    if ( isWhiteSpaceOrNulChar(next_ch) ||
         g_opt.getNextCharOK().find(static_cast<TCHAR>(next_ch)) != tstr::npos )
    {
        bNextCharOK = true;
    }

    TBracketType nRightBracketType = getRightBracketType(next_ch, bofIgnoreMode);
    if ( nRightBracketType != tbtNone )
    {
        if ( nRightBracketType == tbtTag2 )
            nRightBracketType = tbtTag;

        bNextCharOK = (nRightBracketType != nBracketType || g_opt.getBracketsRightExistsOK());
    }

    if ( bNextCharOK && (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote) )
    {
        bPrevCharOK = false;

        // previous character
        char prev_ch = (nEditPos >= 1) ? sciMsgr.getCharAt(nEditPos - 1) : 0;

        if ( isWhiteSpaceOrNulChar(prev_ch) ||
             g_opt.getPrevCharOK().find(static_cast<TCHAR>(prev_ch)) != tstr::npos )
        {
            bPrevCharOK = true;
        }
    }

    if ( bPrevCharOK && bNextCharOK )
    {
        if ( isEscapedPos(sciMsgr, nEditPos) )
            bPrevCharOK = false;
    }

    if ( bPrevCharOK && bNextCharOK )
    {
        if ( nBracketType == tbtTag )
        {
            if ( g_opt.getBracketsDoTag2() )
                nBracketType = tbtTag2;
        }

        sciMsgr.beginUndoAction();
        // inserting the brackets pair
        sciMsgr.replaceSelText(strBrackets[nBracketType]);
        // placing the caret between brackets
        ++nEditPos;
        sciMsgr.setSel(nEditPos, nEditPos);
        sciMsgr.endUndoAction();

        m_nAutoRightBracketPos = nEditPos;
        return cprBrAutoCompl;
    }

    return cprNone;
}

bool CXBrackets::isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, TBracketType* pnBracketType, bool bInSelection)
{
    if ( pszTextLeft == pszTextRight )
        return false;

    TBracketType nBrType = *pnBracketType;
    const char* cszBrPair = strBrackets[nBrType];

    if ( pszTextLeft[0] != cszBrPair[0] )
        return false;

    bool bRet = false;
    if ( (pszTextRight[0] == cszBrPair[1]) &&
         (cszBrPair[2] == 0 || pszTextRight[1] == cszBrPair[2]) )
    {
        bRet = true;
        if ( nBrType != tbtTag )
            return bRet;
    }

    if ( nBrType == tbtTag )
    {
        nBrType = tbtTag2;
        if ( bInSelection )
            --pszTextRight; // '/' is present in "/>"
    }
    else if ( nBrType == tbtTag2 )
    {
        nBrType = tbtTag;
        if ( bInSelection )
            ++pszTextRight; // '/' is not present in ">"
    }
    else
        return bRet;

    // Note: both tbtTag and tbtTag2 start with '<'
    cszBrPair = strBrackets[nBrType];
    if ( (pszTextRight[0] == cszBrPair[1]) &&
         (cszBrPair[2] == 0 || pszTextRight[1] == cszBrPair[2]) )
    {
        *pnBracketType = nBrType;
        bRet = true;
    }

    return bRet;
}

bool CXBrackets::SelAutoBrFunc(TBracketType nBracketType)
{
    const UINT uSelAutoBr = g_opt.getBracketsSelAutoBr();
    if ( uSelAutoBr == CXBracketsOptions::sabNone )
        return false; // nothing to do - not processed

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return false; // nothing to do with multiple selections - not processed

    const int nSelectionMode = sciMsgr.getSelectionMode();
    if ( nSelectionMode == SC_SEL_RECTANGLE || nSelectionMode == SC_SEL_THIN )
        return false; // rectangle selection - not processed

    const Sci_Position nEditPos = sciMsgr.getSelectionStart();
    const Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();
    if ( nEditPos == nEditEndPos )
        return false; // no selection - not processed

    const Sci_Position nSelPos = nEditPos < nEditEndPos ? nEditPos : nEditEndPos;

    if (nBracketType == tbtTag && g_opt.getBracketsDoTag2())
        nBracketType = tbtTag2;

    TBracketType nBrAltType = nBracketType;
    int nBrPairLen = lstrlenA(strBrackets[nBracketType]);

    // getting the selected text
    const Sci_Position nSelLen = sciMsgr.getSelText(nullptr);
    std::vector<char> vSelText(nSelLen + nBrPairLen + 1);
    sciMsgr.getSelText(vSelText.data() + 1); // always starting from pSelText[1]

    sciMsgr.beginUndoAction();

    if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemove ||
         uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter )
    {
        vSelText[0] = nSelPos != 0 ? sciMsgr.getCharAt(nSelPos - 1) : 0; // previous character (before the selection)
        for (int i = 0; i < nBrPairLen - 1; i++)
        {
            vSelText[nSelLen + 1 + i] = sciMsgr.getCharAt(nSelPos + nSelLen + i); // next characters (after the selection)
        }
        vSelText[nSelLen + nBrPairLen] = 0;

        if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
        {
            // already in brackets/quotes : ["text"] ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            vSelText[nSelLen - nBrPairLen + 2] = 0;
            sciMsgr.replaceSelText(vSelText.data() + 2);
            sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
        }
        else if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter &&
                  nSelPos != 0 &&
                  isEnclosedInBrackets(vSelText.data(), vSelText.data() + nSelLen + 1, &nBrAltType, false) )
        {
            // already in brackets/quotes : "[text]" ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            vSelText[nSelLen + 1] = 0;
            sciMsgr.SendSciMsg(WM_SETREDRAW, FALSE, 0);
            sciMsgr.setSel(nSelPos - 1, nSelPos + nSelLen + nBrPairLen - 1);
            sciMsgr.SendSciMsg(WM_SETREDRAW, TRUE, 0);
            sciMsgr.replaceSelText(vSelText.data() + 1);
            sciMsgr.setSel(nSelPos - 1, nSelPos + nSelLen - 1);
        }
        else
        {
            // enclose in brackets/quotes
            vSelText[0] = strBrackets[nBracketType][0];
            lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }
    else
    {
        vSelText[0] = strBrackets[nBracketType][0];

        if ( uSelAutoBr == CXBracketsOptions::sabEncloseAndSel )
        {
            if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
            {
                // already in brackets/quotes; exclude them
                if ( nBrAltType != nBracketType )
                {
                    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
                }
                vSelText[nSelLen - nBrPairLen + 2] = 0;
                sciMsgr.replaceSelText(vSelText.data() + 2);
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
            }
            else
            {
                // enclose in brackets/quotes
                lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
                sciMsgr.replaceSelText(vSelText.data());
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen + nBrPairLen);
            }
        }
        else // CXBracketsOptions::sabEnclose
        {
            lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }

    sciMsgr.endUndoAction();

    m_nAutoRightBracketPos = -1;

    return true; // processed
}

unsigned int CXBrackets::getFileType()
{
    TCHAR szExt[CXBracketsOptions::MAX_EXT];
    unsigned int uType = tfmIsSupported;

    szExt[0] = 0;
    m_nppMsgr.getCurrentFileExtPart(CXBracketsOptions::MAX_EXT - 1, szExt);

    if ( szExt[0] )
    {
        TCHAR* pszExt = szExt;

        if ( *pszExt == _T('.') )
        {
            ++pszExt;
            if ( !(*pszExt) )
                return uType;
        }

        ::CharLower(pszExt);

        if ( lstrcmp(pszExt, _T("c")) == 0 ||
             lstrcmp(pszExt, _T("cc")) == 0 ||
             lstrcmp(pszExt, _T("cpp")) == 0 ||
             lstrcmp(pszExt, _T("cxx")) == 0 ||
             lstrcmp(pszExt, _T("h")) == 0 ||
             lstrcmp(pszExt, _T("hh")) == 0 ||
             lstrcmp(pszExt, _T("hpp")) == 0 ||
             lstrcmp(pszExt, _T("hxx")) == 0 )
        {
            uType |= tfmComment1 | tfmEscaped1;
        }
        else if ( lstrcmp(pszExt, _T("pas")) == 0 )
        {
            uType |= tfmComment1;
        }
        else
        {
            if ( g_opt.IsHtmlCompatible(pszExt) )
                uType |= tfmHtmlCompatible;

            if ( g_opt.IsEscapedFileExt(pszExt) )
                uType |= tfmEscaped1;
        }

        if ( g_opt.IsSingleQuoteFileExt(pszExt) )
            uType |= tfmSingleQuote;

        if ( g_opt.IsSupportedFile(pszExt) )
            uType |= tfmIsSupported;
        else if ( (uType & tfmIsSupported) != 0 )
            uType ^= tfmIsSupported;
    }

    return uType;
}
