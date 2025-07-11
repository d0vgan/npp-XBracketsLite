#include "XBrackets.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"


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

const int nMaxBrPairLen = 3; // </>

bool CXBrackets::isNppMacroStarted = false;
bool CXBrackets::isNppWndUnicode = true;
WNDPROC CXBrackets::nppOriginalWndProc = NULL;

/*
// from "resource.h" (Notepad++)
#define ID_MACRO                       20000
#define ID_MACRO_LIMIT                 20200
#define IDCMD                          50000
#define IDC_EDIT_TOGGLEMACRORECORDING  (IDCMD + 5)
#define MACRO_USER                     (WM_USER + 4000)
#define WM_MACRODLGRUNMACRO            (MACRO_USER + 02)
*/

LRESULT CXBrackets::nppCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return (isNppWndUnicode ? CallWindowProcW : CallWindowProcA)(nppOriginalWndProc, hWnd, uMsg, wParam, lParam);
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
    else if ( uMsg == WM_NOTIFY )
    {
        const CNppMessager& nppMsgr = thePlugin.getNppMsgr();
        const NMHDR* pNmHdr = reinterpret_cast<const NMHDR *>(lParam);
        if ( pNmHdr->hwndFrom == nppMsgr.getSciMainWnd() ||
             pNmHdr->hwndFrom == nppMsgr.getSciSecondWnd() )
        {
            // notifications from Scintilla:
            if ( pNmHdr->code == SCN_CHARADDED )
            {
                thePlugin.nppBeNotified(reinterpret_cast<SCNotification *>(lParam));
                return 0; // avoid further processing by Notepad++
            }
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

CXBrackets::CXBrackets() :
  m_nAutoRightBracketPos(-1),
  m_nnFileType{ tftNone, tfmIsSupported },
  m_nSelPos(0),
  m_nSelLen(0),
  m_isProcessingSelAutoBr(false),
  m_nDelTextTimerId(0)
{
    ::InitializeCriticalSection(&m_csDelTextTimer);
}

CXBrackets::~CXBrackets()
{
    ::DeleteCriticalSection(&m_csDelTextTimer);
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

static void CALLBACK DelTextTimerProc(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
{
    thePlugin.OnDelTextTimer(idEvent);
}

void CXBrackets::OnDelTextTimer(UINT_PTR idEvent)
{
    ::EnterCriticalSection(&m_csDelTextTimer);

    if ( idEvent == m_nDelTextTimerId )
    {
        ::KillTimer(NULL, idEvent);
        m_nDelTextTimerId = 0;

        if ( !m_isProcessingSelAutoBr )
        {
            // the previous selection is no more relevant:
            m_nSelPos = 0;
            m_nSelLen = 0;
            // we don't reset m_pSelText here
        }
    }

    ::LeaveCriticalSection(&m_csDelTextTimer);
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
    else
    {
        // >>> notifications from Notepad++
        switch ( pscn->nmhdr.code )
        {
            case SCN_CHARADDED:
                OnSciCharAdded(pscn->ch);
                // the previous selection is no more relevant:
                m_nSelPos = 0;
                m_nSelLen = 0;
                m_pSelText.reset();
                break;

            case SCN_MODIFIED:
                if ( (pscn->modificationType & (SC_MOD_BEFOREDELETE | SC_PERFORMED_USER)) == (SC_MOD_BEFOREDELETE | SC_PERFORMED_USER) ) // deleting text
                {
                    if ( (pscn->modificationType & (SC_PERFORMED_UNDO | SC_PERFORMED_REDO)) == 0 ) // not undo or redo
                    {
                        if ( OnBeforeDeleteText(pscn) )
                        {
                            const int nDelTextTimerIntervalMs = 150;

                            // Selected text can be deleted either by typing any character (we are interested in that)
                            // or by pressing Delete/BackSpace button.
                            // When SCN_CHARADDED happens during nDelTextTimerIntervalMs, it means the selected text
                            // was deleted while typing a character (so SelAutoBrFunc shoud work).
                            // If SCN_CHARADDED does not happen during nDelTextTimerIntervalMs, it means the selected
                            // text was deleted by Delete/BackSpace button, and the DelTextTimer sets m_nSelPos and
                            // m_nSelLen to 0 (so SelAutoBrFunc shoud exit immediately).

                            // Side effect:
                            // If the sequence of Delete/BackSpace and then '[' or "'" is quickly pressed, taking
                            // less than nDelTextTimerIntervalMs, then the previously selected and deleted text
                            // appears within the [] or ''.

                            ::EnterCriticalSection(&m_csDelTextTimer);
                            if ( m_nDelTextTimerId == 0 )
                            {
                                m_nDelTextTimerId = ::SetTimer(NULL, 0, nDelTextTimerIntervalMs, DelTextTimerProc);
                            }
                            ::LeaveCriticalSection(&m_csDelTextTimer);
                        }
                    }
                }
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

    if ( g_opt.getBracketsSelAutoBr() != CXBracketsOptions::sabNone )
    {
        m_nppMsgr.SendNppMsg(NPPM_ADDSCNMODIFIEDFLAGS, 0, SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE);
    }

    if ( isNppWndUnicode )
    {
        nppOriginalWndProc = (WNDPROC) SetWindowLongPtrW(
          m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppNewWndProc );
    }
    else
    {
        nppOriginalWndProc = (WNDPROC) SetWindowLongPtrA(
          m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppNewWndProc );
    }
}

void CXBrackets::OnNppShutdown()
{
    if ( nppOriginalWndProc )
    {
        if ( isNppWndUnicode )
        {
            ::SetWindowLongPtrW( m_nppMsgr.getNppWnd(),
                GWLP_WNDPROC, (LONG_PTR) nppOriginalWndProc );
        }
        else
        {
            ::SetWindowLongPtrA( m_nppMsgr.getNppWnd(),
                GWLP_WNDPROC, (LONG_PTR) nppOriginalWndProc );
        }
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
        if ( (uOptions & bofIgnoreMode) != 0 ||
             g_opt.getBracketsDoDoubleQuote() )
        {
            nLeftBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 ||
             (g_opt.getBracketsDoSingleQuote() &&
              ((m_nnFileType.second & tfmSingleQuote) != 0 || !g_opt.getBracketsDoSingleQuoteIf())) )
        {
            nLeftBracketType = tbtSglQuote;
        }
        break;
    case _TCH('<') :
        if ( (uOptions & bofIgnoreMode) != 0 ||
             (g_opt.getBracketsDoTag() &&
              ((m_nnFileType.second & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf())) )
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
        if ( (uOptions & bofIgnoreMode) != 0 ||
             g_opt.getBracketsDoDoubleQuote() )
        {
            nRightBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 ||
             (g_opt.getBracketsDoSingleQuote() &&
              ((m_nnFileType.second & tfmSingleQuote) != 0 || !g_opt.getBracketsDoSingleQuoteIf())) )
        {
            nRightBracketType = tbtSglQuote;
        }
        break;
    case _TCH('>') :
        if ( (uOptions & bofIgnoreMode) != 0 || 
             (g_opt.getBracketsDoTag() &&
              ((m_nnFileType.second & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf())) )
        {
            nRightBracketType = tbtTag;
        }
        // no break here
    case _TCH('/') :
        if ( (uOptions & bofIgnoreMode) != 0 ||
             (g_opt.getBracketsDoTag2() &&
              ((m_nnFileType.second & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf())) )
        {
            nRightBracketType = tbtTag2;
        }
        break;
    }

    return nRightBracketType;
}

void CXBrackets::OnSciCharAdded(const int ch)
{
    if ( !g_opt.getBracketsAutoComplete() )
        return;

    if ( (m_nnFileType.second & tfmIsSupported) == 0 )
        return;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return; // nothing to do with multiple selections

    if ( m_nAutoRightBracketPos >= 0 )
    {
        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        int nRightBracketType = getRightBracketType(ch);

        if ( nRightBracketType != tbtNone )
        {
            Sci_Position pos = sciMsgr.getCurrentPos() - 1;

            if ( pos == m_nAutoRightBracketPos )
            {
                // previous character
                char prev_ch = sciMsgr.getCharAt(pos - 1);
                if ( prev_ch == strBrackets[nRightBracketType][0] )
                {
                    char next_ch = sciMsgr.getCharAt(pos + 1);
                    if ( next_ch == strBrackets[nRightBracketType][1] )
                    {
                        sciMsgr.beginUndoAction();
                        // remove just typed right bracket
                        sciMsgr.setSel(pos, pos + 1);
                        sciMsgr.replaceSelText("");
                        // move the caret
                        ++pos;
                        if ( nRightBracketType == tbtTag2 )
                            ++pos;
                        sciMsgr.setSel(pos, pos);
                        sciMsgr.endUndoAction();

                        m_nAutoRightBracketPos = -1;
                        return;
                    }
                }
            }
        }
    }

    m_nAutoRightBracketPos = -1;

    int nLeftBracketType = getLeftBracketType(ch);
    if ( nLeftBracketType != tbtNone )
    {
        // a typed character is a bracket
        AutoBracketsFunc(nLeftBracketType);
    }
}

bool CXBrackets::OnBeforeDeleteText(const SCNotification* pscn)
{
    (pscn);
    return PrepareSelAutoBrFunc();
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
    if ( !g_opt.getBracketsAutoComplete() )
        return;

    m_nAutoRightBracketPos = -1;
    m_nnFileType = getFileType();
}

static void getEscapedPrefixPos(const Sci_Position nOffset, Sci_Position* pnPos, int* pnLen)
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

static bool isEscapedPrefix(const char* str, int len)
{
    int k = 0;
    while ( (len > 0) && (str[--len] == '\\') )
    {
        ++k;
    }
    return (k % 2) ? true : false;
}

void CXBrackets::AutoBracketsFunc(int nBracketType)
{
    if ( SelAutoBrFunc(nBracketType) )
        return;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();

    // is something selected?
    if ( nEditEndPos != nEditPos )
    {
        if ( sciMsgr.getSelectionMode() == SC_SEL_RECTANGLE )
            return;

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

    if ( next_ch == '\x0D' ||
         next_ch == '\x0A' ||
         next_ch == '\x00' ||
         next_ch == ' '  ||
         next_ch == '\t' ||
         next_ch == '.'  ||
         next_ch == ','  ||
         next_ch == '!'  ||
         next_ch == '?'  ||
         next_ch == ':'  ||
         next_ch == ';'  ||
         next_ch == '<' )
    {
        bNextCharOK = true;
    }
    else
    {
        int nRightBracketType = getRightBracketType(next_ch, bofIgnoreMode);
        if ( nRightBracketType == tbtTag2 )
            nRightBracketType = tbtTag;

        if ( nRightBracketType != tbtNone && 
             (nRightBracketType != nBracketType || g_opt.getBracketsRightExistsOK()) )
        {
            bNextCharOK = true;
        }
    }

    if ( bNextCharOK && (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote) )
    {
        bPrevCharOK = false;

        // previous character
        char prev_ch = (nEditPos >= 2) ? sciMsgr.getCharAt(nEditPos - 2) : 0;

        if ( prev_ch == '\x0D' ||
             prev_ch == '\x0A' ||
             prev_ch == '\x00' ||
             prev_ch == ' '  ||
             prev_ch == '\t' ||
             prev_ch == '('  ||
             prev_ch == '['  ||
             prev_ch == '{'  ||
             prev_ch == '<'  ||
             prev_ch == '=' )
        {
            bPrevCharOK = true;
        }
    }

    if ( bPrevCharOK && bNextCharOK &&
         g_opt.getBracketsSkipEscaped() && (m_nnFileType.second & tfmEscaped1) != 0 )
    {
        char szPrefix[MAX_ESCAPED_PREFIX + 2];
        Sci_Position pos;
        int len; // len <= MAX_ESCAPED_PREFIX, so 'int' is always enough

        getEscapedPrefixPos(nEditPos - 1, &pos, &len);
        len = static_cast<int>(sciMsgr.getTextRange(pos, pos + len, szPrefix));
        if ( isEscapedPrefix(szPrefix, len) )
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
        // inserting the closing bracket
        sciMsgr.replaceSelText(strBrackets[nBracketType] + 1);
        // placing the caret between brackets
        sciMsgr.setSel(nEditPos, nEditPos);
        sciMsgr.endUndoAction();

        m_nAutoRightBracketPos = nEditPos;
    }

}

bool CXBrackets::isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, int* pnBracketType, bool bInSelection)
{
    int nBrType = *pnBracketType;
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

bool CXBrackets::PrepareSelAutoBrFunc()
{
    if ( m_isProcessingSelAutoBr )
        return false; // avoiding recursive calls - not processed

    // the previous selection is no more relevant:
    m_nSelPos = 0;
    m_nSelLen = 0;
    m_pSelText.reset();

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

    // getting the selected text
    const Sci_Position nSelLen = sciMsgr.getSelText(nullptr);
    m_pSelText = std::make_unique<char[]>(nSelLen + nMaxBrPairLen + 1);
    sciMsgr.getSelText(m_pSelText.get() + 1); // always starting from pSelText[1]

    if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemove ||
         uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter )
    {
        m_pSelText[0] = nEditPos != 0 ? sciMsgr.getCharAt(nEditPos - 1) : 0; // previous character (before the selection)
        for ( int i = 0; i < nMaxBrPairLen - 1; i++ )
        {
            m_pSelText[nSelLen + 1 + i] = sciMsgr.getCharAt(nEditPos + nSelLen + i); // next characters (after the selection)
        }
        m_pSelText[nSelLen + nMaxBrPairLen] = 0;
    }

    ::EnterCriticalSection(&m_csDelTextTimer);
    m_nSelPos = nSelPos;
    m_nSelLen = nSelLen;
    ::LeaveCriticalSection(&m_csDelTextTimer);

    return true;
}

bool CXBrackets::SelAutoBrFunc(int nBracketType)
{
    m_isProcessingSelAutoBr = true;

    if ( m_nSelLen == 0 || !m_pSelText )
    {
        m_isProcessingSelAutoBr = false;
        return false; // no previously selected text - not processed
    }

    if ( nBracketType == tbtTag && g_opt.getBracketsDoTag2() )
        nBracketType = tbtTag2;

    int nBrAltType = nBracketType;
    int nBrPairLen = lstrlenA(strBrackets[nBracketType]);

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    sciMsgr.beginUndoAction();

    sciMsgr.SendSciMsg(WM_SETREDRAW, FALSE, 0);
    sciMsgr.setSel(m_nSelPos, m_nSelPos + 1); // selecting the just typed bracket character
    sciMsgr.SendSciMsg(WM_SETREDRAW, TRUE, 0);

    const UINT uSelAutoBr = g_opt.getBracketsSelAutoBr();
    if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemove ||
         uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter )
    {
        if ( isEnclosedInBrackets(m_pSelText.get() + 1, m_pSelText.get() + m_nSelLen - nBrPairLen + 2, &nBrAltType, true) )
        {
            // already in brackets/quotes : ["text"] ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            m_pSelText[m_nSelLen - nBrPairLen + 2] = 0;
            sciMsgr.replaceSelText(m_pSelText.get() + 2);
            sciMsgr.setSel(m_nSelPos, m_nSelPos + m_nSelLen - nBrPairLen);
        }
        else if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter &&
                  m_nSelPos != 0 &&
                  isEnclosedInBrackets(m_pSelText.get(), m_pSelText.get() + m_nSelLen + 1, &nBrAltType, false) )
        {
            // already in brackets/quotes : "[text]" ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            m_pSelText[m_nSelLen + 1] = 0;
            sciMsgr.SendSciMsg(WM_SETREDRAW, FALSE, 0);
            sciMsgr.setSel(m_nSelPos - 1, m_nSelPos + nBrPairLen); // including the just typed bracket character
            sciMsgr.SendSciMsg(WM_SETREDRAW, TRUE, 0);
            sciMsgr.replaceSelText(m_pSelText.get() + 1);
            sciMsgr.setSel(m_nSelPos - 1, m_nSelPos + m_nSelLen - 1);
        }
        else
        {
            // enclose in brackets/quotes
            m_pSelText[0] = strBrackets[nBracketType][0];
            lstrcpyA(m_pSelText.get() + m_nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(m_pSelText.get());
            sciMsgr.setSel(m_nSelPos + 1, m_nSelPos + m_nSelLen + 1);
        }
    }
    else
    {
        m_pSelText[0] = strBrackets[nBracketType][0];

        if ( uSelAutoBr == CXBracketsOptions::sabEncloseAndSel )
        {
            if ( isEnclosedInBrackets(m_pSelText.get() + 1, m_pSelText.get() + m_nSelLen - nBrPairLen + 2, &nBrAltType, true) )
            {
                // already in brackets/quotes; exclude them
                if ( nBrAltType != nBracketType )
                {
                    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
                }
                m_pSelText[m_nSelLen - nBrPairLen + 2] = 0;
                sciMsgr.replaceSelText(m_pSelText.get() + 2);
                sciMsgr.setSel(m_nSelPos, m_nSelPos + m_nSelLen - nBrPairLen);
            }
            else
            {
                // enclose in brackets/quotes
                lstrcpyA(m_pSelText.get() + m_nSelLen + 1, strBrackets[nBracketType] + 1);
                sciMsgr.replaceSelText(m_pSelText.get());
                sciMsgr.setSel(m_nSelPos, m_nSelPos + m_nSelLen + nBrPairLen);
            }
        }
        else // CXBracketsOptions::sabEnclose
        {
            lstrcpyA(m_pSelText.get() + m_nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(m_pSelText.get());
            sciMsgr.setSel(m_nSelPos + 1, m_nSelPos + m_nSelLen + 1);
        }
    }

    sciMsgr.endUndoAction();

    m_nAutoRightBracketPos = -1;
    m_isProcessingSelAutoBr = false;

    return true; // processed
}

std::pair<CXBrackets::TFileType, unsigned short> CXBrackets::getFileType()
{
    TCHAR szExt[CXBracketsOptions::MAX_EXT];
    std::pair<TFileType, unsigned short> nnType = std::make_pair(tftNone, tfmIsSupported);

    szExt[0] = 0;
    m_nppMsgr.getCurrentFileExtPart(CXBracketsOptions::MAX_EXT - 1, szExt);

    if ( szExt[0] )
    {
        TCHAR* pszExt = szExt;

        if ( *pszExt == _T('.') )
        {
            ++pszExt;
            if ( !(*pszExt) )
                return nnType;
        }

        ::CharLower(pszExt);

        if ( lstrcmp(pszExt, _T("c")) == 0 ||
             lstrcmp(pszExt, _T("cc")) == 0 ||
             lstrcmp(pszExt, _T("cpp")) == 0 ||
             lstrcmp(pszExt, _T("cxx")) == 0 )
        {
            nnType.first = tftC_Cpp;
            nnType.second = tfmComment1 | tfmEscaped1;
        }
        else if ( lstrcmp(pszExt, _T("h")) == 0 ||
                  lstrcmp(pszExt, _T("hh")) == 0 ||
                  lstrcmp(pszExt, _T("hpp")) == 0 ||
                  lstrcmp(pszExt, _T("hxx")) == 0 )
        {
            nnType.first = tftH_Hpp;
            nnType.second = tfmComment1 | tfmEscaped1;
        }
        else if ( lstrcmp(pszExt, _T("pas")) == 0 )
        {
            nnType.first = tftPas;
            nnType.second = tfmComment1;
        }
        else
        {
            nnType.first = tftText;

            if ( g_opt.IsHtmlCompatible(pszExt) )
                nnType.second |= tfmHtmlCompatible;

            if ( g_opt.IsEscapedFileExt(pszExt) )
                nnType.second |= tfmEscaped1;
        }

        if ( g_opt.IsSingleQuoteFileExt(pszExt) )
            nnType.second |= tfmSingleQuote;

        if ( g_opt.IsSupportedFile(pszExt) )
            nnType.second |= tfmIsSupported;
        else if ( (nnType.second & tfmIsSupported) != 0 )
            nnType.second ^= tfmIsSupported;
    }

    return nnType;
}
