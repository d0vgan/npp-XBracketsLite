// Main.cpp : Defines the entry point for the DLL application.
//

#include "core/base.h"
#include "XBracketsPlugin.h"


extern "C" BOOL APIENTRY DllMain( 
                        HINSTANCE hInstance, 
                        DWORD     dwReason, 
                        LPVOID    lpReserved
					 )
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            GetPlugin().OnDllProcessAttach(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            GetPlugin().OnDllProcessDetach();
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        default:
            break;
    }
    
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData nppd)
{
    GetPlugin().nppSetInfo(nppd);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
    return GetPlugin().nppGetName();
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* pscn)
{
    GetPlugin().nppBeNotified(pscn);
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return GetPlugin().nppMessageProc(Message, wParam, lParam);
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* pnbFuncItems)
{
    return GetPlugin().nppGetFuncsArray(pnbFuncItems);
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif
