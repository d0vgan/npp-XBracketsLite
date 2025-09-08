#include "MsgTester.h"
#include "PluginCommunication/xbrackets_msgs.h"
#include "XBracketsPluginMsgSender.h"


const TCHAR* const CMsgTester::PLUGIN_NAME = _T("XBracketsPluginMsgTester");

FuncItem* CMsgTester::nppGetFuncsArray(int* pnbFuncItems)
{
    *pnbFuncItems = CMsgTesterMenu::N_NBFUNCITEMS;
    return CMsgTesterMenu::arrFuncItems;
}

const TCHAR* CMsgTester::nppGetName()
{
    return CMsgTester::PLUGIN_NAME;
}

void CMsgTester::nppBeNotified(SCNotification* pscn)
{
    if ( pscn->nmhdr.hwndFrom == m_nppMsgr.getNppWnd() )
    {
        // >>> notifications from Notepad++
        switch ( pscn->nmhdr.code )
        {
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

void CMsgTester::OnNppSetInfo(const NppData& nppd)
{
    m_nppPluginMenu.SetNppData(nppd);
}

void CMsgTester::OnNppReady()
{
    // TODO:  add your code here :)
}

void CMsgTester::OnNppShutdown()
{
    // TODO:  add your code here :)
}

CMsgTester& GetMsgTester()
{
    static CMsgTester theMsgTester;

    return theMsgTester;
}
