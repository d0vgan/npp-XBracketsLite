#ifndef _xbrackets_plugin_msg_tester_menu_h_
#define _xbrackets_plugin_msg_tester_menu_h_
//----------------------------------------------------------------------------
#include "core/NppPluginMenu.h"
//#include "resource.h"


class CMsgTesterMenu : public CNppPluginMenu
{
    public:
        enum NMenuItems {
            N_XBR_GETMATCHINGBR = 0,
            N_XBR_GETNEARESTBR,
            
            N_NBFUNCITEMS
        };

        static FuncItem arrFuncItems[N_NBFUNCITEMS];

    protected:
        static void funcXbrGetMatchingBrackets();
        static void funcXbrGetNearestBrackets();
};

//----------------------------------------------------------------------------
#endif
