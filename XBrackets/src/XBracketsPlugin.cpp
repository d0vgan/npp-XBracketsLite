#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"
#include "PluginCommunication/xbrackets_msgs.h"

extern CXBracketsPlugin thePlugin;
CXBracketsOptions g_opt;

const TCHAR* CXBracketsPlugin::PLUGIN_NAME = _T("XBrackets Lite");

bool CXBracketsPlugin::isNppMacroStarted = false;
bool CXBracketsPlugin::isNppWndUnicode = true;
WNDPROC CXBracketsPlugin::nppOriginalWndProc = NULL;
WNDPROC CXBracketsPlugin::sciOriginalWndProc1 = NULL;
WNDPROC CXBracketsPlugin::sciOriginalWndProc2 = NULL;

CXBracketsPlugin::CConfigFileChangeListener::CConfigFileChangeListener(CXBracketsPlugin* pPlugin) : m_pPlugin(pPlugin)
{
}

void CXBracketsPlugin::CConfigFileChangeListener::HandleFileChange(const FileInfoStruct* pFile)
{
    if ( pFile->fileSize.LowPart == 0 )
        return; // Notepad++ seems to replace the file during the saving

    m_pPlugin->OnConfigFileChanged(pFile->filePath);
}

CXBracketsPlugin::CXBracketsPlugin() : m_ConfigFileChangeListener(this)
{
}

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
            thePlugin.OnSciTextChanged(nullptr);
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

LRESULT CXBracketsPlugin::nppMessageProc(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if ( uMessage == NPPM_MSGTOPLUGIN )
    {
        return OnNppMsgToPlugin(reinterpret_cast<CommunicationInfo *>(lParam));
    }

    return 1;
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

            case SCN_AUTOCCOMPLETED:
                OnSciAutoCompleted(pscn);
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
}

void CXBracketsPlugin::OnNppBufferReload()
{
    m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
}

void CXBracketsPlugin::OnNppFileOpened()
{
    //this handler is not needed because file opening
    //is handled by OnNppBufferActivated()
}

void CXBracketsPlugin::OnNppFileReload()
{
    m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
}

void CXBracketsPlugin::OnNppFileSaved()
{
    // AutoRightBr is still valid, but file type may be changed:
    m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfTree);
}

void CXBracketsPlugin::OnNppReady()
{
    ReadOptions();
    CXBracketsMenu::UpdateMenuState();
    m_BracketsLogic.UpdateFileType();

    unsigned int uSciFlags = (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT);
    if ( g_opt.getUpdateTreeAllowed() )
    {
        uSciFlags |= (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE);
    }
    m_nppMsgr.SendNppMsg(NPPM_ADDSCNMODIFIEDFLAGS, 0, uSciFlags);

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
    m_FileWatcher.StopWatching();

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
        m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
    }
    else
    {
        if ( nPrevAutoComplete < 0 ) // initialize now
            nPrevAutoComplete = g_opt.getBracketsAutoComplete() ? 1 : 0;

        g_opt.setBracketsAutoComplete(nPrevAutoComplete > 0);
        m_BracketsLogic.UpdateFileType();
    }

    CXBracketsMenu::AllowAutocomplete(!isNppMacroStarted);
}

LRESULT CXBracketsPlugin::OnNppMsgToPlugin(CommunicationInfo* pInfo)
{
    if ( pInfo == nullptr || pInfo->info == nullptr )
        return FALSE;

    switch ( pInfo->internalMsg )
    {
        case XBRM_GETMATCHINGBRACKETS:
        case XBRM_GETNEARESTBRACKETS:
        {
            tXBracketsPairStruct* pBrPair = reinterpret_cast<tXBracketsPairStruct *>(pInfo->info);
            const Sci_Position nPos = pBrPair->nPos;
            ::ZeroMemory(pBrPair, sizeof(tXBracketsPairStruct));
            pBrPair->nPos = nPos;

            const bool isExactPos = (pInfo->internalMsg == XBRM_GETMATCHINGBRACKETS);
            const XBrackets::tBrPairItem* pItem = m_BracketsLogic.FindBracketsByPos(nPos, isExactPos);
            if ( pItem != nullptr )
            {
                pBrPair->pszLeftBr = pItem->pBrPair->leftBr.c_str();
                pBrPair->pszRightBr = pItem->pBrPair->rightBr.c_str();
                pBrPair->nLeftBrLen = static_cast<Sci_Position>(pItem->pBrPair->leftBr.length());
                pBrPair->nRightBrLen = static_cast<Sci_Position>(pItem->pBrPair->rightBr.length());
                pBrPair->nLeftBrPos = pItem->nLeftBrPos;
                pBrPair->nRightBrPos = pItem->nRightBrPos;
                return TRUE;
            }

            break;
        }
    }

    return FALSE;
}

CXBracketsLogic::eCharProcessingResult CXBracketsPlugin::OnSciChar(const int ch)
{
    return m_BracketsLogic.OnCharPress(ch);
}

void CXBracketsPlugin::OnSciModified(SCNotification* pscn)
{
    if ( pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE) )
    {
        OnSciTextChanged(pscn);
    }
}

void CXBracketsPlugin::OnSciAutoCompleted(SCNotification* pscn)
{
    m_BracketsLogic.OnTextAutoCompleted(pscn->text, pscn->position);
}

void CXBracketsPlugin::OnSciTextChanged(SCNotification* pscn)
{
    unsigned int uInvalidateFlags = CXBracketsLogic::icbfAutoRightBr;
    if ( pscn != nullptr )
    {
        uInvalidateFlags |= CXBracketsLogic::icbfAll;
    }
    m_BracketsLogic.InvalidateCachedBrackets(uInvalidateFlags, pscn);
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
    m_sIniFilePath = szPath;
    if ( !m_sIniFilePath.empty() )
    {
        const TCHAR ch = m_sIniFilePath.back();
        if ( ch != _T('\\') && ch != _T('/') )
        {
            m_sIniFilePath += _T('\\');
        }
        m_sUserConfigFilePath = m_sIniFilePath;
    }
    m_sUserConfigFilePath += _T("XBrackets_UserConfig.json");
    m_sIniFilePath += m_sIniFileName;

    g_opt.ReadOptions(m_sIniFilePath.c_str());

    m_sConfigFilePath = getDllDir();
    m_sConfigFilePath.append(_T("\\XBrackets_Config.json"));

    const bool isUserConfig = XBrackets::isExistingFile(m_sUserConfigFilePath);
    if ( !isUserConfig )
    {
        if ( !XBrackets::isExistingFile(m_sConfigFilePath) )
        {
            tstr err = _T("The default config file does not exist:\r\n");
            err.append(m_sConfigFilePath);
            PluginMessageBox(err.c_str(), MB_OK | MB_ICONERROR);
            return;
        }
    }

    const tstr err = g_opt.ReadConfig(isUserConfig ? m_sUserConfigFilePath : m_sConfigFilePath);
    if ( !err.empty() )
    {
        if ( isUserConfig )
        {
            g_opt.ReadConfig(m_sConfigFilePath);
            onConfigFileError(m_sUserConfigFilePath, err);
        }
        else
            PluginMessageBox(err.c_str(), MB_OK | MB_ICONERROR);
    }

    if ( isUserConfig )
    {
        m_FileWatcher.AddFile(m_sUserConfigFilePath.c_str(), &m_ConfigFileChangeListener);
        m_FileWatcher.StartWatching();
    }
}

void CXBracketsPlugin::SaveOptions()
{
    if ( g_opt.MustBeSaved() )
    {
        g_opt.SaveOptions(m_sIniFilePath.c_str());
    }
}

void CXBracketsPlugin::OnSettings()
{
    if ( !XBrackets::isExistingFile(m_sUserConfigFilePath) )
    {
        ::CopyFile(m_sConfigFilePath.c_str(), m_sUserConfigFilePath.c_str(), TRUE);

        m_FileWatcher.AddFile(m_sUserConfigFilePath.c_str(), &m_ConfigFileChangeListener);
        m_FileWatcher.StartWatching();
    }

    m_nppMsgr.doOpen(m_sUserConfigFilePath.c_str());
}

void CXBracketsPlugin::OnConfigFileChanged(const tstr& configFilePath)
{
    const tstr err = g_opt.ReadConfig(configFilePath);
    if ( !err.empty() )
    {
        onConfigFileError(configFilePath, err);
        return;
    }

    m_PluginMenu.UpdateMenuState();
    m_BracketsLogic.UpdateFileType();
}

void CXBracketsPlugin::onConfigFileError(const tstr& configFilePath, const tstr& err)
{
    PluginMessageBox(err.c_str(), MB_OK | MB_ICONERROR);
    m_nppMsgr.doOpen(configFilePath.c_str());
    const auto n = err.find(_T("Stopped at offset "));
    if (n != tstr::npos)
    {
        int pos = _ttoi(err.c_str() + n + 18);
        if (pos > 0)
        {
            CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
            sciMsgr.setSel(pos - 1, pos - 1);
        }
    }
}

void CXBracketsPlugin::PluginMessageBox(const TCHAR* szMessageText, UINT uType)
{
    ::MessageBox(
        m_nppMsgr.getNppWnd(),
        szMessageText,
        PLUGIN_NAME,
        uType
      );
}
