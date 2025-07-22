#include "XBracketsLogic.h"
#include "XBracketsOptions.h"
#include <vector>
#include <algorithm>

#if XBR_USE_BRACKETSTREE
#include "json11/json11.hpp"
#endif

// can be _T(x), but _T(x) may be incompatible with ANSI mode
#define _TCH(x)  (x)

extern CXBracketsOptions g_opt;

namespace
{
    bool isEscapedPrefix(const char* str, const int len)
    {
        bool isEscaped = false;
        const char* p = str + len;
        while ( (p != str) && (*(--p) == '\\') )
        {
            isEscaped = !isEscaped;
        }
        return isEscaped;
    }

    bool isWhiteSpaceOrNulChar(const char ch)
    {
        switch ( ch )
        {
        case '\x00':
        case '\x0D':
        case '\x0A':
        case ' ':
        case '\t':
            return true;
        }

        return false;
    }

    bool isSepOrOneOf(const char ch, const char* pChars)
    {
        return ( isWhiteSpaceOrNulChar(ch) || strchr(pChars, ch) != NULL );
    }

    bool isSentenceEndChar(const char ch)
    {
        switch ( ch )
        {
        case '.':
        case '?':
        case '!':
            return true;
        }
        return false;
    }

    std::vector<char> readFile(const TCHAR* filePath)
    {
        HANDLE hFile = ::CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            // ERROR: Could not open the file
            return {};
        }

        DWORD nBytesRead = 0;
        const DWORD nFileSize = ::GetFileSize(hFile, NULL);
        std::vector<char> buf(nFileSize + 1);
        const BOOL isRead = ::ReadFile(hFile, buf.data(), nFileSize, &nBytesRead, NULL);

        ::CloseHandle(hFile);

        if ( !isRead || nFileSize != nBytesRead )
        {
            // ERROR: Could not read the file
            return {};
        }

        buf[nFileSize] = 0; // the trailing '\0'

        return buf;
    }

    tstr string_to_tstr(const std::string& str)
    {
    #ifdef _UNICODE
        if ( str.empty() )
            return tstr();

        std::vector<TCHAR> buf(str.length() + 1);
        buf[0] = 0;
        ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), buf.data(), static_cast<int>(str.length() + 1));
        return tstr(buf.data(), str.length());
    #else
        return str;
    #endif
    }
}

const char* CBracketsCommon::strBrackets[tbtCount] = {
    "",     // tbtNone
    "()",   // tbtBracket
    "[]",   // tbtSquare
    "{}",   // tbtBrace
    "\"\"", // tbtDblQuote
    "\'\'", // tbtSglQuote
    "<>",   // tbtTag
    "</>"   // tbtTag2
};

CBracketsCommon::CBracketsCommon() : m_uFileType(tfmIsSupported)
{
}

CBracketsCommon::~CBracketsCommon()
{
}

unsigned int CBracketsCommon::getFileType() const
{
    return m_uFileType;
}

void CBracketsCommon::setFileType(unsigned int uFileType)
{
    m_uFileType = uFileType;
}

bool CBracketsCommon::isDoubleQuoteSupported() const
{
    return g_opt.getBracketsDoDoubleQuote();
}

bool CBracketsCommon::isSingleQuoteSupported() const
{
    return ( g_opt.getBracketsDoSingleQuote() &&
             ((m_uFileType & tfmSingleQuote) != 0 || !g_opt.getBracketsDoSingleQuoteIf()) );
}

bool CBracketsCommon::isTagSupported() const
{
    return ( g_opt.getBracketsDoTag() &&
             ((m_uFileType & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf()) );
}

bool CBracketsCommon::isTag2Supported() const
{
    return ( g_opt.getBracketsDoTag2() &&
             ((m_uFileType & tfmHtmlCompatible) != 0 || !g_opt.getBracketsDoTagIf()) );
}

bool CBracketsCommon::isSkipEscapedSupported() const
{
    return ( g_opt.getBracketsSkipEscaped() && (m_uFileType & tfmEscaped1) != 0 );
}

bool CBracketsCommon::isDuplicatedPair(TBracketType nBracketType) const
{
    // left bracket == right bracket
    return (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote);
}

bool CBracketsCommon::isEnquotingPair(TBracketType nBracketType) const
{
    // can enquote text
    return (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote);
}

CBracketsCommon::TBracketType CBracketsCommon::getLeftBracketType(const int ch, unsigned int uOptions) const
{
    TBracketType nLeftBracketType = tbtNone;

    // OK for both ANSI and Unicode (ch can be wide character)
    switch ( ch )
    {
    case _TCH('(') :
        nLeftBracketType = tbtBracket;
        break;
    case _TCH('[') :
        nLeftBracketType = tbtSquare;
        break;
    case _TCH('{') :
        nLeftBracketType = tbtBrace;
        break;
    case _TCH('\"') :
        if ( (uOptions & bofIgnoreMode) != 0 || isDoubleQuoteSupported() )
        {
            nLeftBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 || isSingleQuoteSupported() )
        {
            nLeftBracketType = tbtSglQuote;
        }
        break;
    case _TCH('<') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTagSupported() )
        {
            nLeftBracketType = tbtTag;
        }
        break;
    }

    return nLeftBracketType;
}

CBracketsCommon::TBracketType CBracketsCommon::getRightBracketType(const int ch, unsigned int uOptions) const
{
    TBracketType nRightBracketType = tbtNone;

    // OK for both ANSI and Unicode (ch can be wide character)
    switch ( ch )
    {
    case _TCH(')') :
        nRightBracketType = tbtBracket;
        break;
    case _TCH(']') :
        nRightBracketType = tbtSquare;
        break;
    case _TCH('}') :
        nRightBracketType = tbtBrace;
        break;
    case _TCH('\"') :
        if ( (uOptions & bofIgnoreMode) != 0 || isDoubleQuoteSupported() )
        {
            nRightBracketType = tbtDblQuote;
        }
        break;
    case _TCH('\'') :
        if ( (uOptions & bofIgnoreMode) != 0 || isSingleQuoteSupported() )
        {
            nRightBracketType = tbtSglQuote;
        }
        break;
    case _TCH('>') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTagSupported() )
        {
            nRightBracketType = tbtTag;
        }
        // no break here
    case _TCH('/') :
        if ( (uOptions & bofIgnoreMode) != 0 || isTag2Supported() )
        {
            nRightBracketType = tbtTag2;
        }
        break;
    }

    return nRightBracketType;
}

int CBracketsCommon::getDirectionIndex(const eDupPairDirection direction)
{
    int i = 0;

    switch (direction)
    {
    case DP_BACKWARD:
        i = 0;
        break;
    case DP_MAYBEBACKWARD:
        i = 1;
        break;
    case DP_DETECT:
        i = 2;
        break;
    case DP_MAYBEFORWARD:
        i = 3;
        break;
    case DP_FORWARD:
        i = 4;
        break;
    }

    return i;
}

int CBracketsCommon::getDirectionRank(const eDupPairDirection leftDirection, const eDupPairDirection rightDirection)
{
    //  The *exact* numbers in the table below have no special meaning.
    //  The only important thing is relative comparison of the numbers:
    //  which of them are greater than others, and which of them are equal.
    //
    // --> leftDirection --> DP_BACKWARD  DP_MAYBEBACKWARD   DP_DETECT      DP_MAYBEFORWARD   DP_FORWARD
    //    DP_FORWARD         <<( )>>  0   <?( )>>   1        ?(? )>>   2    (?> )>>   3       (>> )>>   4
    //    DP_MAYBEFORWARD    <<( )?>  1   <?( )?>   5        ?(? )?>   9    (?> )?>  13       (>> )?>  17
    //    DP_DETECT          <<( ?)?  2   <?( ?)?   9        ?(? ?)?  16    (?> ?)?  23       (>> ?)?  30
    //    DP_MAYBEBACKWARD   <<( <?)  3   <?( <?)  13        ?(? <?)  23    (?> <?)  33       (>> <?)  43
    //    DP_BACWARD         <<( <<)  4   <?( <<)  17        ?(? <<)  30    (?> <<)  43       (>> <<)  56
    // ^ rightDirection ^
    static const int Ranking[5][5] = {
        { 4, 17, 30, 43, 56 }, // DP_BACKWARD
        { 3, 13, 23, 33, 43 }, // DP_MAYBEBACKWARD
        { 2,  9, 16, 23, 30 }, // DP_DETECT
        { 1,  5,  9, 13, 17 }, // DP_MAYBEFORWARD
        { 0,  1,  2,  3,  4 }  // DP_FORWARD
    };

    int leftIndex = getDirectionIndex(leftDirection);
    int rightIndex = getDirectionIndex(rightDirection);

    return Ranking[rightIndex][leftIndex];
}

void CBracketsCommon::getEscapedPrefixPos(const Sci_Position nOffset, Sci_Position* pnPos, int* pnLen)
{
    if ( nOffset > MAX_ESCAPED_PREFIX )
    {
        *pnPos = nOffset - MAX_ESCAPED_PREFIX;
        *pnLen = MAX_ESCAPED_PREFIX;
    }
    else
    {
        *pnPos = 0;
        *pnLen = static_cast<int>(nOffset);
    }
}

//-----------------------------------------------------------------------------

#if XBR_USE_BRACKETSTREE
CBracketsTree::CBracketsTree()
{
}

static CBracketsTree::tBrPair readBrPairItem(const json11::Json& pairItem)
{
    CBracketsTree::tBrPair brPair;

    for ( const auto& elem : pairItem.array_items() )
    {
        if ( elem.is_string() )
        {
            const std::string& val = elem.string_value();

            CBracketsTree::eBrPairKind kind = CBracketsTree::bpkNone;
            if ( brPair.kind == CBracketsTree::bpkNone )
            {
                if ( val == "brackets" )
                    kind = CBracketsTree::bpkBrackets;
                else if ( val == "single-line-quotes" )
                    kind = CBracketsTree::bpkSgLnQuotes;
                else if ( val == "multi-line-quotes" )
                    kind = CBracketsTree::bpkMlLnQuotes;
                else if ( val == "single-line-comment" )
                    kind = CBracketsTree::bpkSgLnComm;
                else if ( val == "multi-line-comment" )
                    kind = CBracketsTree::bpkMlLnComm;
                else if ( val == "escape-char" )
                    kind = CBracketsTree::bpkEsqChar;
            }

            if ( kind != CBracketsTree::bpkNone )
                brPair.kind = kind;
            else if ( brPair.leftBr.empty() )
                brPair.leftBr = val;
            else if ( brPair.rightBr.empty() && brPair.kind != CBracketsTree::bpkEsqChar )
                brPair.rightBr = val;
        }
    }

    return brPair;
}

static CBracketsTree::tFileSyntax readFileSyntaxItem(const json11::Json& syntaxItem)
{
    CBracketsTree::tFileSyntax fileSyntax;

    for ( const auto& propItem : syntaxItem.object_items() )
    {
        if ( propItem.first == "name" )
        {
            if ( propItem.second.is_string() )
            {
                fileSyntax.name = propItem.second.string_value();
            }
        }
        else if ( propItem.first == "fileExtensions" )
        {
            if ( propItem.second.is_array() )
            {
                for ( const auto& fileExtItem : propItem.second.array_items() )
                {
                    if ( fileExtItem.is_string() )
                    {
                        fileSyntax.fileExtensions.insert(string_to_tstr(fileExtItem.string_value()));
                    }
                }
            }
        }
        else if ( propItem.first == "elements" )
        {
            if ( propItem.second.is_array() )
            {
                for ( const auto& elementItem : propItem.second.array_items() )
                {
                    if ( elementItem.is_array() )
                    {
                        CBracketsTree::tBrPair brPair = readBrPairItem(elementItem);

                        if ( brPair.kind != CBracketsTree::bpkNone )
                        {
                            if ( brPair.kind != CBracketsTree::bpkEsqChar )
                                fileSyntax.pairs.push_back(std::move(brPair));
                            else
                                fileSyntax.qtEsc.push_back(std::move(brPair.leftBr));
                        }
                    }
                }
            }
        }
    }

    return fileSyntax;
}

void CBracketsTree::readConfig(const tstr& cfgFilePath)
{
    using namespace json11;

    const auto jsonBuf = readFile(cfgFilePath.c_str());

    std::string err;
    const auto jsonObj = json11::Json::parse(jsonBuf.data(), err);
    if ( jsonObj.is_null() )
        return;

    if ( jsonObj.is_object() )
    {
        for ( const auto& item : jsonObj.object_items() )
        {
            if ( item.first == "fileSyntax" )
            {
                if ( item.second.is_array() )
                {
                    for ( const auto& syntaxItem : item.second.array_items() )
                    {
                        if ( syntaxItem.is_object() )
                        {
                            m_fileSyntaxes.push_back(readFileSyntaxItem(syntaxItem));
                        }
                    }
                }
            }
        }
    }
}

void CBracketsTree::setFileType(unsigned int uFileType, const tstr& fileExtension)
{
    CBracketsCommon::setFileType(uFileType);

    m_pFileSyntax = nullptr;
    for ( const auto& syntax: m_fileSyntaxes )
    {
        if ( syntax.fileExtensions.find(fileExtension) != syntax.fileExtensions.end() )
        {
            m_pFileSyntax = &syntax;
            break;
        }
    }
}

bool CBracketsTree::isTreeEmpty() const
{
    return m_bracketsTree.empty();
}

void CBracketsTree::buildTree(CSciMessager& sciMsgr)
{
    if ( m_pFileSyntax == nullptr )
        return; // nothing to do

    std::vector<tBrPairItem> bracketsTree;
    std::vector<tBrPairItem> unmatchedBrackets;

    auto addToBracketsTree = [&bracketsTree](tBrPairItem& item)
    {
        // Currently item.nParentLeftBrPos is the size of bracketsTree at
        // the moment when this item has been created.
        // Thus, all the items starting from the index item.nParentLeftBrPos
        // are potentially children of this item.
        const auto itrEnd = bracketsTree.end();
        for ( auto itr = bracketsTree.begin() + item.nParentLeftBrPos; itr != itrEnd; ++itr )
        {
            if ( itr->nParentLeftBrPos == -1 )
                itr->nParentLeftBrPos = item.nLeftBrPos;
        }

        item.nParentLeftBrPos = -1;
        bracketsTree.push_back(item);
    };

    Sci_Position nTextLen = sciMsgr.getTextLength();
    std::vector<char> vText(nTextLen + 1);
    sciMsgr.getText(nTextLen, vText.data());
    vText[nTextLen] = 0;

    Sci_Position nPos = 0;
    Sci_Position nLine = 0;
    for ( const char* p = vText.data(); p < vText.data() + nTextLen; ++p, ++nPos )
    {
        bool isNewLine = false;
        if ( *p == '\r' )
        {
            if ( nPos < nTextLen && *(p + 1) == '\n' )
            {
                ++p;
                ++nPos;
            }
            isNewLine = true;
        }
        else if ( *p == '\n' )
        {
            isNewLine = true;
        }

        if ( isNewLine )
        {
            const tBrPairItem* pBegin = unmatchedBrackets.data();
            const tBrPairItem* pEnd = pBegin + unmatchedBrackets.size();

            if ( pBegin != pEnd )
            {
                const tBrPairItem* pDelSgQtItem = pEnd;
                const tBrPairItem* pDelSgCommItem = pEnd;

                for ( const tBrPairItem* pItem = pEnd - 1; ; --pItem )
                {
                    if ( pItem->nLine != nLine )
                        break;

                    const eBrPairKind kind = pItem->pBrPair->kind;
                    if ( kind == bpkSgLnQuotes )
                        pDelSgQtItem = pItem;
                    else if ( kind == bpkSgLnComm )
                        pDelSgCommItem = pItem;

                    if ( pItem == pBegin)
                        break;
                }

                if ( pDelSgCommItem != pEnd )
                {
                    // removing unmatched single-line commented text
                    const auto itrDel = unmatchedBrackets.begin() + static_cast<size_t>(pDelSgCommItem - pBegin);
                    unmatchedBrackets.erase(itrDel, unmatchedBrackets.end());
                }

                if ( pDelSgQtItem != pEnd && (pDelSgCommItem == pEnd || pDelSgQtItem < pDelSgCommItem) )
                {
                    // removing unmatched single-line quoted text
                    const auto itrBegin = unmatchedBrackets.begin();
                    pEnd = pBegin + unmatchedBrackets.size();
                    for ( auto pItem = pEnd - 1; ; --pItem )
                    {
                        const eBrPairKind kind = pItem->pBrPair->kind;
                        if ( kind == bpkSgLnQuotes )
                            unmatchedBrackets.erase(itrBegin + static_cast<size_t>(pItem - pBegin));

                        if ( pItem == pDelSgQtItem)
                            break;
                    }
                }
            }

            ++nLine;
            continue;
        }

        const tBrPair* pBrPair = getLeftBrPair(p, nTextLen - nPos);
        if ( pBrPair != nullptr )
        {
            if ( pBrPair->leftBr == pBrPair->rightBr )
            {
                // either left or right bracket/quote
                const eBrPairKind kind = pBrPair->kind;
                if ( kind == bpkSgLnQuotes || kind == bpkMlLnQuotes || kind == bpkMlLnComm )
                {
                    // enquoting brackets pair
                    const auto itrBegin = unmatchedBrackets.rbegin();
                    const auto itrEnd = unmatchedBrackets.rend();
                    auto itrItem = itrEnd;
                    auto itrDel = itrEnd;
                    for ( auto itr = itrBegin; itr != itrEnd; ++itr )
                    {
                        if ( itr->pBrPair->leftBr == pBrPair->leftBr )
                        {
                            itrItem = itr;
                            break;
                        }
                        if ( kind == bpkMlLnComm && itr->pBrPair->kind == bpkMlLnComm )
                        {
                            itrDel = itr;
                        }
                    }
                    if ( itrItem != itrEnd )
                    {
                        if ( itrDel == itrEnd )
                        {
                            const eBrPairKind kind = itrItem->pBrPair->kind;
                            if ( (kind != bpkSgLnQuotes && kind != bpkMlLnQuotes) || !isEscapedPos(vText.data(), nPos) )
                            {
                                // itrItem points to an unmatched left bracket, nPos is its right bracket
                                itrItem->nRightBrPos = nPos; // |)
                                addToBracketsTree(*itrItem);
                                // removing the enquoted unmatched brackets:
                                unmatchedBrackets.erase(std::next(itrItem).base(), unmatchedBrackets.end());
                            }
                        }
                    }
                    else
                    {
                        // left bracket
                        const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length());
                        unmatchedBrackets.push_back({nLeftBrPos, -1, nLine, static_cast<Sci_Position>(bracketsTree.size()), pBrPair}); // (|
                    }
                }
                else
                {
                    // non-enquoting brackets pair
                    if ( !unmatchedBrackets.empty() )
                    {
                        auto& item = unmatchedBrackets.back();
                        if ( item.pBrPair->leftBr == pBrPair->leftBr )
                        {
                            // item is an unmatched left bracket, nPos is its right bracket
                            item.nRightBrPos = nPos; // |)
                            addToBracketsTree(item);

                            // removing the enquoted unmatched brackets:
                            unmatchedBrackets.pop_back();
                        }
                        else
                        {
                            // left bracket
                            const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length());
                            unmatchedBrackets.push_back({nLeftBrPos, -1, nLine, static_cast<Sci_Position>(bracketsTree.size()), pBrPair}); // (|
                        }
                    }
                    else
                    {
                        // left bracket
                        const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length());
                        unmatchedBrackets.push_back({nLeftBrPos, -1, nLine, static_cast<Sci_Position>(bracketsTree.size()), pBrPair}); // (|
                    }
                }
            }
            else
            {
                // left bracket
                const auto itrEnd = unmatchedBrackets.rend();
                auto itr = itrEnd;
                if ( pBrPair->kind == bpkMlLnComm )
                {
                    const auto itrBegin = unmatchedBrackets.rbegin();
                    itr = std::find_if(itrBegin, itrEnd, [pBrPair](const tBrPairItem& item){
                        const eBrPairKind kind = item.pBrPair->kind;
                        return (kind == bpkMlLnQuotes || (kind == bpkMlLnComm && item.pBrPair->leftBr == pBrPair->leftBr));
                    });
                }
                if ( itr == itrEnd )
                {
                    const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length());
                    unmatchedBrackets.push_back({nLeftBrPos, -1, nLine, static_cast<Sci_Position>(bracketsTree.size()), pBrPair}); // (|
                }
            }

            if ( pBrPair->leftBr.length() > 1 )
            {
                p += (pBrPair->leftBr.length() - 1);
                nPos += (pBrPair->leftBr.length() - 1);
            }
        }
        else
        {
            pBrPair = getRightBrPair(p, nTextLen - nPos);
            if ( pBrPair != nullptr )
            {
                // right bracket
                if ( !unmatchedBrackets.empty() )
                {
                    const auto itrBegin = unmatchedBrackets.rbegin();
                    const auto itrEnd = unmatchedBrackets.rend();
                    auto itrItem = itrEnd;
                    auto itrDel = itrEnd;
                    for ( auto itr = itrBegin; itr != itrEnd; ++itr )
                    {
                        if ( itr->pBrPair->leftBr == pBrPair->leftBr )
                        {
                            itrItem = itr;
                            break;
                        }

                        const eBrPairKind kind = itr->pBrPair->kind;
                        if ( kind == bpkMlLnComm || ((kind == bpkSgLnQuotes || kind == bpkMlLnQuotes) && pBrPair->kind == bpkBrackets) )
                        {
                            itrDel = itr;
                        }
                    }
                    if ( itrItem != itrEnd )
                    {
                        if ( itrDel == itrEnd )
                        {
                            // itrItem points to an unmatched left bracket, nPos is its right bracket
                            itrItem->nRightBrPos = nPos; // |)
                            addToBracketsTree(*itrItem);
                            // removing the enquoted unmatched brackets:
                            unmatchedBrackets.erase(std::next(itrItem).base(), unmatchedBrackets.end());
                        }
                    }
                }

                if ( pBrPair->rightBr.length() > 1 )
                {
                    p += (pBrPair->rightBr.length() - 1);
                    nPos += (pBrPair->leftBr.length() - 1);
                }
            }
        }
    }

    std::sort(bracketsTree.begin(), bracketsTree.end(), 
        [](const tBrPairItem& item1, const tBrPairItem& item2){
            return item1.nLeftBrPos < item2.nLeftBrPos;
    });

    std::vector<const tBrPairItem*> bracketsByRightBr;

    size_t n = bracketsTree.size();
    bracketsByRightBr.reserve(n);
    for ( const auto& item : bracketsTree )
    {
        auto itr = std::lower_bound(bracketsByRightBr.begin(), bracketsByRightBr.end(), item.nRightBrPos,
            [](const tBrPairItem* pItem, Sci_Position nRightBrPos) { return pItem->nRightBrPos < nRightBrPos; });
        bracketsByRightBr.insert(itr, &item);
    }

    m_bracketsTree.swap(bracketsTree);
    m_bracketsByRightBr.swap(bracketsByRightBr);
}

void CBracketsTree::invalidateTree()
{
    m_bracketsTree.clear();
    m_bracketsByRightBr.clear();
}

void CBracketsTree::updateTree(const SCNotification* pscn)
{
    if ( m_pFileSyntax == nullptr || m_bracketsTree.empty() )
        return; // nothing to do

    if ( (pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) == 0 )
        return; // unsupported modification type

    bool needsInvalidate = false;

    // 1. checking the document's text around pscn->position
    CSciMessager sciMsgr(reinterpret_cast<HWND>(pscn->nmhdr.hwndFrom));

    const Sci_Position nTextLen = sciMsgr.getTextLength();
    Sci_Position nPos1 = pscn->position >= 8 ? pscn->position - 8 : 0;
    Sci_Position nPos2 = pscn->position + 8 < nTextLen ? pscn->position + 8 : nTextLen;
    size_t nOffset = pscn->position >= 8 ? 0 : 8 - pscn->position;

    char szText[18]{};
    sciMsgr.getTextRange(nPos1, nPos2, szText + nOffset);

    for ( auto& item : m_pFileSyntax->pairs )
    {
        size_t nBrLen = item.leftBr.length();
        if ( nBrLen > 1 )
        {
            const char* p = strstr(szText + 9 - nBrLen, item.leftBr.c_str());
            if ( p != nullptr && p < szText + 8 ) // inside the left bracket
            {
                needsInvalidate = true;
                break;
            }
        }

        nBrLen = item.rightBr.length();
        if ( nBrLen > 1 )
        {
            const char* p = strstr(szText + 9 - nBrLen, item.rightBr.c_str());
            if ( p != nullptr && p < szText + 8 ) // inside the right bracket
            {
                needsInvalidate = true;
                break;
            }
        }
    }

    if ( pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT) )
    {
        if ( !needsInvalidate )
        {
            // 2. checking for inserted/deleted new line
            const char* p = pscn->text;
            const char* const pEnd = p + pscn->length;

            for ( ; p != pEnd; ++p )
            {
                const char ch = *p;
                if ( ch == '\n' || ch == '\r' )
                {
                    needsInvalidate = true;
                    break;
                }
            }
        }

        if ( !needsInvalidate )
        {
            // 3. checking for inserted/deleted brackets or quotes
            const char* p = pscn->text;
            const char* const pEnd = p + pscn->length;

            for ( ; p != pEnd && !needsInvalidate; ++p )
            {
                for ( auto& item : m_pFileSyntax->pairs )
                {
                    size_t nBrLen = item.leftBr.length();
                    if ( nBrLen != 0 && p + nBrLen <= pEnd && strstr(p, item.leftBr.c_str()) != nullptr )
                    {
                        needsInvalidate = true;
                        break;
                    }

                    nBrLen = item.rightBr.length();
                    if ( nBrLen != 0 && p + nBrLen <= pEnd && strstr(p, item.rightBr.c_str()) != nullptr )
                    {
                        needsInvalidate = true;
                        break;
                    }
                }
            }
        }
    }

    if ( needsInvalidate )
    {
        invalidateTree();
        return; // tree needs to be recreated
    }

    const Sci_Position pos = pscn->position;
    const Sci_Position len = pscn->length;

    if ( pscn->modificationType & SC_MOD_INSERTTEXT )
    {
        for ( auto& item : m_bracketsTree )
        {
            if ( item.nLeftBrPos >= pos )
            {
                item.nLeftBrPos += len;
                item.nRightBrPos += len;
            }
            else if ( item.nRightBrPos >= pos )
            {
                item.nRightBrPos += len;
            }
        }
    }
    else if ( pscn->modificationType & SC_MOD_DELETETEXT )
    {
        for ( auto& item : m_bracketsTree )
        {
            if ( item.nLeftBrPos >= pos )
            {
                item.nLeftBrPos -= len;
                item.nRightBrPos -= len;
            }
            else if ( item.nRightBrPos >= pos )
            {
                item.nRightBrPos -= len;
            }
        }
    }
}

const CBracketsTree::tBrPairItem* CBracketsTree::findPairByPos(const Sci_Position nPos, bool isExact, unsigned int* puBrPosFlags) const
{
    *puBrPosFlags = bpfNone;

    if ( m_bracketsTree.empty() )
        return nullptr;

    const auto itrRightBegin = m_bracketsByRightBr.begin();
    const auto itrRightEnd = m_bracketsByRightBr.end();
    auto itrRightLocalEnd = itrRightEnd;
    auto itrRightWithin = itrRightEnd;
    auto itrRight = std::lower_bound(itrRightBegin, itrRightEnd, nPos,
        [](const tBrPairItem* pItem, Sci_Position nPos) { return pItem->nRightBrPos < nPos; });

    if ( itrRight == itrRightEnd )
    {
        --itrRight; // try the very last item, in case of ))| or )|)
    }
    else
    {
        itrRightLocalEnd = itrRight + 1; // stop right after the itrRight
        if ( itrRight != itrRightBegin )
            --itrRight; // in case of ))| or )|)
    }

    for ( ; itrRight != itrRightLocalEnd; ++itrRight )
    {
        unsigned int uBrPosFlags = bpfNone;
        const Sci_Position nBeforeRightBrPos = (*itrRight)->nRightBrPos;
        const Sci_Position nAfterRightBrPos = nBeforeRightBrPos + static_cast<Sci_Position>((*itrRight)->pBrPair->rightBr.length());

        if ( nPos == nBeforeRightBrPos )
            uBrPosFlags = (bpfRightBr | bpfBeforeBr); // |))
        else if ( nPos == nAfterRightBrPos )
            uBrPosFlags = (bpfRightBr | bpfAfterBr); // ))|
        else if ( nPos > nBeforeRightBrPos && nPos < nAfterRightBrPos )
            uBrPosFlags = (bpfRightBr | bpfInsideBr); // )|)

        if ( uBrPosFlags != bpfNone )
        {
            *puBrPosFlags = uBrPosFlags;
            return (*itrRight);
        }

        if ( !isExact && nPos > (*itrRight)->nLeftBrPos && nPos < (*itrRight)->nRightBrPos )
        {
            if ( itrRightWithin == itrRightEnd ||
                 ((*itrRightWithin)->nRightBrPos - (*itrRightWithin)->nLeftBrPos > (*itrRight)->nRightBrPos - (*itrRight)->nLeftBrPos) )
            {
                itrRightWithin = itrRight;
            }
        }
    }

    const auto itrLeftBegin = m_bracketsTree.begin();
    const auto itrLeftEnd = m_bracketsTree.end();
    auto itrLeftWithin = itrLeftEnd;
    const auto itrLeftFound = std::lower_bound(itrLeftBegin, itrLeftEnd, nPos,
        [](const tBrPairItem& item, Sci_Position nPos) { return item.nLeftBrPos < nPos; });

    if ( itrLeftFound == itrLeftEnd && isExact )
        return nullptr;

    auto itrLeft = itrLeftFound;
    if ( !isExact && itrLeft != itrLeftBegin )
        --itrLeft;

    for ( ; itrLeft != itrLeftEnd; ++itrLeft )
    {
        unsigned int uBrPosFlags = bpfNone;
        const Sci_Position nAfterLeftBrPos = itrLeft->nLeftBrPos;
        const Sci_Position nBeforeLeftBrPos = nAfterLeftBrPos - static_cast<Sci_Position>(itrLeft->pBrPair->leftBr.length());
        if ( nBeforeLeftBrPos > nPos )
            break;

        if ( nPos == nAfterLeftBrPos )
            uBrPosFlags = (bpfLeftBr | bpfAfterBr); // ((|
        else if ( nPos == nBeforeLeftBrPos )
            uBrPosFlags = (bpfLeftBr | bpfBeforeBr); // |((
        else if ( nPos > nBeforeLeftBrPos && nPos < nAfterLeftBrPos )
            uBrPosFlags = (bpfLeftBr | bpfInsideBr); // (|(

        if ( uBrPosFlags != bpfNone )
        {
            *puBrPosFlags = uBrPosFlags;
            return &(*itrLeft);
        }

        if ( !isExact && nPos > itrLeft->nLeftBrPos && nPos < itrLeft->nRightBrPos )
        {
            if ( itrLeftWithin == itrLeftEnd ||
                 (itrLeftWithin->nRightBrPos - itrLeftWithin->nLeftBrPos > itrLeft->nRightBrPos - itrLeft->nLeftBrPos) )
            {
                itrLeftWithin = itrLeft;
            }
        }
    }

    if ( !isExact )
    {
        if ( itrLeftWithin != itrLeftEnd )
        {
            if ( itrRightWithin == itrRightEnd ||
                 ((*itrRightWithin)->nRightBrPos - (*itrRightWithin)->nLeftBrPos > itrLeftWithin->nRightBrPos - itrLeftWithin->nLeftBrPos) )
                return &(*itrLeftWithin);
        }

        if ( itrRightWithin != itrRightEnd )
            return (*itrRightWithin);

        if ( itrLeftFound != itrLeftEnd )
            return findParent(&(*itrLeftFound));
    }

    return nullptr;
}

const CBracketsTree::tBrPairItem* CBracketsTree::findParent(const tBrPairItem* pBrPair) const
{
    if ( pBrPair == nullptr || pBrPair->nParentLeftBrPos == -1 )
        return nullptr;

    const auto itrLeftBegin = m_bracketsTree.begin();
    const auto itrLeftEnd = m_bracketsTree.end();
    const auto itrLeft = std::lower_bound(itrLeftBegin, itrLeftEnd, pBrPair->nParentLeftBrPos,
        [](const tBrPairItem& item, Sci_Position nPos) { return item.nLeftBrPos < nPos; });

    if ( itrLeft == itrLeftEnd )
        return nullptr;

    return &(*itrLeft);
}

const CBracketsTree::tBrPair* CBracketsTree::getLeftBrPair(const char* p, size_t nLen) const
{
    if ( m_pFileSyntax == nullptr )
        return nullptr;

    for ( const auto& brPair : m_pFileSyntax->pairs )
    {
        if ( brPair.leftBr.length() <= nLen )
        {
            if ( memcmp(p, brPair.leftBr.c_str(), brPair.leftBr.length()) == 0 )
                return &brPair;
        }
    }

    return nullptr;
}

const CBracketsTree::tBrPair* CBracketsTree::getRightBrPair(const char* p, size_t nLen) const
{
    if ( m_pFileSyntax == nullptr )
        return nullptr;

    for ( const auto& brPair : m_pFileSyntax->pairs )
    {
        if ( brPair.kind != bpkSgLnComm && brPair.rightBr.length() <= nLen )
        {
            if ( memcmp(p, brPair.rightBr.c_str(), brPair.rightBr.length()) == 0 )
                return &brPair;
        }
    }

    return nullptr;
}

bool CBracketsTree::isEscapedPos(const char* pTextBegin, const Sci_Position nPos) const
{
    if ( m_pFileSyntax == nullptr || m_pFileSyntax->qtEsc.empty() )
        return false;

    Sci_Position nPrefixPos{};
    int nPrefixLen{}; // len <= MAX_ESCAPED_PREFIX, so 'int' is always enough

    getEscapedPrefixPos(nPos, &nPrefixPos, &nPrefixLen);
    
    const char* prefix = pTextBegin + nPrefixPos;
    bool isEscaped = false;

    for ( const auto& esqStr : m_pFileSyntax->qtEsc )
    {
        isEscaped = false;

        for ( const char* p = prefix + nPrefixLen; ; )
        {
            p -= esqStr.length();
            if ( p < prefix || memcmp(p, esqStr.c_str(), esqStr.length()) != 0 )
                break;

            isEscaped = !isEscaped;
        }

        if ( isEscaped )
            break;
    }

    return isEscaped;
}
#endif

//-----------------------------------------------------------------------------

CXBracketsLogic::CXBracketsLogic() :
    m_nAutoRightBracketPos(-1),
    m_nCachedLeftBrPos(-1),
    m_nCachedRightBrPos(-1)
{
}

void CXBracketsLogic::SetNppData(const NppData& nppd)
{
    m_nppMsgr.setNppData(nppd);
}

void CXBracketsLogic::ReadConfig(const tstr& cfgFilePath)
{
    m_bracketsTree.readConfig(cfgFilePath);
}

void CXBracketsLogic::InvalidateCachedBrackets(unsigned int uInvalidateFlags, SCNotification* pscn)
{
    if ( uInvalidateFlags & icbfBrPair )
    {
        m_nCachedLeftBrPos = -1;
        m_nCachedRightBrPos = -1;
    }
    if ( uInvalidateFlags & icbfAutoRightBr )
    {
        m_nAutoRightBracketPos = -1;
    }
#if XBR_USE_BRACKETSTREE
    if ( uInvalidateFlags & icbfTree )
    {
        if ( pscn == nullptr || (pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) == 0 )
            m_bracketsTree.invalidateTree();
        else
            m_bracketsTree.updateTree(pscn);
    }
#endif
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::OnChar(const int ch)
{
    const Sci_Position nAutoRightBrPos = m_nAutoRightBracketPos; // will be used below

    InvalidateCachedBrackets(icbfBrPair | icbfAutoRightBr);

    if ( !g_opt.getBracketsAutoComplete() )
        return cprNone;

    if ( (getFileType() & tfmIsSupported) == 0 )
        return cprNone;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return cprNone; // nothing to do with multiple selections

    if ( nAutoRightBrPos >= 0 )
    {
        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        TBracketType nRightBracketType = getRightBracketType(ch);

        if ( nRightBracketType != tbtNone )
        {
            Sci_Position pos = sciMsgr.getCurrentPos();

            if ( pos == nAutoRightBrPos )
            {
                // previous character
                char prev_ch = sciMsgr.getCharAt(pos - 1);
                if ( prev_ch == strBrackets[nRightBracketType][0] )
                {
                    char next_ch = sciMsgr.getCharAt(pos);
                    if ( next_ch == strBrackets[nRightBracketType][1] )
                    {
                        ++pos;
                        if ( nRightBracketType == tbtTag2 )
                            ++pos;
                        sciMsgr.setSel(pos, pos);

                        return cprBrAutoCompl;
                    }
                }
            }
        }
    }

    TBracketType nLeftBracketType = getLeftBracketType(ch);
    if ( nLeftBracketType == tbtNone )
        return cprNone;

    // a typed character is a bracket
    return autoBracketsFunc(nLeftBracketType);
}

bool CXBracketsLogic::isEscapedPos(const CSciMessager& sciMsgr, const Sci_Position nCharPos) const
{
    if ( !isSkipEscapedSupported() )
        return false;

    char szPrefix[MAX_ESCAPED_PREFIX + 2];
    Sci_Position nPrefixPos{};
    int nPrefixLen{}; // len <= MAX_ESCAPED_PREFIX, so 'int' is always enough

    getEscapedPrefixPos(nCharPos, &nPrefixPos, &nPrefixLen);
    nPrefixLen = static_cast<int>(sciMsgr.getTextRange(nPrefixPos, nPrefixPos + nPrefixLen, szPrefix));
    return isEscapedPrefix(szPrefix, nPrefixLen);
}

CXBracketsLogic::eDupPairDirection CXBracketsLogic::getDuplicatedPairDirection(const CSciMessager& sciMsgr, const Sci_Position nCharPos, const char curr_ch) const
{
    static HWND hLocalEditWnd = NULL;
    static char szWordDelimiters[] = " \t\n'`\"\\|[](){}<>,.;:+-=~!@#$%^&*/?";

    if ( nCharPos <= 0 )
        return DP_FORWARD; // no char before, search forward

    const char prev_ch = sciMsgr.getCharAt(nCharPos - 1);
    if ( prev_ch == curr_ch )
        return DP_BACKWARD; // quoted text, search backward

    int nStyles[4] = {
        -1,  // [0]  nCharPos - 1
        -1,  // [1]  nCharPos
        -1,  // [2]  nCharPos + 1
        -1   // [3]  nCharPos + 2
    };

    if ( nCharPos > 0 )
    {
        nStyles[0] = sciMsgr.getStyleIndexAt(nCharPos - 1);
        nStyles[1] = sciMsgr.getStyleIndexAt(nCharPos);
        nStyles[2] = sciMsgr.getStyleIndexAt(nCharPos + 1);

        if ( nStyles[2] == nStyles[1] && nStyles[1] != nStyles[0] )
            return DP_FORWARD;

        if ( nStyles[0] == nStyles[1] && nStyles[1] != nStyles[2] )
            return DP_BACKWARD;
    }

    const Sci_Position nTextLen = sciMsgr.getTextLength();
    if ( nTextLen <= nCharPos + 1 )
        return DP_BACKWARD; // end of the file

    const char next_ch = sciMsgr.getCharAt(nCharPos + 1);
    if ( next_ch == curr_ch )
        return DP_FORWARD; //  quoted text, search forward

    if ( nTextLen > nCharPos + 2 )
    {
        if ( nStyles[1] == -1 )
            nStyles[1] = sciMsgr.getStyleIndexAt(nCharPos);
        if ( nStyles[2] == -1 )
            nStyles[2] = sciMsgr.getStyleIndexAt(nCharPos + 1);
        nStyles[3] = sciMsgr.getStyleIndexAt(nCharPos + 2);

        if ( nStyles[3] == nStyles[2] && nStyles[2] != nStyles[1] )
            return DP_FORWARD;

        if ( nStyles[1] == nStyles[2] && nStyles[2] != nStyles[3] )
            return DP_BACKWARD;
    }

    if ( prev_ch == '\r' || prev_ch == '\n' )
    {
        // previous char is EOL, search forward
        return isSepOrOneOf(next_ch, szWordDelimiters) ? DP_MAYBEFORWARD : DP_FORWARD;
    }

    if ( next_ch == '\r' || next_ch == '\n')
    {
        // next char is EOL, search backward
        return isSepOrOneOf(prev_ch, szWordDelimiters) ? DP_MAYBEBACKWARD : DP_BACKWARD;
    }

    if ( isSepOrOneOf(prev_ch, szWordDelimiters) )
    {
        // previous char is a separator
        if ( !isSepOrOneOf(next_ch, szWordDelimiters) )
            return DP_FORWARD; // next char is not a separator, search forward

        if ( isSentenceEndChar(prev_ch) )
        {
            if ( !isSentenceEndChar(next_ch) ) // e.g.  ."-
                return DP_MAYBEBACKWARD;
        }
    }
    else if ( isSepOrOneOf(next_ch, szWordDelimiters) )
    {
        // next char is a separator
        if ( !isSepOrOneOf(prev_ch, szWordDelimiters) )
            return DP_BACKWARD; // previous char is not a separator, search backward

        if ( isSentenceEndChar(prev_ch) )
        {
            if ( !isSentenceEndChar(next_ch) ) // e.g.  ."-
                return DP_MAYBEBACKWARD;
        }
    }

    return DP_DETECT;
}

unsigned int CXBracketsLogic::isAtBracketCharacter(const CSciMessager& sciMsgr, const Sci_Position nCharPos, TBracketType* out_nBrType, eDupPairDirection* out_nDupDirection) const
{
    TBracketType nBrType = tbtNone;
    eDupPairDirection nDupDirCurr = DP_NONE;
    eDupPairDirection nDupDirPrev = DP_NONE;
    TBracketType nDetectBrType = tbtNone;
    unsigned int nDetectBrAtPos = abcNone;

    *out_nBrType = tbtNone;
    *out_nDupDirection = DP_NONE;

    const char prev_ch = (nCharPos != 0) ? sciMsgr.getCharAt(nCharPos - 1) : 0;
    const char curr_ch = sciMsgr.getCharAt(nCharPos);

    if ( prev_ch != 0 )
    {
        nBrType = getLeftBracketType(prev_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos - 1) ) //  (|
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                nDupDirPrev = getDuplicatedPairDirection(sciMsgr, nCharPos - 1, prev_ch);
                if ( nDupDirPrev == DP_FORWARD || nDupDirPrev == DP_MAYBEFORWARD )
                {
                    // left quote
                    *out_nDupDirection = nDupDirPrev;
                }
                else if ( nDupDirPrev == DP_DETECT )
                {
                    nDetectBrAtPos = abcBrIsOnLeft;
                    nDetectBrType = nBrType;
                    nBrType = tbtNone;
                }
                else
                    nBrType = tbtNone;
            }
            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcLeftBr | abcBrIsOnLeft);
            }
        }
    }

    if ( curr_ch != 0 )
    {
        nBrType = getLeftBracketType(curr_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos) ) //  |(
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                nDupDirCurr = getDuplicatedPairDirection(sciMsgr, nCharPos, curr_ch);
                if ( nDupDirCurr == DP_FORWARD || nDupDirCurr == DP_MAYBEFORWARD )
                {
                    // left quote
                    *out_nDupDirection = nDupDirCurr;
                }
                else if ( nDupDirCurr == DP_DETECT )
                {
                    nDetectBrAtPos = abcBrIsOnRight;
                    nDetectBrType = nBrType;
                    nBrType = tbtNone;
                }
                else
                    nBrType = tbtNone;
            }
            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcLeftBr | abcBrIsOnRight);
            }
        }
    }

    if ( curr_ch != 0 )
    {
        nBrType = getRightBracketType(curr_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos) ) //  |)
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                if ( nDupDirCurr == DP_NONE )
                    nDupDirCurr = getDuplicatedPairDirection(sciMsgr, nCharPos, curr_ch);

                if ( nDupDirCurr == DP_BACKWARD || nDupDirCurr == DP_MAYBEBACKWARD )
                    *out_nDupDirection = nDupDirCurr; // right quote
                else
                    nBrType = tbtNone;
            }

            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcRightBr | abcBrIsOnRight);
            }
        }
    }

    if ( prev_ch != 0 )
    {
        nBrType = getRightBracketType(prev_ch);
        if ( nBrType != tbtNone && !isEscapedPos(sciMsgr, nCharPos - 1) ) //  )|
        {
            if ( isDuplicatedPair(nBrType) )
            {
                // quotes
                if ( nDupDirPrev == DP_NONE )
                    nDupDirPrev = getDuplicatedPairDirection(sciMsgr, nCharPos - 1, prev_ch);

                if ( nDupDirPrev == DP_BACKWARD || nDupDirPrev == DP_MAYBEBACKWARD )
                    *out_nDupDirection = nDupDirPrev; // right quote
                else
                    nBrType = tbtNone;
            }

            if ( nBrType != tbtNone )
            {
                *out_nBrType = nBrType;
                return (abcRightBr | abcBrIsOnLeft);
            }
        }
    }

    if ( nDetectBrType != tbtNone )
    {
        *out_nBrType = nDetectBrType;
        *out_nDupDirection = DP_DETECT;
        return (abcDetectBr | nDetectBrAtPos);
    }

    return (abcNone);
}

bool CXBracketsLogic::isInBracketsStack(const std::vector<tBracketItem>& bracketsStack, TBracketType nBrType)
{
    const auto itrEnd = bracketsStack.end();
    return (std::find_if(bracketsStack.begin(), itrEnd, [nBrType](const tBracketItem& item){ return item.nBrType == nBrType; }) != itrEnd);
}

bool CXBracketsLogic::findLeftBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state)
{
    switch ( state->nRightBrType )
    {
        case tbtBracket:
        case tbtSquare:
        case tbtBrace:
        case tbtTag:
        {
            Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nStartPos));
            if ( nMatchBrPos != -1 )
            {
                state->nLeftBrPos = nMatchBrPos + 1;
                state->nLeftBrType = state->nRightBrType;
                state->nLeftDupDir = DP_NONE;
                return true; // rely on Scintilla
            }
        }
        break;
    }

    std::vector<tBracketItem> bracketsStack;
    std::vector<char> vLine;

    const Sci_Position nStartLine = sciMsgr.getLineFromPosition(nStartPos);

    for ( Sci_Position nLine = nStartLine; ; --nLine )
    {
        bool bExitLoop = false;
        Sci_Position nLineStartPos = sciMsgr.getPositionFromLine(nLine);
        Sci_Position nLineLen = sciMsgr.getLine(nLine, nullptr);

        vLine.resize(nLineLen + 1);
        nLineLen = sciMsgr.getLine(nLine, vLine.data());

        Sci_Position i = (nLine != nStartLine) ? nLineLen : (nStartPos - nLineStartPos);
        if ( i == 0 )
            continue;

        for ( --i; ; --i )
        {
            const char ch = vLine[i];
            TBracketType nBrType = getLeftBracketType(ch);
            if ( nBrType != tbtNone )
            {
                Sci_Position nBrPos = nLineStartPos + i;
                if ( isEscapedPos(sciMsgr, nBrPos) )
                {
                    nBrType = tbtNone;
                }
                else
                {
                    if ( isDuplicatedPair(nBrType) )
                    {
                        eDupPairDirection nDupPairDirection = getDuplicatedPairDirection(sciMsgr, nBrPos, ch);
                        if ( nDupPairDirection == DP_FORWARD || nDupPairDirection == DP_MAYBEFORWARD )
                        {
                            // left quote found
                            if ( bracketsStack.empty() )
                            {
                                state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                                state->nLeftBrType = nBrType;
                                state->nLeftDupDir = nDupPairDirection;
                                bExitLoop = true;
                                break; // OK, found
                            }

                            if ( bracketsStack.back().nBrType != nBrType ||  // (1)
                                 getDirectionRank(nDupPairDirection, bracketsStack.back().nDupDir) < getDirectionRank(DP_FORWARD, DP_DETECT) )  // (2)
                            {
                                // (1) could be either a left quote inside (another) quotes
                                // (1) or a left quote outside (another) quotes, not sure...
                                // - or -
                                // (2) can't be sure if it is a pair of quotes
                                bExitLoop = true;
                                break;
                            }

                            bracketsStack.pop_back();
                        }
                        else if ( nDupPairDirection == DP_DETECT )
                        {
                            if ( bracketsStack.empty() )
                            {
                                if ( state->nRightBrType == nBrType &&
                                     state->nRightDupDir == DP_BACKWARD )
                                {
                                    state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                                    state->nLeftBrType = nBrType;
                                    state->nLeftDupDir = nDupPairDirection;
                                    bExitLoop = true;
                                    break; // OK, found
                                }

                                if ( nBrType == state->nRightBrType || isInBracketsStack(bracketsStack, nBrType) )
                                {
                                    // quotes within the same quotes
                                    bExitLoop = true;
                                    break;
                                }

                                // treating it as a right quote
                                bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                            }
                            else if ( bracketsStack.back().nBrType == nBrType )
                            {
                                if ( bracketsStack.back().nDupDir != DP_BACKWARD )
                                {
                                    bExitLoop = true;
                                    break; // can't be sure if it's a pair of quotes or not
                                }

                                bracketsStack.pop_back();
                            }
                            else
                            {
                                if ( nBrType == state->nRightBrType || isInBracketsStack(bracketsStack, nBrType) )
                                {
                                    // quotes within the same quotes
                                    bExitLoop = true;
                                    break;
                                }

                                // treating it as a right quote
                                bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                            }
                        }
                        else
                        {
                            if ( nBrType == state->nRightBrType || isInBracketsStack(bracketsStack, nBrType) )
                            {
                                // quotes within the same quotes
                                bExitLoop = true;
                                break;
                            }

                            // right quote found while looking for a left one
                            bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                        }
                    }
                    else
                    {
                        if ( bracketsStack.empty() )
                        {
                            state->nLeftBrPos = nBrPos + 1; // exclude the bracket itself
                            state->nLeftBrType = nBrType;
                            state->nLeftDupDir = DP_NONE;
                            bExitLoop = true;
                            break; // OK, found
                        }

                        if ( bracketsStack.back().nBrType != nBrType )
                        {
                            // could be either a left bracket inside quotes
                            // or a left bracket outside quotes, not sure...
                            bExitLoop = true;
                            break;
                        }

                        bracketsStack.pop_back();
                    }
                }
            }
            else
            {
                nBrType = getRightBracketType(ch);
                if ( nBrType != tbtNone )
                {
                    Sci_Position nBrPos = nLineStartPos + i;
                    if ( !isEscapedPos(sciMsgr, nBrPos) )
                    {
                        // found a right bracket instead of a left bracket
                        // this can't be a duplicated pair since they have been handled above
                        Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nBrPos));
                        if ( nMatchBrPos != -1 )
                        {
                            // skipping everything up to nMatchBrPos
                            Sci_Position nMatchLine = sciMsgr.getLineFromPosition(nMatchBrPos);
                            if ( nMatchLine != nLine )
                            {
                                nLine = nMatchLine;
                                nLineStartPos = sciMsgr.getPositionFromLine(nLine);
                                nLineLen = sciMsgr.getLine(nLine, nullptr);

                                vLine.resize(nLineLen + 1);
                                nLineLen = sciMsgr.getLine(nLine, vLine.data());
                            }
                            i = nMatchBrPos - nLineStartPos;
                        }
                        else
                        {
                            bracketsStack.push_back({nBrPos, nBrType, DP_NONE});
                        }
                    }
                }
            }

            if ( i == 0 )
                break;
        }

        if ( bExitLoop || nLine == 0 )
            break;
    }

    if ( state->nLeftBrType == tbtNone &&   // no left bracket found
         state->nRightBrType == tbtNone &&  // no right bracket was given
         bracketsStack.size() == 1 )        // exactly 1 item in the stack
    {
        const tBracketItem brItem = bracketsStack.back();
        if ( isDuplicatedPair(brItem.nBrType) && brItem.nDupDir == DP_DETECT )
        {
            state->nLeftBrPos = brItem.nBrPos + 1; // exclude the bracket itself
            state->nLeftBrType = brItem.nBrType;
            state->nLeftDupDir = brItem.nDupDir;
        }
    }

    return (state->nLeftBrType != tbtNone);
}

bool CXBracketsLogic::findRightBracket(const CSciMessager& sciMsgr, const Sci_Position nStartPos, tGetBracketsState* state)
{
    switch ( state->nLeftBrType )
    {
        case tbtBracket:
        case tbtSquare:
        case tbtBrace:
        case tbtTag:
        {
            Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nStartPos - 1));
            if ( nMatchBrPos != -1 )
            {
                state->nRightBrPos = nMatchBrPos;
                state->nRightBrType = state->nLeftBrType;
                state->nRightDupDir = DP_NONE;
                return true; // rely on Scintilla
            }
        }
        break;
    }

    std::vector<tBracketItem> bracketsStack;
    std::vector<char> vLine;

    const Sci_Position nLinesCount = sciMsgr.getLineCount();
    const Sci_Position nStartLine = sciMsgr.getLineFromPosition(nStartPos);

    for ( Sci_Position nLine = nStartLine; nLine < nLinesCount; ++nLine )
    {
        bool bExitLoop = false;
        Sci_Position nLineStartPos = sciMsgr.getPositionFromLine(nLine);
        Sci_Position nLineLen = sciMsgr.getLine(nLine, nullptr);

        vLine.resize(nLineLen + 1);
        nLineLen = sciMsgr.getLine(nLine, vLine.data());

        for ( Sci_Position i = (nLine != nStartLine) ? 0 : (nStartPos - nLineStartPos); i < nLineLen; ++i )
        {
            const char ch = vLine[i];
            TBracketType nBrType = getRightBracketType(ch);
            if ( nBrType != tbtNone )
            {
                Sci_Position nBrPos = nLineStartPos + i;
                if ( isEscapedPos(sciMsgr, nBrPos) )
                {
                    nBrType = tbtNone;
                }
                else
                {
                    if ( isDuplicatedPair(nBrType) )
                    {
                        eDupPairDirection nDupPairDirection = getDuplicatedPairDirection(sciMsgr, nBrPos, ch);
                        if ( nDupPairDirection == DP_BACKWARD || nDupPairDirection == DP_MAYBEBACKWARD )
                        {
                            // right quote found
                            if ( bracketsStack.empty() )
                            {
                                state->nRightBrPos = nBrPos;
                                state->nRightBrType = nBrType;
                                state->nRightDupDir = nDupPairDirection;
                                bExitLoop = true;
                                break; // OK, found
                            }

                            if ( bracketsStack.back().nBrType != nBrType ||  // (1)
                                 getDirectionRank(bracketsStack.back().nDupDir, nDupPairDirection) < getDirectionRank(DP_DETECT, DP_BACKWARD) )  // (2)
                            {
                                // (1) could be either a right quote inside (another) quotes
                                // (1) or a right quote outside (another) quotes, not sure...
                                // - or -
                                // (2) can't be sure if it is a pair of quotes
                                bExitLoop = true;
                                break;
                            }

                            bracketsStack.pop_back();
                        }
                        else if ( nDupPairDirection == DP_DETECT )
                        {
                            if ( bracketsStack.empty() )
                            {
                                if ( state->nLeftBrType == nBrType &&
                                     state->nLeftDupDir == DP_FORWARD )
                                {
                                    state->nRightBrPos = nBrPos;
                                    state->nRightBrType = nBrType;
                                    state->nRightDupDir = nDupPairDirection;
                                    bExitLoop = true;
                                    break; // OK, found
                                }

                                if ( nBrType == state->nLeftBrType || isInBracketsStack(bracketsStack, nBrType) )
                                {
                                    // quotes within the same quotes
                                    bExitLoop = true;
                                    break;
                                }

                                // treating it as a left quote
                                bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                            }
                            else if ( bracketsStack.back().nBrType == nBrType )
                            {
                                if ( bracketsStack.back().nDupDir != DP_FORWARD )
                                {
                                    bExitLoop = true;
                                    break; // can't be sure if it's a pair of quotes or not
                                }

                                bracketsStack.pop_back();
                            }
                            else
                            {
                                if ( nBrType == state->nLeftBrType || isInBracketsStack(bracketsStack, nBrType) )
                                {
                                    // quotes within the same quotes
                                    bExitLoop = true;
                                    break;
                                }

                                // treating it as a left quote
                                bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                            }
                        }
                        else
                        {
                            if ( nBrType == state->nLeftBrType || isInBracketsStack(bracketsStack, nBrType) )
                            {
                                // quotes within the same quotes
                                bExitLoop = true;
                                break;
                            }

                            // left quote found while looking for a right one
                            bracketsStack.push_back({nBrPos, nBrType, nDupPairDirection});
                        }
                    }
                    else
                    {
                        if ( bracketsStack.empty() )
                        {
                            state->nRightBrPos = nBrPos;
                            state->nRightBrType = nBrType;
                            state->nRightDupDir = DP_NONE;
                            bExitLoop = true;
                            break; // OK, found
                        }

                        if ( bracketsStack.back().nBrType != nBrType )
                        {
                            // could be either a right bracket inside quotes
                            // or a right bracket outside quotes, not sure...
                            bExitLoop = true;
                            break;
                        }

                        bracketsStack.pop_back();
                    }
                }
            }
            else
            {
                nBrType = getLeftBracketType(ch);
                if ( nBrType != tbtNone )
                {
                    Sci_Position nBrPos = nLineStartPos + i;
                    if ( !isEscapedPos(sciMsgr, nBrPos) )
                    {
                        // found a left bracket instead of a right bracket
                        // this can't be a duplicated pair since they have been handled above
                        Sci_Position nMatchBrPos = static_cast<Sci_Position>(sciMsgr.SendSciMsg(SCI_BRACEMATCH, nBrPos));
                        if ( nMatchBrPos != -1 )
                        {
                            // skipping everything up to nMatchBrPos
                            Sci_Position nMatchLine = sciMsgr.getLineFromPosition(nMatchBrPos);
                            if ( nMatchLine != nLine )
                            {
                                nLine = nMatchLine;
                                nLineStartPos = sciMsgr.getPositionFromLine(nLine);
                                nLineLen = sciMsgr.getLine(nLine, nullptr);

                                vLine.resize(nLineLen + 1);
                                nLineLen = sciMsgr.getLine(nLine, vLine.data());
                            }
                            i = nMatchBrPos - nLineStartPos;
                        }
                        else
                        {
                            bracketsStack.push_back({nBrPos, nBrType, DP_NONE});
                        }
                    }
                }
            }
        }

        if ( bExitLoop )
            break;
    }

    if ( state->nRightBrType == tbtNone &&  // no right bracket found
         state->nLeftBrType == tbtNone &&   // no left bracket was given
         bracketsStack.size() == 1 )        // exactly 1 item in the stack
    {
        const tBracketItem brItem = bracketsStack.back();
        if ( isDuplicatedPair(brItem.nBrType) && brItem.nDupDir == DP_DETECT )
        {
            state->nRightBrPos = brItem.nBrPos;
            state->nRightBrType = brItem.nBrType;
            state->nRightDupDir = brItem.nDupDir;
        }
    }

    return (state->nRightBrType != tbtNone);
}

void CXBracketsLogic::PerformBracketsAction(eGetBracketsAction nBrAction)
{
    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return; // multiple selections

    tGetBracketsState state;
    state.nSelStart = sciMsgr.getSelectionStart();
    state.nSelEnd = sciMsgr.getSelectionEnd();
    if ( state.nSelStart != state.nSelEnd )
    {
        // something is selected: inverting the selection positions
        if ( sciMsgr.getCurrentPos() == state.nSelEnd )
            sciMsgr.setSel(state.nSelEnd, state.nSelStart);
        else
            sciMsgr.setSel(state.nSelStart, state.nSelEnd);
        return;
    }

    state.nCharPos = state.nSelStart;

#if XBR_USE_BRACKETSTREE
    if ( m_bracketsTree.isTreeEmpty() )
    {
        m_bracketsTree.buildTree(sciMsgr);
    }
#else
    if ( m_nCachedLeftBrPos != -1 && m_nCachedRightBrPos != -1 )
    {
        Sci_Position nTargetSelEnd(-1);
        Sci_Position nTargetSelStart(-1);

        if ( state.nCharPos == m_nCachedLeftBrPos )
            nTargetSelEnd = m_nCachedRightBrPos;
        else if ( state.nCharPos == m_nCachedLeftBrPos - 1 )
            nTargetSelEnd = m_nCachedRightBrPos + 1;
        else if ( state.nCharPos == m_nCachedRightBrPos )
            nTargetSelEnd = m_nCachedLeftBrPos;
        else if ( state.nCharPos == m_nCachedRightBrPos + 1 )
            nTargetSelEnd = m_nCachedLeftBrPos - 1;

        if ( nTargetSelEnd != -1 )
        {
            if ( nBrAction == baGoToMatching || nBrAction == baGoToNearest )
                nTargetSelStart = nTargetSelEnd;
            else
                nTargetSelStart = state.nCharPos;

            sciMsgr.setSel(nTargetSelStart, nTargetSelEnd);
            return;
        }
    }
#endif

    Sci_Position nStartPos = state.nCharPos;
    TBracketType nBrType = tbtNone;
    eDupPairDirection nDupDir = DP_NONE;
    bool isBrPairFound = false;
    bool isNearestBr = false;

    unsigned int nAtBr = isAtBracketCharacter(sciMsgr, nStartPos, &nBrType, &nDupDir);
    if ( nBrType == tbtTag2 )
        nBrType = tbtTag;

#if XBR_USE_BRACKETSTREE
    unsigned int uBrPosFlags = 0;
    const auto pBrItem = m_bracketsTree.findPairByPos(nStartPos, false, &uBrPosFlags);
    if ( pBrItem != nullptr && pBrItem->nRightBrPos != -1 )
    {
        state.nLeftBrPos = pBrItem->nLeftBrPos;
        state.nRightBrPos = pBrItem->nRightBrPos;

        if ( uBrPosFlags == (CBracketsTree::bpfLeftBr | CBracketsTree::bpfBeforeBr) ||
             uBrPosFlags == (CBracketsTree::bpfRightBr | CBracketsTree::bpfAfterBr) )
        {
            state.nRightBrPos += static_cast<Sci_Position>(pBrItem->pBrPair->rightBr.length()); //  |)  ->  )|
            state.nLeftBrPos -= static_cast<Sci_Position>(pBrItem->pBrPair->leftBr.length());  //  (|  ->  |(
        }

        isBrPairFound = true;
    }
#else
    if ( nAtBr & abcLeftBr )
    {
        // at left bracket...
        if ( nAtBr & abcBrIsOnRight )
            ++nStartPos; //  |(  ->  (|

        state.nLeftBrPos = nStartPos; // exclude the bracket itself
        state.nLeftBrType = nBrType;
        state.nLeftDupDir = nDupDir;

        if ( findRightBracket(sciMsgr, nStartPos, &state) && nBrType == state.nRightBrType )
        {
            m_nCachedLeftBrPos = state.nLeftBrPos;
            m_nCachedRightBrPos = state.nRightBrPos;

            if ( nAtBr & abcBrIsOnRight )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
    }
    else if ( nAtBr & abcRightBr )
    {
        // at right bracket...
        if ( nAtBr & abcBrIsOnLeft )
            --nStartPos; //  )|  ->  |)

        state.nRightBrPos = nStartPos;
        state.nRightBrType = nBrType;
        state.nRightDupDir = nDupDir;

        if ( findLeftBracket(sciMsgr, nStartPos, &state) && nBrType == state.nLeftBrType )
        {
            m_nCachedLeftBrPos = state.nLeftBrPos;
            m_nCachedRightBrPos = state.nRightBrPos;

            if ( nAtBr & abcBrIsOnLeft )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
    }
    else if ( nAtBr & abcDetectBr )
    {
        // detect: either at left or at right quote
        unsigned int nAtBrNow = nAtBr;

        // 1. try as a left quote...
        if ( nAtBr & abcBrIsOnRight )
        {
            ++nStartPos; //  |(  ->  (|
            nAtBrNow = abcDetectBr | abcBrIsOnLeft;
        }

        state.nLeftBrPos = nStartPos; // exclude the quote itself
        state.nLeftBrType = nBrType;
        state.nLeftDupDir = nDupDir;

        if ( findRightBracket(sciMsgr, nStartPos, &state) && nBrType == state.nRightBrType )
        {
            m_nCachedLeftBrPos = state.nLeftBrPos;
            m_nCachedRightBrPos = state.nRightBrPos;

            if ( nAtBr & abcBrIsOnRight )
            {
                ++state.nRightBrPos; //  |)  ->  )|
                --state.nLeftBrPos;  //  (|  ->  |(
            }

            isBrPairFound = true;
        }
        else
        {
            state.nLeftBrPos = -1;
            state.nLeftBrType = tbtNone;
            state.nLeftDupDir = DP_NONE;

            // 2. try as a right quote...
            nAtBrNow = nAtBr;
            if ( (nAtBr & abcBrIsOnLeft) || (nAtBr & abcBrIsOnRight) )
            {
                --nStartPos; //  )|  ->  |)
                nAtBrNow = abcDetectBr | abcBrIsOnRight;
            }

            state.nRightBrPos = nStartPos;
            state.nRightBrType = nBrType;
            state.nRightDupDir = nDupDir;

            if ( findLeftBracket(sciMsgr, nStartPos, &state) && nBrType == state.nLeftBrType )
            {
                m_nCachedLeftBrPos = state.nLeftBrPos;
                m_nCachedRightBrPos = state.nRightBrPos;

                if ( nAtBr & abcBrIsOnLeft )
                {
                    ++state.nRightBrPos; //  |)  ->  )|
                    --state.nLeftBrPos;  //  (|  ->  |(
                }

                isBrPairFound = true;
            }
        }
    }
    else
    {
        // not at a bracket
        if ( nBrAction == baGoToMatching || nBrAction == baSelToMatching )
            return; // no matching bracket

        // finding nearest brackets...
        if ( findLeftBracket(sciMsgr, nStartPos, &state) &&
             findRightBracket(sciMsgr, nStartPos, &state) )
        {
            if ( state.nLeftBrType == state.nRightBrType )
            {
                if ( !isDuplicatedPair(state.nLeftBrType) ||
                     getDirectionRank(state.nLeftDupDir, state.nRightDupDir) >= getDirectionRank(DP_FORWARD, DP_DETECT) )
                {
                    isNearestBr = true;
                    isBrPairFound = true;
                }
            }
        }
    }
#endif

    if ( isBrPairFound )
    {
        Sci_Position nTargetSelStart(-1), nTargetSelEnd(-1);

        // TODO: adjust selection positions for isNearestBr
        if ( nBrAction == baGoToMatching || nBrAction == baGoToNearest )
        {
            if ( state.nSelStart == state.nLeftBrPos )
            {
                nTargetSelStart = state.nRightBrPos;
                nTargetSelEnd = state.nRightBrPos;
            }
            else
            {
                nTargetSelStart = state.nLeftBrPos;
                nTargetSelEnd = state.nLeftBrPos;
            }
        }
        else
        {
            if ( state.nSelStart == state.nLeftBrPos )
            {
                nTargetSelStart = state.nLeftBrPos;
                nTargetSelEnd = state.nRightBrPos;
            }
            else
            {
                nTargetSelStart = state.nRightBrPos;
                nTargetSelEnd = state.nLeftBrPos;
            }
        }

        sciMsgr.setSel(nTargetSelStart, nTargetSelEnd);
    }
}

void CXBracketsLogic::UpdateFileType()
{
    tstr fileExtension;
    unsigned int uFileType = detectFileType(&fileExtension);
    setFileType(uFileType);
#if XBR_USE_BRACKETSTREE
    m_bracketsTree.setFileType(uFileType, fileExtension);
#endif
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::autoBracketsFunc(TBracketType nBracketType)
{
    if ( autoBracketsOverSelectionFunc(nBracketType) )
        return cprSelAutoCompl;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();

    // is something selected?
    if ( nEditEndPos != nEditPos )
    {
        const int nSelectionMode = sciMsgr.getSelectionMode();
        if ( nSelectionMode == SC_SEL_RECTANGLE || nSelectionMode == SC_SEL_THIN )
            return cprNone;

        // removing selection
        sciMsgr.replaceSelText("");
    }

    // Theory:
    // - The character just pressed is a standard bracket symbol
    // - Thus, this character takes 1 byte in both ANSI and UTF-8
    // - Thus, we check next (and previous) byte in Scintilla to
    //   determine whether the bracket autocompletion is allowed
    // - In general, previous byte can be a trailing byte of one
    //   multi-byte UTF-8 character, but we just ignore this :)
    //   (it is safe because non-leading byte in UTF-8
    //    is always >= 0x80 whereas the character codes
    //    of standard Latin symbols are < 0x80)

    bool  bPrevCharOK = true;
    bool  bNextCharOK = false;
    char  next_ch = sciMsgr.getCharAt(nEditPos);

    if ( isWhiteSpaceOrNulChar(next_ch) ||
         g_opt.getNextCharOK().find(static_cast<TCHAR>(next_ch)) != tstr::npos )
    {
        bNextCharOK = true;
    }

    TBracketType nRightBracketType = getRightBracketType(next_ch, bofIgnoreMode);
    if ( nRightBracketType != tbtNone )
    {
        if ( nRightBracketType == tbtTag2 )
            nRightBracketType = tbtTag;

        bNextCharOK = (nRightBracketType != nBracketType || g_opt.getBracketsRightExistsOK());
    }

    if ( bNextCharOK && (nBracketType == tbtDblQuote || nBracketType == tbtSglQuote) )
    {
        bPrevCharOK = false;

        // previous character
        char prev_ch = (nEditPos >= 1) ? sciMsgr.getCharAt(nEditPos - 1) : 0;

        if ( isWhiteSpaceOrNulChar(prev_ch) ||
             g_opt.getPrevCharOK().find(static_cast<TCHAR>(prev_ch)) != tstr::npos )
        {
            bPrevCharOK = true;
        }
    }

    if ( bPrevCharOK && bNextCharOK )
    {
        if ( isEscapedPos(sciMsgr, nEditPos) )
            bPrevCharOK = false;
    }

    if ( bPrevCharOK && bNextCharOK )
    {
        if ( nBracketType == tbtTag )
        {
            if ( g_opt.getBracketsDoTag2() )
                nBracketType = tbtTag2;
        }

        sciMsgr.beginUndoAction();
        // inserting the brackets pair
        sciMsgr.replaceSelText(strBrackets[nBracketType]);
        // placing the caret between brackets
        ++nEditPos;
        sciMsgr.setSel(nEditPos, nEditPos);
        sciMsgr.endUndoAction();

        m_nAutoRightBracketPos = nEditPos;
        return cprBrAutoCompl;
    }

    return cprNone;
}

bool CXBracketsLogic::isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, TBracketType* pnBracketType, bool bInSelection)
{
    if ( pszTextLeft == pszTextRight )
        return false;

    TBracketType nBrType = *pnBracketType;
    const char* cszBrPair = strBrackets[nBrType];

    if ( pszTextLeft[0] != cszBrPair[0] )
        return false;

    bool bRet = false;
    if ( (pszTextRight[0] == cszBrPair[1]) &&
         (cszBrPair[2] == 0 || pszTextRight[1] == cszBrPair[2]) )
    {
        bRet = true;
        if ( nBrType != tbtTag )
            return bRet;
    }

    if ( nBrType == tbtTag )
    {
        nBrType = tbtTag2;
        if ( bInSelection )
            --pszTextRight; // '/' is present in "/>"
    }
    else if ( nBrType == tbtTag2 )
    {
        nBrType = tbtTag;
        if ( bInSelection )
            ++pszTextRight; // '/' is not present in ">"
    }
    else
        return bRet;

    // Note: both tbtTag and tbtTag2 start with '<'
    cszBrPair = strBrackets[nBrType];
    if ( (pszTextRight[0] == cszBrPair[1]) &&
         (cszBrPair[2] == 0 || pszTextRight[1] == cszBrPair[2]) )
    {
        *pnBracketType = nBrType;
        bRet = true;
    }

    return bRet;
}

bool CXBracketsLogic::autoBracketsOverSelectionFunc(TBracketType nBracketType)
{
    const UINT uSelAutoBr = g_opt.getBracketsSelAutoBr();
    if ( uSelAutoBr == CXBracketsOptions::sabNone )
        return false; // nothing to do - not processed

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
        return false; // nothing to do with multiple selections - not processed

    const int nSelectionMode = sciMsgr.getSelectionMode();
    if ( nSelectionMode == SC_SEL_RECTANGLE || nSelectionMode == SC_SEL_THIN )
        return false; // rectangle selection - not processed

    const Sci_Position nEditPos = sciMsgr.getSelectionStart();
    const Sci_Position nEditEndPos = sciMsgr.getSelectionEnd();
    if ( nEditPos == nEditEndPos )
        return false; // no selection - not processed

    const Sci_Position nSelPos = nEditPos < nEditEndPos ? nEditPos : nEditEndPos;

    if ( nBracketType == tbtTag && g_opt.getBracketsDoTag2() )
        nBracketType = tbtTag2;

    TBracketType nBrAltType = nBracketType;
    int nBrPairLen = lstrlenA(strBrackets[nBracketType]);

    // getting the selected text
    const Sci_Position nSelLen = sciMsgr.getSelText(nullptr);
    std::vector<char> vSelText(nSelLen + nBrPairLen + 1);
    sciMsgr.getSelText(vSelText.data() + 1); // always starting from pSelText[1]

    sciMsgr.beginUndoAction();

    if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemove ||
         uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter )
    {
        vSelText[0] = nSelPos != 0 ? sciMsgr.getCharAt(nSelPos - 1) : 0; // previous character (before the selection)
        for (int i = 0; i < nBrPairLen - 1; i++)
        {
            vSelText[nSelLen + 1 + i] = sciMsgr.getCharAt(nSelPos + nSelLen + i); // next characters (after the selection)
        }
        vSelText[nSelLen + nBrPairLen] = 0;

        if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
        {
            // already in brackets/quotes : ["text"] ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            vSelText[nSelLen - nBrPairLen + 2] = 0;
            sciMsgr.replaceSelText(vSelText.data() + 2);
            sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
        }
        else if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter &&
                  nSelPos != 0 &&
                  isEnclosedInBrackets(vSelText.data(), vSelText.data() + nSelLen + 1, &nBrAltType, false) )
        {
            // already in brackets/quotes : "[text]" ; excluding them
            if ( nBrAltType != nBracketType )
            {
                nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            }
            vSelText[nSelLen + 1] = 0;
            sciMsgr.SendSciMsg(WM_SETREDRAW, FALSE, 0);
            sciMsgr.setSel(nSelPos - 1, nSelPos + nSelLen + nBrPairLen - 1);
            sciMsgr.SendSciMsg(WM_SETREDRAW, TRUE, 0);
            sciMsgr.replaceSelText(vSelText.data() + 1);
            sciMsgr.setSel(nSelPos - 1, nSelPos + nSelLen - 1);
        }
        else
        {
            // enclose in brackets/quotes
            vSelText[0] = strBrackets[nBracketType][0];
            lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }
    else
    {
        vSelText[0] = strBrackets[nBracketType][0];

        if ( uSelAutoBr == CXBracketsOptions::sabEncloseAndSel )
        {
            if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
            {
                // already in brackets/quotes; exclude them
                if ( nBrAltType != nBracketType )
                {
                    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
                }
                vSelText[nSelLen - nBrPairLen + 2] = 0;
                sciMsgr.replaceSelText(vSelText.data() + 2);
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
            }
            else
            {
                // enclose in brackets/quotes
                lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
                sciMsgr.replaceSelText(vSelText.data());
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen + nBrPairLen);
            }
        }
        else // CXBracketsOptions::sabEnclose
        {
            lstrcpyA(vSelText.data() + nSelLen + 1, strBrackets[nBracketType] + 1);
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }

    sciMsgr.endUndoAction();

    m_nAutoRightBracketPos = -1;

    return true; // processed
}

unsigned int CXBracketsLogic::detectFileType(tstr* pFileExt)
{
    TCHAR szExt[CXBracketsOptions::MAX_EXT];
    TCHAR* pszExt = szExt;
    unsigned int uType = tfmIsSupported;

    szExt[0] = 0;
    m_nppMsgr.getCurrentFileExtPart(CXBracketsOptions::MAX_EXT - 1, szExt);

    if ( szExt[0] )
    {
        if ( *pszExt == _T('.') )
        {
            ++pszExt;
            if ( !(*pszExt) )
            {
                if ( pFileExt )
                {
                    pFileExt->clear();
                }
                return uType;
            }
        }

        ::CharLower(pszExt);

        if ( lstrcmp(pszExt, _T("c")) == 0 ||
             lstrcmp(pszExt, _T("cc")) == 0 ||
             lstrcmp(pszExt, _T("cpp")) == 0 ||
             lstrcmp(pszExt, _T("cxx")) == 0 ||
             lstrcmp(pszExt, _T("h")) == 0 ||
             lstrcmp(pszExt, _T("hh")) == 0 ||
             lstrcmp(pszExt, _T("hpp")) == 0 ||
             lstrcmp(pszExt, _T("hxx")) == 0 )
        {
            uType |= tfmComment1 | tfmEscaped1;
        }
        else if ( lstrcmp(pszExt, _T("pas")) == 0 )
        {
            uType |= tfmComment1;
        }
        else
        {
            if ( g_opt.IsHtmlCompatible(pszExt) )
                uType |= tfmHtmlCompatible;

            if ( g_opt.IsEscapedFileExt(pszExt) )
                uType |= tfmEscaped1;
        }

        if ( g_opt.IsSingleQuoteFileExt(pszExt) )
            uType |= tfmSingleQuote;

        if ( g_opt.IsSupportedFile(pszExt) )
            uType |= tfmIsSupported;
        else if ( (uType & tfmIsSupported) != 0 )
            uType ^= tfmIsSupported;
    }

    if ( pFileExt )
    {
        *pFileExt = pszExt;
    }
    return uType;
}
