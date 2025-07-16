#ifndef _xbracketslogic_npp_plugin_h_
#define _xbracketslogic_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/base.h"
#include "core/NppMessager.h"
#include "core/SciMessager.h"
#include <vector>

class CXBracketsLogic
{
public:
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

public:
    CXBracketsLogic();

    // interaction with the plugin
    void SetNppData(const NppData& nppd);
    void UpdateFileType();
    void InvalidateCachedBrPair();
    void InvalidateCachedAutoRightBr();
    eCharProcessingResult OnChar(const int ch);
    void PerformBracketsAction(eGetBracketsAction nBrAction);

private:
    enum TFileType2 {
        tfmNone           = 0x0000,
        tfmComment1       = 0x0001,
        tfmHtmlCompatible = 0x0002,
        tfmEscaped1       = 0x0004,
        tfmSingleQuote    = 0x0008,
        tfmIsSupported    = 0x1000
    };

    enum TBracketType {
        tbtNone = 0,
        tbtBracket,  //  (
        tbtSquare,   //  [
        tbtBrace,    //  {
        tbtDblQuote, //  "
        tbtSglQuote, //  '
        tbtTag,      //  <
        tbtTag2,     //  />

        tbtCount
    };

    enum eDupPairDirection
    {
        DP_NONE          = 0x00,
        DP_FORWARD       = 0x01,
        DP_BACKWARD      = 0x02,
        DP_DETECT        = 0x08,
        DP_MAYBEFORWARD  = (DP_DETECT | DP_FORWARD),
        DP_MAYBEBACKWARD = (DP_DETECT | DP_BACKWARD)
    };

    enum eAtBrChar {
        abcNone        = 0x00,
        abcLeftBr      = 0x01, // left:  (
        abcRightBr     = 0x02, // right: )
        abcDetectBr    = 0x04, // can be either left or right
        abcBrIsOnLeft  = 0x10, // on left:  (|  or  )|
        abcBrIsOnRight = 0x20  // on right: |(  or  |)
    };

    enum eConsts {
        MAX_ESCAPED_PREFIX  = 20
    };

    struct tGetBracketsState
    {
        Sci_Position nSelStart{-1};
        Sci_Position nSelEnd{-1};
        Sci_Position nCharPos{-1};
        Sci_Position nLeftBrPos{-1};
        Sci_Position nRightBrPos{-1};
        TBracketType nLeftBrType{tbtNone};
        TBracketType nRightBrType{tbtNone};
        eDupPairDirection nLeftDupDirection{DP_NONE};
        eDupPairDirection nRightDupDirection{DP_NONE};
    };

    struct tBracketItem
    {
        Sci_Position nBrPos{-1};
        TBracketType nBrType{tbtNone};
        eDupPairDirection nDupDir{DP_NONE};
    };

    static const char* strBrackets[tbtCount];

    // internal vars
    CNppMessager m_nppMsgr;
    Sci_Position m_nAutoRightBracketPos;
    Sci_Position m_nCachedLeftBrPos;
    Sci_Position m_nCachedRightBrPos;
    unsigned int m_uFileType;

private:
    // custom functions
    eCharProcessingResult autoBracketsFunc(TBracketType nBracketType);
    bool selAutoBrFunc(TBracketType nBracketType);
    bool isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, TBracketType* pnBracketType, bool bInSelection);
    unsigned int getFileType();

    bool isDoubleQuoteSupported() const;
    bool isSingleQuoteSupported() const;
    bool isTagSupported() const;
    bool isTag2Supported() const;
    bool isSkipEscapedSupported() const;

    enum eBracketOptions {
        bofIgnoreMode = 0x01
    };
    TBracketType getLeftBracketType(const int ch, unsigned int uOptions = 0) const;
    TBracketType getRightBracketType(const int ch, unsigned int uOptions = 0) const;

    static int getDirectionIndex(const eDupPairDirection direction);
    static int getDirectionRank(const eDupPairDirection leftDirection, const eDupPairDirection rightDirection);
    static void getEscapedPrefixPos(const Sci_Position nOffset, Sci_Position* pnPos, int* pnLen);
    static bool isInBracketsStack(const std::vector<tBracketItem>& bracketsStack, TBracketType nBrType);

    bool isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const;
    bool isDuplicatedPair(TBracketType nBracketType) const;
    eDupPairDirection getDuplicatedPairDirection(const CSciMessager& sciMsgr, const Sci_Position nCharPos, const char curr_ch) const;
    unsigned int isAtBracketCharacter(const CSciMessager& sciMsgr, const Sci_Position nCharPos, TBracketType* out_nBrType, eDupPairDirection* out_nDupDirection) const;
    bool findLeftBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state);
    bool findRightBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state);
};

//---------------------------------------------------------------------------
#endif
