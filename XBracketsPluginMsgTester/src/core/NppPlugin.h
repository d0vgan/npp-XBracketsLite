#ifndef _npp_plugin_h_
#define _npp_plugin_h_
//----------------------------------------------------------------------------
#include "base.h"
#include "SciMessager.h"
#include "NppMessager.h"
#include "NppPluginMenu.h"
#include <string>

typedef std::basic_string<TCHAR> tstr;

class CNppPlugin
{
    protected:
        CNppMessager  m_nppMsgr;
        HMODULE       m_hDllModule;
        tstr          m_sDllDir;
        tstr          m_sDllFileName;
        tstr          m_sIniFileName;

    public:
        CNppPlugin();
        virtual ~CNppPlugin();

        // called from DllMain
        void OnDllProcessAttach(HINSTANCE hDLLInstance);
        void OnDllProcessDetach();

        // standard n++ plugin functions
        virtual void         nppBeNotified(SCNotification* pscn)  { }
        virtual FuncItem*    nppGetFuncsArray(int* pnbFuncItems) = 0;
        virtual const TCHAR* nppGetName() = 0;
        virtual LRESULT      nppMessageProc(UINT uMessage, WPARAM wParam, LPARAM lParam)  { return 1; }
        void                 nppSetInfo(const NppData& nppd);

        // common n++ notification
        virtual void OnNppSetInfo(const NppData& nppd)  { }

        HMODULE getDllModule() const  { return m_hDllModule; }
        const tstr& getDllDir() const  { return m_sDllDir; } // full directory path to the .dll
        const tstr& getDllFileName() const  { return m_sDllFileName; } // name.dll
        const tstr& getIniFileName() const  { return m_sIniFileName; } // name.ini
        HWND getNppWnd() const  { return m_nppMsgr.getNppWnd(); }
        const CNppMessager& getNppMsgr() const  { return m_nppMsgr; }
        CNppMessager& getNppMsgr()  { return m_nppMsgr; }
};

//----------------------------------------------------------------------------
#endif
