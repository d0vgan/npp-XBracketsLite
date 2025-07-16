#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"

extern CXBracketsPlugin thePlugin;
CXBracketsOptions g_opt;

const TCHAR* CXBracketsPlugin::PLUGIN_NAME = _T("XBrackets Lite");

bool CXBracketsPlugin::isNppMacroStarted = false;
bool CXBracketsPlugin::isNppWndUnicode = true;
WNDPROC CXBracketsPlugin::nppOriginalWndProc = NULL;
WNDPROC CXBracketsPlugin::sciOriginalWndProc1 = NULL;
WNDPROC CXBracketsPlugin::sciOriginalWndProc2 = NULL;

LRESULT CXBracketsPlugin::nppCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pCallWindowProc = isNppWndUnicode ? CallWindowProcW : CallWindowProcA;
    return pCallWindowProc(nppOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CXBracketsPlugin::sciCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pSciWndProc = thePlugin.getNppMsgr().getSciMainWnd() == hWnd ? sciOriginalWndProc1 : sciOriginalWndProc2;
    auto pCallWindowProc = isNppWndUnicode ? CallWindowProcW : CallWindowProcA;
    return pCallWindowProc(pSciWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CXBracketsPlugin::nppNewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_COMMAND )
    {
        const WORD id = LOWORD(wParam);
        switch ( id )
        {
            case IDM_FILE_RELOAD:
                thePlugin.OnNppFileReload();
                break;

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
    else if ( uMsg == NPPM_RELOADFILE )
    {
        thePlugin.OnNppFileReload();
    }
    else if ( uMsg == NPPM_RELOADBUFFERID )
    {
        thePlugin.OnNppBufferReload();
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

LRESULT CALLBACK CXBracketsPlugin::sciNewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_CHAR )
    {
        // this happens _before_ the character is processed by Scintilla
        if ( thePlugin.OnSciChar(static_cast<int>(wParam)) != CXBracketsLogic::cprNone )
        {
            return 0; // processed by XBrackets, don't forward to Scintilla
        }
    }
    else if ( uMsg == WM_KEYDOWN )
    {
        if ( wParam == VK_DELETE || wParam == VK_BACK )
        {
            thePlugin.OnSciTextChanged();
        }
    }

    return sciCallWndProc(hWnd, uMsg, wParam, lParam);
}

FuncItem* CXBracketsPlugin::nppGetFuncsArray(int* pnbFuncItems)
{
    *pnbFuncItems = CXBracketsMenu::N_NBFUNCITEMS;
    return CXBracketsMenu::arrFuncItems;
}

const TCHAR* CXBracketsPlugin::nppGetName()
{
    return PLUGIN_NAME;
}

void CXBracketsPlugin::nppBeNotified(SCNotification* pscn)
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
        // >>> notifications from Scintilla
        switch ( pscn->nmhdr.code )
        {
            case SCN_MODIFIED:
                OnSciModified(pscn);
                break;
        }
        // <<< notifications from Scintilla
    }
}

void CXBracketsPlugin::OnNppSetInfo(const NppData& nppd)
{
    m_PluginMenu.SetNppData(nppd);
    m_BracketsLogic.SetNppData(nppd);
    isNppWndUnicode = ::IsWindowUnicode(nppd._nppHandle) ? true : false;
}

void CXBracketsPlugin::OnNppBufferActivated()
{
    m_BracketsLogic.UpdateFileType();
    m_BracketsLogic.InvalidateCachedAutoRightBr();
    m_BracketsLogic.InvalidateCachedBrPair();
}

void CXBracketsPlugin::OnNppBufferReload()
{
    m_BracketsLogic.InvalidateCachedAutoRightBr();
    m_BracketsLogic.InvalidateCachedBrPair();
}

void CXBracketsPlugin::OnNppFileOpened()
{
    //this handler is not needed because file opening
    //is handled by OnNppBufferActivated()
}

void CXBracketsPlugin::OnNppFileReload()
{
    m_BracketsLogic.InvalidateCachedAutoRightBr();
    m_BracketsLogic.InvalidateCachedBrPair();
}

void CXBracketsPlugin::OnNppFileSaved()
{
    m_BracketsLogic.UpdateFileType();
    // CachedAutoRightBr is still valid
    m_BracketsLogic.InvalidateCachedBrPair(); // file type may be changed
}

void CXBracketsPlugin::OnNppReady()
{
    ReadOptions();
    CXBracketsMenu::UpdateMenuState();
    m_BracketsLogic.UpdateFileType();
    m_BracketsLogic.InvalidateCachedAutoRightBr();
    m_BracketsLogic.InvalidateCachedBrPair();

    m_nppMsgr.SendNppMsg(NPPM_ADDSCNMODIFIEDFLAGS, 0, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);

    auto pSetWindowLongPtr = isNppWndUnicode ? SetWindowLongPtrW : SetWindowLongPtrA;

    nppOriginalWndProc = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppNewWndProc );

    sciOriginalWndProc1 = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getSciMainWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );

    sciOriginalWndProc2 = (WNDPROC) pSetWindowLongPtr(
        m_nppMsgr.getSciSecondWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );
}

void CXBracketsPlugin::OnNppShutdown()
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

void CXBracketsPlugin::OnNppMacro(int nMacroState)
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
        m_BracketsLogic.UpdateFileType();
        m_BracketsLogic.InvalidateCachedAutoRightBr();
        m_BracketsLogic.InvalidateCachedBrPair();
    }

    CXBracketsMenu::AllowAutocomplete(!isNppMacroStarted);
}

CXBracketsLogic::eCharProcessingResult CXBracketsPlugin::OnSciChar(const int ch)
{
    return m_BracketsLogic.OnChar(ch);
}

void CXBracketsPlugin::OnSciModified(SCNotification* pscn)
{
    if ( pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT) )
    {
        OnSciTextChanged();
    }
}

void CXBracketsPlugin::OnSciTextChanged()
{
    m_BracketsLogic.InvalidateCachedAutoRightBr();
    m_BracketsLogic.InvalidateCachedBrPair();
}

void CXBracketsPlugin::GoToMatchingBracket()
{
    m_BracketsLogic.PerformBracketsAction(CXBracketsLogic::baGoToMatching);
}

void CXBracketsPlugin::GoToNearestBracket()
{
    m_BracketsLogic.PerformBracketsAction(CXBracketsLogic::baGoToNearest);
}

void CXBracketsPlugin::SelToMatchingBracket()
{
    m_BracketsLogic.PerformBracketsAction(CXBracketsLogic::baSelToMatching);
}

void CXBracketsPlugin::SelToNearestBrackets()
{
    m_BracketsLogic.PerformBracketsAction(CXBracketsLogic::baSelToNearest);
}

void CXBracketsPlugin::ReadOptions()
{
    TCHAR szPath[2*MAX_PATH + 1];

    m_nppMsgr.getPluginsConfigDir(2*MAX_PATH, szPath);
    lstrcat(szPath, _T("\\"));
    lstrcat(szPath, m_szIniFileName);

    g_opt.ReadOptions(szPath);
}

void CXBracketsPlugin::SaveOptions()
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
