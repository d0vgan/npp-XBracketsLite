#ifndef _xbrackets_plugin_msg_tester_h_
#define _xbrackets_plugin_msg_tester_h_
//----------------------------------------------------------------------------
#include "core/NppPlugin.h"
#include "MsgTesterMenu.h"


class CMsgTester: public CNppPlugin
{
    public:
        static const TCHAR* const PLUGIN_NAME;

    protected:
        CMsgTesterMenu m_nppPluginMenu;

    public:
        // standard n++ plugin functions
        virtual void         nppBeNotified(SCNotification* pscn) override;
        virtual FuncItem*    nppGetFuncsArray(int* pnbFuncItems) override;
        virtual const TCHAR* nppGetName() override;

        // common n++ notification
        virtual void OnNppSetInfo(const NppData& nppd) override;

        // custom n++ notifications
        void OnNppReady();
        void OnNppShutdown();

};

CMsgTester& GetMsgTester();

//----------------------------------------------------------------------------
#endif
