#include "MsgTesterMenu.h"
#include "MsgTester.h"
#include "XBracketsPluginMsgSender.h"

FuncItem CMsgTesterMenu::arrFuncItems[N_NBFUNCITEMS] = {
    { _T("XBRM_GETMATCHINGBRACKETS"), funcXbrGetMatchingBrackets, 0, false, NULL },
    { _T("XBRM_GETNEARESTBRACKETS"),  funcXbrGetNearestBrackets,  0, false, NULL }
};

void CMsgTesterMenu::funcXbrGetMatchingBrackets()
{
    char str[100];
    tXBracketsPairStruct brPair{};

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    brPair.nPos = sciMsgr.getCurrentPos();

    CXBracketsPluginMsgSender xbrMsgr( GetMsgTester().getNppWnd(), GetMsgTester().getDllFileName().c_str() );
    xbrMsgr.XBrGetMatchingBrackets(&brPair);

    if ( brPair.pszLeftBr != nullptr )
    {
        wsprintfA( str, "leftBr = %s\nrightBr = %s\nleftBrPos = %d\nrightBrPos = %d",
            brPair.pszLeftBr, brPair.pszRightBr, brPair.nLeftBrPos, brPair.nRightBrPos );
    }
    else
    {
        wsprintfA( str, "no matching brackets found at pos %d", brPair.nPos );
    }
    ::MessageBoxA( GetMsgTester().getNppWnd(), str, "XBrackets Result: Matching Brackets", MB_OK );
}

void CMsgTesterMenu::funcXbrGetNearestBrackets()
{
    char str[100];
    tXBracketsPairStruct brPair{};

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    brPair.nPos = sciMsgr.getCurrentPos();

    CXBracketsPluginMsgSender xbrMsgr( GetMsgTester().getNppWnd(), GetMsgTester().getDllFileName().c_str() );
    xbrMsgr.XBrGetNearestBrackets(&brPair);

    if ( brPair.pszLeftBr != nullptr )
    {
        wsprintfA( str, "leftBr = %s\nrightBr = %s\nleftBrPos = %d\nrightBrPos = %d",
            brPair.pszLeftBr, brPair.pszRightBr, brPair.nLeftBrPos, brPair.nRightBrPos );
    }
    else
    {
        wsprintfA( str, "no nearest brackets found at pos %d", brPair.nPos );
    }
    ::MessageBoxA( GetMsgTester().getNppWnd(), str, "XBrackets Result: Nearest Brackets", MB_OK );
}
