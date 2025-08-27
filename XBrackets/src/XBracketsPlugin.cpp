#include "XBracketsPlugin.h"
#include "XBracketsOptions.h"
#include "core/npp_files/resource.h"
#include "PluginCommunication/xbrackets_msgs.h"

#define XBRM_INTERNAL 0x0A00
#define XBRM_CFGUPD   (XBRM_INTERNAL + 1)

const TCHAR* CXBracketsPlugin::PLUGIN_NAME = _T("XBrackets Lite");

bool CXBracketsPlugin::isNppMacroStarted = false;
bool CXBracketsPlugin::isNppWndUnicode = true;
bool CXBracketsPlugin::isNppReady = false;
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

CXBracketsPlugin::CXBracketsPlugin() :
  m_ConfigFileChangeListener(this),
  m_nHlSciIdx(-1),
  m_nHlTimerId(0),
  m_nHlSciStyleInd(-1),
  m_nHlSciStyleIndByNpp(-1),
  m_isCfgUpdInProgress(false)
{
    ::ZeroMemory(&m_ciCfgUpd, sizeof(m_ciCfgUpd));
}

CXBracketsPlugin::~CXBracketsPlugin()
{
    m_csHl.Lock();
    if ( m_nHlTimerId != 0 )
    {
        ::KillTimer(NULL, m_nHlTimerId);
        m_nHlTimerId = 0;
    }
    m_csHl.Unlock();
}

LRESULT CXBracketsPlugin::nppCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pCallWindowProc = isNppWndUnicode ? CallWindowProcW : CallWindowProcA;
    return pCallWindowProc(nppOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CXBracketsPlugin::sciCallWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC pSciWndProc = GetPlugin().getNppMsgr().getSciMainWnd() == hWnd ? sciOriginalWndProc1 : sciOriginalWndProc2;
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
                GetPlugin().OnNppFileReload();
                break;

            case IDM_MACRO_STARTRECORDINGMACRO:
                GetPlugin().OnNppMacro(MACRO_START);
                break;

            case IDM_MACRO_STOPRECORDINGMACRO:
                GetPlugin().OnNppMacro(MACRO_STOP);
                break;

            case IDC_EDIT_TOGGLEMACRORECORDING:
                GetPlugin().OnNppMacro(MACRO_TOGGLE);
                break;

            case IDM_MACRO_PLAYBACKRECORDEDMACRO:
                {
                    GetPlugin().OnNppMacro(MACRO_START);
                    LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
                    GetPlugin().OnNppMacro(MACRO_STOP);
                    return lResult;
                }
                break;

            default:
                if ( (id >= ID_MACRO) && (id < ID_MACRO_LIMIT) )
                {
                    GetPlugin().OnNppMacro(MACRO_START);
                    LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
                    GetPlugin().OnNppMacro(MACRO_STOP);
                    return lResult;
                }
                break;
        }
    }
    else if ( uMsg == NPPM_RELOADFILE )
    {
        GetPlugin().OnNppFileReload();
    }
    else if ( uMsg == NPPM_RELOADBUFFERID )
    {
        GetPlugin().OnNppBufferReload();
    }
    else if ( uMsg == WM_MACRODLGRUNMACRO )
    {
        GetPlugin().OnNppMacro(MACRO_START);
        LRESULT lResult = nppCallWndProc(hWnd, uMsg, wParam, lParam);
        GetPlugin().OnNppMacro(MACRO_STOP);
        return lResult;
    }
    else if ( uMsg == NPPM_MSGTOPLUGIN )
    {
        return GetPlugin().OnNppMsgToPlugin(reinterpret_cast<CommunicationInfo *>(lParam));
    }

    return nppCallWndProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CXBracketsPlugin::sciNewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( uMsg == WM_CHAR )
    {
        // this happens _before_ the character is processed by Scintilla
        if ( GetPlugin().OnSciChar(static_cast<int>(wParam)) != CXBracketsLogic::cprNone )
        {
            return 0; // processed by XBrackets, don't forward to Scintilla
        }
    }

    return sciCallWndProc(hWnd, uMsg, wParam, lParam);
}

void CALLBACK CXBracketsPlugin::HlTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
    CXBracketsPlugin& thePlugin = GetPlugin();

    thePlugin.m_csHl.Lock();
    if ( thePlugin.m_nHlTimerId != 0 )
    {
        auto elapsed_duration = std::chrono::system_clock::now() - thePlugin.m_lastTextChangedTimePoint;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_duration).count();
        if ( elapsed_ms >= GetOptions().getHighlightTypingDelayMs() )
        {
            ::KillTimer(NULL, thePlugin.m_nHlTimerId);
            thePlugin.m_nHlTimerId = 0;
            thePlugin.highlightActiveBrackets(-1);
        }
    }
    thePlugin.m_csHl.Unlock();
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

            case SCN_AUTOCCOMPLETED:
                OnSciAutoCompleted(pscn);
                break;

            case SCN_UPDATEUI:
                OnSciUpdateUI(pscn);
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
    if ( !isNppReady )
        return;

    clearActiveBrackets();
    m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfAll);
}

void CXBracketsPlugin::OnNppBufferReload()
{
    clearActiveBrackets();
    m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
}

void CXBracketsPlugin::OnNppFileOpened()
{
    //this handler is not needed because file opening
    //is handled by OnNppBufferActivated()
}

void CXBracketsPlugin::OnNppFileReload()
{
    clearActiveBrackets();
    m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
}

void CXBracketsPlugin::OnNppFileSaved()
{
    // AutoRightBr is still valid, but file type may be changed:
    if ( m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfTree) )
    {
        clearActiveBrackets();
    }
}

void CXBracketsPlugin::OnNppReady()
{
    m_ciCfgUpd.internalMsg = XBRM_CFGUPD;
    m_ciCfgUpd.srcModuleName = getDllFileName().c_str();
    m_ciCfgUpd.info = &m_ciCfgUpd; // now it's non-null, as expected :)

    ReadOptions();
    CXBracketsMenu::UpdateMenuState();
    m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfAll | CXBracketsLogic::uftfConfigUpdated);

    unsigned int uSciFlags = (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE);
    m_nppMsgr.SendNppMsg(NPPM_ADDSCNMODIFIEDFLAGS, 0, uSciFlags);

    auto pSetWindowLongPtr = isNppWndUnicode ? SetWindowLongPtrW : SetWindowLongPtrA;

    nppOriginalWndProc = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getNppWnd(), GWLP_WNDPROC, (LONG_PTR) nppNewWndProc );

    sciOriginalWndProc1 = (WNDPROC) pSetWindowLongPtr(
      m_nppMsgr.getSciMainWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );

    sciOriginalWndProc2 = (WNDPROC) pSetWindowLongPtr(
        m_nppMsgr.getSciSecondWnd(), GWLP_WNDPROC, (LONG_PTR) sciNewWndProc );

    isNppReady = true;
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
        nPrevAutoComplete = GetOptions().getBracketsAutoComplete() ? 1 : 0;
        GetOptions().setBracketsAutoComplete(false);
        clearActiveBrackets();
        m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll);
    }
    else
    {
        if ( nPrevAutoComplete < 0 ) // initialize now
            nPrevAutoComplete = GetOptions().getBracketsAutoComplete() ? 1 : 0;

        GetOptions().setBracketsAutoComplete(nPrevAutoComplete > 0);
        m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfAll);
    }

    CXBracketsMenu::AllowAutocomplete(!isNppMacroStarted);
}

LRESULT CXBracketsPlugin::OnNppMsgToPlugin(CommunicationInfo* pInfo)
{
    if ( pInfo == nullptr || pInfo->info == nullptr )
        return FALSE;

    switch ( pInfo->internalMsg )
    {
    case XBRM_CFGUPD:
        onConfigFileUpdated();
        break;

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
        }
        break;
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
        OnSciTextChange(pscn);
    }
}

void CXBracketsPlugin::OnSciAutoCompleted(SCNotification* pscn)
{
    clearActiveBrackets();
    m_BracketsLogic.OnTextAutoCompleted(pscn->text, pscn->position);
}

void CXBracketsPlugin::OnSciUpdateUI(SCNotification* pscn)
{
    if ( m_nHlSciStyleInd < 0 )
        return;

    const HWND hSciWnd = reinterpret_cast<HWND>(pscn->nmhdr.hwndFrom);
    const int nSciIdx = (hSciWnd == m_nppMsgr.getSciMainWnd()) ? 0 : 1;
    if ( m_nHlSciIdx != -1 && nSciIdx != m_nHlSciIdx)
    {
        if ( m_hlBrPair[nSciIdx].nLeftBrPos != -1 && m_hlBrPair[nSciIdx].nRightBrPos != -1 )
        {
            clearActiveBrackets(nSciIdx);
        }
        return;
    }

    CSciMessager sciMsgr(hSciWnd);
    const Sci_Position pos = sciMsgr.getCurrentPos();

    const tHighlightBrPair& hlBrPair = m_hlBrPair[nSciIdx];
    if ( hlBrPair.nLeftBrPos != -1 && hlBrPair.nRightBrPos != -1 )
    {
        if ( pos < hlBrPair.nLeftBrPos || pos > hlBrPair.nRightBrPos + hlBrPair.nRightBrLen ||
             (pos > hlBrPair.nLeftBrPos + hlBrPair.nLeftBrLen && pos < hlBrPair.nRightBrPos) )
        {
            clearActiveBrackets();
        }
    }

    if ( hlBrPair.nLeftBrPos == -1 && hlBrPair.nRightBrPos == -1 )
    {
        if ( m_BracketsLogic.IsAtBracketPos(sciMsgr, pos) )
        {
            m_csHl.Lock();
            if ( m_nHlTimerId == 0 )
            {
                m_lastTextChangedTimePoint = std::chrono::system_clock::now();
                m_lastTextChangedTimePoint -= std::chrono::milliseconds(GetOptions().getHighlightTypingDelayMs());
                m_nHlTimerId = ::SetTimer(NULL, 0,  USER_TIMER_MINIMUM, HlTimerProc);
              #ifdef _DEBUG
                char str[100];
                wsprintfA(str, "OnSciUpdateUI -> HlTimerProc, pos = %d\n", static_cast<int>(pos));
                OutputDebugStringA(str);
              #endif
            }
            m_csHl.Unlock();
        }
    }
}

void CXBracketsPlugin::clearActiveBrackets(int nSciIdx)
{
    int iStart = 0;
    int iEnd = 2;
    if ( nSciIdx == 0 || nSciIdx == 1 )
    {
        iStart = nSciIdx;
        iEnd = iStart + 1;
    }

    m_csHl.Lock();
    // clearing the brackets "indication"
    for ( int i = iStart; i < iEnd; i++ )
    {
        if ( m_hlBrPair[i].nLeftBrPos != -1 && m_hlBrPair[i].nRightBrPos != -1 )
        {
            CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWndByIdx(i));

            sciMsgr.SendSciMsg(SCI_SETINDICATORCURRENT, m_nHlSciStyleInd);
            sciMsgr.SendSciMsg(SCI_INDICATORCLEARRANGE, m_hlBrPair[i].nLeftBrPos, m_hlBrPair[i].nLeftBrLen);
            sciMsgr.SendSciMsg(SCI_INDICATORCLEARRANGE, m_hlBrPair[i].nRightBrPos, m_hlBrPair[i].nRightBrLen);

            // nothing is "indicated"
            m_hlBrPair[i].nLeftBrPos = -1;
            m_hlBrPair[i].nRightBrPos = -1;
        }
    }
    if ( m_hlBrPair[0].nLeftBrPos == -1 && m_hlBrPair[1].nLeftBrPos == -1 )
    {
        // there's no highlighted brackets in both Scintilla's viewes
        m_nHlSciIdx = -1;
    }
    m_csHl.Unlock();
}

void CXBracketsPlugin::highlightActiveBrackets(Sci_Position pos)
{
    const int nSciIdx = m_nppMsgr.getCurrentScintillaIdx();
    tHighlightBrPair& hlBrPair = m_hlBrPair[nSciIdx];
    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWndByIdx(nSciIdx));

    if ( m_nHlSciStyleInd < 0 || hlBrPair.nLeftBrPos != -1 )
        return;

    if ( pos < 0 )
        pos = sciMsgr.getCurrentPos();

    const auto pBrItem = m_BracketsLogic.FindBracketsByPos(pos, true);
    if ( pBrItem == nullptr )
        return;

    // new positions for the "indication"
    hlBrPair.nLeftBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->leftBr.length());
    hlBrPair.nLeftBrPos = pBrItem->nLeftBrPos - hlBrPair.nLeftBrLen;
    hlBrPair.nRightBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->rightBr.length());
    hlBrPair.nRightBrPos = pBrItem->nRightBrPos;
    m_nHlSciIdx = nSciIdx;

    // setting the brackets "indication"
    sciMsgr.SendSciMsg(SCI_SETINDICATORCURRENT, m_nHlSciStyleInd);
    sciMsgr.SendSciMsg(SCI_INDICATORFILLRANGE, hlBrPair.nLeftBrPos, hlBrPair.nLeftBrLen);
    sciMsgr.SendSciMsg(SCI_INDICATORFILLRANGE, hlBrPair.nRightBrPos, hlBrPair.nRightBrLen);
}

void CXBracketsPlugin::OnSciTextChange(SCNotification* pscn)
{
    if ( pscn->modificationType & (SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE) )
    {
        clearActiveBrackets();
        m_BracketsLogic.InvalidateCachedBrackets(CXBracketsLogic::icbfAll, pscn);
    }
    else if ( pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT) )
    {
        m_csHl.Lock();
        m_lastTextChangedTimePoint = std::chrono::system_clock::now();
        if ( m_nHlTimerId == 0 )
        {
            m_nHlTimerId = ::SetTimer(NULL, 0, GetOptions().getHighlightTypingDelayMs()/4, HlTimerProc);
        }
        m_csHl.Unlock();
    }
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

    GetOptions().ReadOptions(m_sIniFilePath.c_str());

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

    const tstr err = GetOptions().ReadConfig(isUserConfig ? m_sUserConfigFilePath : m_sConfigFilePath);
    if ( !err.empty() )
    {
        if ( isUserConfig )
        {
            GetOptions().ReadConfig(m_sConfigFilePath);
            onConfigFileError(m_sUserConfigFilePath, err);
        }
        else
            PluginMessageBox(err.c_str(), MB_OK | MB_ICONERROR);
    }

    onConfigFileHasBeenRead();

    if ( isUserConfig )
    {
        m_FileWatcher.AddFile(m_sUserConfigFilePath.c_str(), &m_ConfigFileChangeListener);
        m_FileWatcher.StartWatching();
    }
}

void CXBracketsPlugin::SaveOptions()
{
    if ( GetOptions().MustBeSaved() )
    {
        GetOptions().SaveOptions(m_sIniFilePath.c_str());
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
    // This method is called from a different thread.
    // Let's send a plugin message to self to do the further processing in the main thread.
    m_csCfgUpd.Lock();
    m_sCfgFileUpd = configFilePath;
    if ( !m_isCfgUpdInProgress )
    {
        ::PostMessage( m_nppMsgr.getNppWnd(), NPPM_MSGTOPLUGIN,
            (WPARAM) getDllFileName().c_str(), (LPARAM) &m_ciCfgUpd );
        m_isCfgUpdInProgress = true;
    }
    m_csCfgUpd.Unlock();
}

void CXBracketsPlugin::onConfigFileUpdated()
{
    m_csCfgUpd.Lock();
    tstr configFilePath = m_sCfgFileUpd;
    const tstr err = GetOptions().ReadConfig(configFilePath);
    m_isCfgUpdInProgress = false;
    m_csCfgUpd.Unlock();

    if ( !err.empty() )
    {
        onConfigFileError(configFilePath, err);
        return;
    }

    clearActiveBrackets(); // clear the existing style indication first
    onConfigFileHasBeenRead(); // now read the new style, if any
    m_PluginMenu.UpdateMenuState();
    m_BracketsLogic.UpdateFileType(CXBracketsLogic::icbfAll | CXBracketsLogic::uftfConfigUpdated);
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

void CXBracketsPlugin::onConfigFileHasBeenRead()
{
    m_csHl.Lock();
    m_nHlSciStyleInd = GetOptions().getHighlightSciStyleIndIdx();
    if ( m_nHlSciStyleInd >= 0 )
    {
        if ( m_nHlSciStyleIndByNpp < 0 )
        {
            int nIndIdx = -1;
            if ( m_nppMsgr.SendNppMsg(NPPM_ALLOCATEINDICATOR, 1, (LPARAM) &nIndIdx) && nIndIdx != -1 )
            {
                m_nHlSciStyleIndByNpp = nIndIdx;
                m_nHlSciStyleInd = nIndIdx;
            }
        }
        else
        {
            m_nHlSciStyleInd = m_nHlSciStyleIndByNpp;
        }

        for ( int i = 0; i < 2; i++ )
        {
            CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWndByIdx(i));
            sciMsgr.SendSciMsg(SCI_INDICSETSTYLE, m_nHlSciStyleInd, GetOptions().getHighlightSciStyleIndType());
            sciMsgr.SendSciMsg(SCI_INDICSETFORE, m_nHlSciStyleInd, GetOptions().getHighlightSciColor());
        }
    }
    m_csHl.Unlock();
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

CXBracketsPlugin& GetPlugin()
{
    static CXBracketsPlugin thePlugin;

    return thePlugin;
}
