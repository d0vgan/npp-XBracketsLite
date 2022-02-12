#include "XBrackets.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"


// can be _T(x), but _T(x) may be incompatible with ANSI mode
#define _TCH(x)  (x)


extern CXBrackets thePlugin;
CXBracketsOptions g_opt;

const TCHAR* CXBrackets::PLUGIN_NAME = _T("XBrackets Lite");
const char* CXBrackets::strBrackets[tbtCount - 1] = {
    "()",
    "[]",
    "{}",
    "\"\"",
    "\'\'",
    "<>",
    "</>"
};

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
    return ( isNppWndUnicode ?
               ::CallWindowProcW(nppOriginalWndProc, hWnd, uMsg, wParam, lParam) :
                 ::CallWindowProcA(nppOriginalWndProc, hWnd, uMsg, wParam, lParam) );
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

CXBrackets::CXBrackets()
{
    m_nAutoRightBracketPos = -1;
    m_nFileType = tftNone;
    m_bSupportedFileType = true;
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
    else
    {
        // >>> notifications from Notepad++
        switch ( pscn->nmhdr.code )
        {
            case SCN_CHARADDED:
                OnSciCharAdded(pscn->ch);
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
        nPrevAutoComplete = g_opt.m_bBracketsAutoComplete ? 1 : 0;
        g_opt.m_bBracketsAutoComplete = false;
    }
    else
    {
        if ( nPrevAutoComplete < 0 ) // initialize now
            nPrevAutoComplete = g_opt.m_bBracketsAutoComplete ? 1 : 0;

        g_opt.m_bBracketsAutoComplete = (nPrevAutoComplete > 0);
        UpdateFileType();
    }

    CXBracketsMenu::AllowAutocomplete(!isNppMacroStarted);
}

void CXBrackets::OnSciCharAdded(const int ch)
{
    if ( !g_opt.m_bBracketsAutoComplete )
        return;

    if ( !m_bSupportedFileType )
        return;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    int nSelections = (int) sciMsgr.SendSciMsg(SCI_GETSELECTIONS);
    if ( nSelections > 1 )
        return; // nothing to do with multiple selections
    
    int nBracketType = tbtNone;
    
    if ( m_nAutoRightBracketPos >= 0 )
    {
        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        int nRightBracketType = tbtNone;

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
                nRightBracketType = tbtDblQuote;
                break;
            case _TCH('\'') :
                if ( g_opt.m_bBracketsDoSingleQuote )
                    nRightBracketType = tbtSglQuote;
                break;
            case _TCH('>') :
                if ( g_opt.m_bBracketsDoTag )
                    nRightBracketType = tbtTag; 
                // no break here
            case _TCH('/') :
                if ( g_opt.m_bBracketsDoTag2 )
                    nRightBracketType = tbtTag2;
                break;
        }
    
        if ( nRightBracketType != tbtNone )
        {
            Sci_Position pos = sciMsgr.getCurrentPos() - 1;
        
            if ( pos == m_nAutoRightBracketPos )
            {
                // previous character
                char prev_ch = sciMsgr.getCharAt(pos - 1);
                if ( prev_ch == strBrackets[nRightBracketType - tbtBracket][0] )
                {
                    char next_ch = sciMsgr.getCharAt(pos + 1);
                    if ( next_ch == strBrackets[nRightBracketType - tbtBracket][1] )
                    {
                        sciMsgr.beginUndoAction();
                        // remove just typed right bracket
                        sciMsgr.setSel(pos, pos + 1);
                        sciMsgr.setSelText("");
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

    // OK for both ANSI and Unicode (ch can be wide character)
    switch ( ch )
    {
        case _TCH('(') :
            nBracketType = tbtBracket;
            break;
        case _TCH('[') :
            nBracketType = tbtSquare;
            break;
        case _TCH('{') :
            nBracketType = tbtBrace;
            break;
        case _TCH('\"') :
            nBracketType = tbtDblQuote;
            break;
        case _TCH('\'') :
            if ( g_opt.m_bBracketsDoSingleQuote )
                nBracketType = tbtSglQuote;
            break;
        case _TCH('<') :
            if ( g_opt.m_bBracketsDoTag )
                nBracketType = tbtTag;
            break;
    }

    if ( nBracketType != tbtNone )
    {
        // a typed character is a bracket
        AutoBracketsFunc(nBracketType);
    }
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
    if ( !g_opt.m_bBracketsAutoComplete )
        return;
    
    m_nAutoRightBracketPos = -1;
    m_nFileType = getFileType(m_bSupportedFileType);
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
    if ( nBracketType == tbtTag )
    {
        if ( g_opt.m_bBracketsDoTagIf && (m_nFileType != tftHtmlCompatible) )
            return;
    }
    
    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();

    // is something selected?
    if ( nEditEndPos != nEditPos )
    {
        if ( sciMsgr.getSelectionMode() == SC_SEL_RECTANGLE )
            return;

        // removing selection
        sciMsgr.setSelText("");
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
    else if (
      ( (next_ch == ')')  &&
        (g_opt.m_bBracketsRightExistsOK || (nBracketType != tbtBracket)) )  ||
      ( (next_ch == ']')  &&
        (g_opt.m_bBracketsRightExistsOK || (nBracketType != tbtSquare)) )   ||
      ( (next_ch == '}')  &&
        (g_opt.m_bBracketsRightExistsOK || (nBracketType != tbtBrace)) )    ||
      ( (next_ch == '\"') &&
        (g_opt.m_bBracketsRightExistsOK || (nBracketType != tbtDblQuote)) ) ||
      ( (next_ch == '\'') &&
        ((!g_opt.m_bBracketsDoSingleQuote) || g_opt.m_bBracketsRightExistsOK ||
         (nBracketType != tbtSglQuote)) ) ||
      ( (next_ch == '>' || next_ch == '/') &&
        ((!g_opt.m_bBracketsDoTag) || g_opt.m_bBracketsRightExistsOK ||
         (nBracketType != tbtTag)) ) )
    {
        bNextCharOK = true;
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

    if ( bPrevCharOK && bNextCharOK && g_opt.m_bBracketsSkipEscaped )
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
            if ( g_opt.m_bBracketsDoTag2 )
                nBracketType = tbtTag2;
        }

        sciMsgr.beginUndoAction();
        // selection
        sciMsgr.setSel(nEditPos - 1, nEditPos);
        // inserting brackets
        sciMsgr.setSelText(strBrackets[nBracketType - tbtBracket]);
        // placing the caret between brackets
        sciMsgr.setSel(nEditPos, nEditPos);
        sciMsgr.endUndoAction();

        m_nAutoRightBracketPos = nEditPos;
    }

}

int CXBrackets::getFileType(bool& isSupported)
{
    TCHAR szExt[CXBracketsOptions::MAX_EXT];
    int   nType = tftNone;

    isSupported = true;
    szExt[0] = 0;
    m_nppMsgr.getCurrentFileExtPart(CXBracketsOptions::MAX_EXT - 1, szExt);

    if ( szExt[0] )
    {
        TCHAR* pszExt = szExt;
        
        if ( *pszExt == _T('.') )
        {
            ++pszExt;
            if ( !(*pszExt) )
                return nType;
        }

        ::CharLower(pszExt);

        if ( lstrcmp(pszExt, _T("c")) == 0 ||
             lstrcmp(pszExt, _T("cc")) == 0 ||
             lstrcmp(pszExt, _T("cpp")) == 0 ||
             lstrcmp(pszExt, _T("cxx")) == 0 )
        {
            nType = tftC_Cpp;
        }
        else if ( lstrcmp(pszExt, _T("h")) == 0 ||
                  lstrcmp(pszExt, _T("hh")) == 0 ||
                  lstrcmp(pszExt, _T("hpp")) == 0 ||
                  lstrcmp(pszExt, _T("hxx")) == 0 )
        {
            nType = tftH_Hpp;
        }
        else if ( lstrcmp(pszExt, _T("pas")) == 0 )
        {
            nType = tftPas;
        }
        else if ( g_opt.IsHtmlCompatible(pszExt) )
        {
            nType = tftHtmlCompatible;
        }
        else
        {
            nType = tftText;
        }

        isSupported = g_opt.IsSupportedFile(pszExt);
    }

    return nType;
}
