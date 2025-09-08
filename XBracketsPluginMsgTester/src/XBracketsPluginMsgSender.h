#ifndef _xbrackets_plugin_msg_sender_h_
#define _xbrackets_plugin_msg_sender_h_
//---------------------------------------------------------------------------
#include "core/NppPluginMsgSender.h"
#include "PluginCommunication/xbrackets_msgs.h"


class CXBracketsPluginMsgSender : public CNppPluginMsgSender
{
    protected:
        std::basic_string<TCHAR> m_destModuleName;

    public:
        CXBracketsPluginMsgSender(HWND hNppWnd, const TCHAR* srcModuleName, 
          const TCHAR* destModuleName = _T("XBrackets.dll")) : 
            CNppPluginMsgSender(hNppWnd, srcModuleName),
            m_destModuleName(destModuleName)
        {
        }

        void setDestModuleName(const TCHAR* destModuleName)
        {
            m_destModuleName = destModuleName;
        }

        BOOL SendXbrMsg(long internalMsg, void* info)
        {
            return SendMsg( m_destModuleName.c_str(), internalMsg, info );
        }

        BOOL XBrGetMatchingBrackets(tXBracketsPairStruct* pBrPair)
        {
            return SendXbrMsg( XBRM_GETMATCHINGBRACKETS, (void *) pBrPair );
        }

        BOOL XBrGetNearestBrackets(tXBracketsPairStruct* pBrPair)
        {
            return SendXbrMsg( XBRM_GETNEARESTBRACKETS, (void *) pBrPair );
        }
};

//---------------------------------------------------------------------------
#endif
