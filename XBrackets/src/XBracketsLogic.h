#ifndef _xbracketslogic_npp_plugin_h_
#define _xbracketslogic_npp_plugin_h_
//---------------------------------------------------------------------------
#include "XBracketsCommon.h"
#include <vector>

class CBracketsTree
{
public:
    using tstr = XBrackets::tstr;
    using tBrPair = XBrackets::tBrPair;
    using tBrPairItem = XBrackets::tBrPairItem;
    using tFileSyntax = XBrackets::tFileSyntax;

    enum eBrPairPosFlags
    {
        bpfNone     = 0x00,
        bpfLeftBr   = 0x01, // ((
        bpfRightBr  = 0x02, // ))
        bpfBeforeBr = 0x10, // |((
        bpfAfterBr  = 0x20, // ((|
        bpfInsideBr = 0x40  // (|(
    };

public:
    CBracketsTree();

    void setFileSyntax(const tFileSyntax* pFileSyntax);

    bool isTreeEmpty() const;
    void buildTree(CSciMessager& sciMsgr);
    void invalidateTree();
    void updateTree(const SCNotification* pscn);

    const tBrPairItem* findPairByPos(const Sci_Position nPos, bool isExact, unsigned int* puBrPosFlags) const;
    const tBrPairItem* findParent(const tBrPairItem* pBrPair) const;

private:
    const tBrPair* getLeftBrPair(const char* p, size_t nLen, bool isLineStart) const;
    std::vector<const tBrPair*> getRightBrPair(const char* p, size_t nLen, size_t nCurrentOffset, bool isLineStart) const;
    bool isEscapedPos(const char* pTextBegin, const Sci_Position nPos) const;

private:
    std::vector<tBrPairItem> m_bracketsTree;
    std::vector<const tBrPairItem*> m_bracketsByRightBr;
    const tFileSyntax* m_pFileSyntax{nullptr};
};

class CXBracketsLogic
{
public:
    using tstr = XBrackets::tstr;
    using tBrPair = XBrackets::tBrPair;
    using tFileSyntax = XBrackets::tFileSyntax;

    enum eCharProcessingResult {
        cprNone = 0,
        cprBrAutoCompl,
        cprSelAutoCompl
    };

    enum eGetBracketsAction {
        baGoToMatching = 0,
        baSelToMatching,
        baGoToNearest,
        baSelToNearest
    };

    enum eInvalidateCachedBracketsFlags{
        icbfAutoRightBr = 0x01,
        icbfTree        = 0x02,

        icbfAll = (icbfAutoRightBr | icbfTree)
    };

public:
    CXBracketsLogic();

    // interaction with the plugin
    void SetNppData(const NppData& nppd);
    void UpdateFileType(unsigned int uInvalidateFlags = icbfAll);
    void InvalidateCachedBrackets(unsigned int uInvalidateFlags, SCNotification* pscn = nullptr);
    eCharProcessingResult OnChar(const int ch);
    void PerformBracketsAction(eGetBracketsAction nBrAction);

private:
    struct tGetBracketsState
    {
        Sci_Position nSelStart{-1};
        Sci_Position nSelEnd{-1};
        Sci_Position nCharPos{-1};
        Sci_Position nLeftBrPos{-1};
        Sci_Position nRightBrPos{-1};
    };

    // internal vars
    CBracketsTree m_bracketsTree;
    CNppMessager m_nppMsgr;
    const tFileSyntax* m_pFileSyntax{nullptr};
    Sci_Position m_nAutoRightBracketPos{-1};
    int          m_nAutoRightBracketType{-1};
    int          m_nAutoRightBracketOffset{-1};
    unsigned int m_uFileType{XBrackets::tfmIsSupported};

private:
    // custom functions
    int getAutocompleteLeftBracketType(CSciMessager& sciMsgr, const char ch) const;
    int getAutocompleteRightBracketType(CSciMessager& sciMsgr, const char ch) const;
    const tBrPair* getAutoCompleteBrPair(int nBracketType) const;
    eCharProcessingResult autoBracketsFunc(int nBracketType);
    bool autoBracketsOverSelectionFunc(int nBracketType);
    bool isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, int* pnBracketType, bool bInSelection);
    unsigned int detectFileType(tstr* pFileExt = nullptr);
    bool isSkipEscapedSupported() const;
    bool isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const;
};

//---------------------------------------------------------------------------
#endif
