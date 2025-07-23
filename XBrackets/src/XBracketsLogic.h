#ifndef _xbracketslogic_npp_plugin_h_
#define _xbracketslogic_npp_plugin_h_
//---------------------------------------------------------------------------
#include "XBracketsCommon.h"
#include <vector>
#include <list>

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

    void setFileType(const tstr& fileExtension);

    unsigned int getAutocompleteLeftBracketType(CSciMessager& sciMsgr, const char ch) const;
    const tBrPair* getAutoCompleteBrPair(unsigned int nBracketType) const;

    bool isTreeEmpty() const;
    void buildTree(CSciMessager& sciMsgr);
    void invalidateTree();
    void updateTree(const SCNotification* pscn);

    const tBrPairItem* findPairByPos(const Sci_Position nPos, bool isExact, unsigned int* puBrPosFlags) const;
    const tBrPairItem* findParent(const tBrPairItem* pBrPair) const;

private:
    const tBrPair* getLeftBrPair(const char* p, size_t nLen) const;
    const tBrPair* getRightBrPair(const char* p, size_t nLen) const;

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
        icbfBrPair      = 0x01,
        icbfAutoRightBr = 0x02,
        icbfTree        = 0x04,

        icbfAll = (icbfBrPair | icbfAutoRightBr | icbfTree)
    };

    static const char* strBrackets[XBrackets::tbtCount];

public:
    CXBracketsLogic();

    // interaction with the plugin
    void SetNppData(const NppData& nppd);
    void UpdateFileType();
    void InvalidateCachedBrackets(unsigned int uInvalidateFlags = icbfAll, SCNotification* pscn = nullptr);
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
    Sci_Position m_nAutoRightBracketPos;
    Sci_Position m_nCachedLeftBrPos;
    Sci_Position m_nCachedRightBrPos;
    unsigned int m_uFileType;

private:
    // custom functions
    eCharProcessingResult autoBracketsFunc(unsigned int nBracketType);
    bool autoBracketsOverSelectionFunc(unsigned int nBracketType);
    bool isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, unsigned int* pnBracketType, bool bInSelection);
    unsigned int detectFileType(tstr* pFileExt = nullptr);
    unsigned int getFileType() const;
    void setFileType(unsigned int uFileType);

    bool isDoubleQuoteSupported() const;
    bool isSingleQuoteSupported() const;
    bool isTagSupported() const;
    bool isTag2Supported() const;
    bool isSkipEscapedSupported() const;

    bool isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const;

    enum eBracketOptions {
        bofIgnoreMode = 0x01
    };
    XBrackets::TBracketType getLeftBracketType(const int ch, unsigned int uOptions = 0) const;
    XBrackets::TBracketType getRightBracketType(const int ch, unsigned int uOptions = 0) const;
};

//---------------------------------------------------------------------------
#endif
