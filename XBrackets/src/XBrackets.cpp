#include "XBrackets.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"
#include <vector>


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
  m_nnFileType{ tftNone, tfmIsSupported }
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

CXBrackets::eCharProcessingResult CXBrackets::OnSciChar(const int ch)
{
    if ( !g_opt.getBracketsAutoComplete() )
        return cprNone;

    if ( (m_nnFileType.second & tfmIsSupported) == 0 )
        return cprNone;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return cprNone; // nothing to do with multiple selections

    if ( m_nAutoRightBracketPos >= 0 )
    {
        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        int nRightBracketType = getRightBracketType(ch);

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

    int nLeftBracketType = getLeftBracketType(ch);
    if ( nLeftBracketType == tbtNone )
        return cprNone;

    // a typed character is a bracket
    return AutoBracketsFunc(nLeftBracketType);
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

static bool isWhiteSpaceOrNulChar(const char ch)
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

CXBrackets::eCharProcessingResult CXBrackets::AutoBracketsFunc(int nBracketType)
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

    int nRightBracketType = getRightBracketType(next_ch, bofIgnoreMode);
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

    if ( bPrevCharOK && bNextCharOK &&
         g_opt.getBracketsSkipEscaped() && (m_nnFileType.second & tfmEscaped1) != 0 )
    {
        char szPrefix[MAX_ESCAPED_PREFIX + 2];
        Sci_Position pos;
        int len; // len <= MAX_ESCAPED_PREFIX, so 'int' is always enough

        getEscapedPrefixPos(nEditPos, &pos, &len);
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

bool CXBrackets::isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, int* pnBracketType, bool bInSelection)
{
    if ( pszTextLeft == pszTextRight )
        return false;

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

bool CXBrackets::SelAutoBrFunc(int nBracketType)
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

    int nBrAltType = nBracketType;
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
