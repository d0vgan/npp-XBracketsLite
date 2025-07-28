#include "XBracketsLogic.h"
#include "XBracketsOptions.h"
#include <string>
#include <vector>
#include <algorithm>

using namespace XBrackets;

// can be _T(x), but _T(x) may be incompatible with ANSI mode
#define _TCH(x)  (x)

extern CXBracketsOptions g_opt;

namespace
{
    void getEscapedPrefixPos(const Sci_Position nOffset, Sci_Position* pnPos, int* pnLen)
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
}

//-----------------------------------------------------------------------------

CBracketsTree::CBracketsTree()
{
}

void CBracketsTree::setFileSyntax(const tFileSyntax* pFileSyntax)
{
    m_pFileSyntax = pFileSyntax;
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
                    if ( kind == bpkSgLnQuotes || kind == bpkSgLnBrackets )
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
                        if ( kind == bpkSgLnQuotes || kind == bpkSgLnBrackets )
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
                        if ( kind == bpkMlLnComm || ((kind == bpkSgLnQuotes || kind == bpkMlLnQuotes) && (pBrPair->kind == bpkMlLnBrackets || pBrPair->kind == bpkSgLnBrackets)) )
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

    CSciMessager sciMsgr(reinterpret_cast<HWND>(pscn->nmhdr.hwndFrom));
    if ( sciMsgr.getSelections() > 1 )
    {
        invalidateTree();
        return; // tree needs to be recreated
    }

    // 1. checking the document's text around pscn->position
    bool needsInvalidate = false;
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

const tBrPairItem* CBracketsTree::findPairByPos(const Sci_Position nPos, bool isExact, unsigned int* puBrPosFlags) const
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

const tBrPairItem* CBracketsTree::findParent(const tBrPairItem* pBrPair) const
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

const tBrPair* CBracketsTree::getLeftBrPair(const char* p, size_t nLen) const
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

const tBrPair* CBracketsTree::getRightBrPair(const char* p, size_t nLen) const
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

//-----------------------------------------------------------------------------

CXBracketsLogic::CXBracketsLogic()
{
}

void CXBracketsLogic::SetNppData(const NppData& nppd)
{
    m_nppMsgr.setNppData(nppd);
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
        m_nAutoRightBracketType = -1;
        m_nAutoRightBracketOffset = -1;
    }
    if ( uInvalidateFlags & icbfTree )
    {
        if ( pscn == nullptr || (pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) == 0 )
            m_bracketsTree.invalidateTree();
        else
            m_bracketsTree.updateTree(pscn);
    }
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::OnChar(const int ch)
{
    const Sci_Position nAutoRightBrPos = m_nAutoRightBracketPos;
    const int nAutoRightBrType = m_nAutoRightBracketType;
    const int nAutoRightBrOffset = m_nAutoRightBracketOffset; // will be used below

    InvalidateCachedBrackets(icbfBrPair | icbfAutoRightBr);

    if ( !g_opt.getBracketsAutoComplete() )
        return cprNone;

    if ( (m_uFileType & tfmIsSupported) == 0 )
        return cprNone;

    CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());

    if ( sciMsgr.getSelections() > 1 )
    {
        m_bracketsTree.invalidateTree();
        return cprNone; // nothing to do with multiple selections
    }

    if ( nAutoRightBrPos >= 0 && nAutoRightBrType >= 0 )
    {
        if ( m_pFileSyntax == nullptr || nAutoRightBrType >= static_cast<int>(m_pFileSyntax->autocomplete.size()) )
            return cprNone;

        // the right bracket has been just added (automatically)
        // but you may duplicate it manually
        Sci_Position pos = sciMsgr.getCurrentPos();
        if ( pos >= nAutoRightBrPos )
        {
            const std::string& rightBr = m_pFileSyntax->autocomplete[nAutoRightBrType].rightBr;
            const int nRightBrLen = static_cast<int>(rightBr.length());
            if ( pos == nAutoRightBrPos + nAutoRightBrOffset )
            {
                if ( ch == rightBr[nAutoRightBrOffset] )
                {
                    ++pos;
                    if ( pos < nAutoRightBrPos + nRightBrLen )
                    {
                        // e.g. it is "-->" in the "<!--|-->" pair
                        m_nAutoRightBracketPos = nAutoRightBrPos;
                        m_nAutoRightBracketType = nAutoRightBrType;
                        m_nAutoRightBracketOffset = nAutoRightBrOffset + 1;
                    }
                    sciMsgr.setSel(pos, pos);
                    return cprBrAutoCompl;
                }
            }
        }
    }

    int nLeftBracketType = getAutocompleteLeftBracketType(sciMsgr, static_cast<char>(ch));
    if ( nLeftBracketType == -1 )
        return cprNone;

    // a typed character is a bracket
    return autoBracketsFunc(nLeftBracketType);
}

bool CXBracketsLogic::isSkipEscapedSupported() const
{
    if ( m_pFileSyntax != nullptr )
    {
        for ( const auto& esqStr : m_pFileSyntax->qtEsc )
        {
            if ( !esqStr.empty() )
                return true;
        }
    }
    return false;
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

    if ( m_bracketsTree.isTreeEmpty() )
    {
        m_bracketsTree.buildTree(sciMsgr);
    }

    Sci_Position nStartPos = state.nCharPos;
    bool isBrPairFound = false;

    bool isExactPos = (nBrAction == baGoToMatching || nBrAction == baSelToMatching);
    unsigned int uBrPosFlags = 0;
    const auto pBrItem = m_bracketsTree.findPairByPos(nStartPos, isExactPos, &uBrPosFlags);
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

    if ( isBrPairFound )
    {
        Sci_Position nTargetSelStart(-1), nTargetSelEnd(-1);

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

void CXBracketsLogic::UpdateFileType(unsigned int uInvalidateFlags)
{
    tstr fileExtension;

    m_uFileType = detectFileType(&fileExtension);
    m_pFileSyntax = nullptr;

    if ( m_uFileType & tfmIsSupported )
    {
        for ( const auto& syntax : g_opt.getFileSyntaxes() )
        {
            if ( syntax.fileExtensions.find(fileExtension) != syntax.fileExtensions.end() )
            {
                m_pFileSyntax = &syntax;
                break;
            }
        }
        if ( m_pFileSyntax == nullptr )
            m_pFileSyntax = g_opt.getDefaultFileSyntax();
    }

    m_bracketsTree.setFileSyntax(m_pFileSyntax);

    InvalidateCachedBrackets(uInvalidateFlags);
}

int CXBracketsLogic::getAutocompleteLeftBracketType(CSciMessager& sciMsgr, const char ch) const
{
    if ( m_pFileSyntax == nullptr )
        return -1;

    std::vector<unsigned int> autocompleteIdx;
    unsigned int nMaxBrLen = 0;
    unsigned int idx = 0;

    for ( const auto& brPair : m_pFileSyntax->autocomplete )
    {
        const unsigned int nBrLen = static_cast<unsigned int>(brPair.leftBr.length());
        if ( nBrLen != 0 )
        {
            if ( ch == brPair.leftBr[nBrLen - 1] )
            {
                if ( nMaxBrLen < nBrLen )
                {
                    nMaxBrLen = nBrLen;
                }
                autocompleteIdx.push_back(idx);
            }
        }
        ++idx;
    }

    if ( nMaxBrLen == 0 )
        return -1;

    if ( nMaxBrLen == 1 )
        return autocompleteIdx.front();

    std::vector<char> text(nMaxBrLen + 1);
    unsigned int nLen = nMaxBrLen - 1;
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    if ( nEditPos < static_cast<Sci_Position>(nLen) )
    {
        nLen = static_cast<unsigned int>(nEditPos);
        nEditPos = 0;
    }
    else
    {
        nEditPos -= nLen;
    }
    sciMsgr.getTextRange(nEditPos, nEditPos + nLen, text.data());
    text[nLen] = ch;
    text[nLen + 1] = 0;

    for ( unsigned int i : autocompleteIdx )
    {
        const std::string& leftBr = m_pFileSyntax->autocomplete[i].leftBr;
        if ( strcmp(text.data() + nLen + 1 - leftBr.length(), leftBr.c_str()) == 0 )
            return i;
    }

    return -1;
}

int CXBracketsLogic::getAutocompleteRightBracketType(CSciMessager& sciMsgr, const char ch) const
{
    if ( m_pFileSyntax == nullptr )
        return -1;

    std::vector<unsigned int> autocompleteIdx;
    unsigned int nMaxBrLen = 0;
    unsigned int idx = 0;

    for ( const auto& brPair : m_pFileSyntax->autocomplete )
    {
        const unsigned int nBrLen = static_cast<unsigned int>(brPair.rightBr.length());
        if ( nBrLen != 0 )
        {
            if ( ch == brPair.rightBr[0] )
            {
                if ( nMaxBrLen < nBrLen )
                {
                    nMaxBrLen = nBrLen;
                }
                autocompleteIdx.push_back(idx);
            }
        }
        ++idx;
    }

    if ( nMaxBrLen == 0 )
        return -1;

    if ( nMaxBrLen == 1 )
        return autocompleteIdx.front();

    std::vector<char> text(nMaxBrLen + 1);
    unsigned int nLen = nMaxBrLen;
    Sci_Position nEditPos = sciMsgr.getSelectionStart();
    sciMsgr.getTextRange(nEditPos, nEditPos + nLen, text.data());
    text[nLen] = 0;

    for ( unsigned int i : autocompleteIdx )
    {
        const std::string& rightBr = m_pFileSyntax->autocomplete[i].rightBr;
        if ( strcmp(text.data() + nLen - rightBr.length(), rightBr.c_str()) == 0 )
            return i;
    }

    return -1;
}

const tBrPair* CXBracketsLogic::getAutoCompleteBrPair(int nBracketType) const
{
    if ( nBracketType == -1 || m_pFileSyntax == nullptr || static_cast<int>(m_pFileSyntax->autocomplete.size()) <= nBracketType )
        return nullptr;

    return &m_pFileSyntax->autocomplete[nBracketType];
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::autoBracketsFunc(int nBracketType)
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

    int nRightBracketType = getAutocompleteRightBracketType(sciMsgr, next_ch);
    if ( nRightBracketType != -1 )
    {
        bNextCharOK = (nRightBracketType != nBracketType || g_opt.getBracketsRightExistsOK());
    }

    const tBrPair* pBrPair = getAutoCompleteBrPair(nBracketType);
    if ( bNextCharOK && pBrPair->leftBr == pBrPair->rightBr )
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
        //if ( nBracketType == tbtTag )
        //{
        //    if ( g_opt.getBracketsDoTag2() )
        //        nBracketType = tbtTag2;
        //}

        sciMsgr.beginUndoAction();
        // inserting the brackets pair
        std::string brPair;
        brPair.reserve(1 + pBrPair->rightBr.length());
        brPair += pBrPair->leftBr.back();
        brPair += pBrPair->rightBr;
        sciMsgr.replaceSelText(brPair.c_str());
        // placing the caret between brackets
        ++nEditPos;
        sciMsgr.setSel(nEditPos, nEditPos);
        sciMsgr.endUndoAction();

        m_nAutoRightBracketPos = nEditPos;
        m_nAutoRightBracketType = nBracketType;
        m_nAutoRightBracketOffset = 0;
        return cprBrAutoCompl;
    }

    return cprNone;
}

bool CXBracketsLogic::isEnclosedInBrackets(const char* pszTextLeft, const char* pszTextRight, int* pnBracketType, bool bInSelection)
{
    if ( pszTextLeft == pszTextRight )
        return false;

    unsigned int nBrType = *pnBracketType;
    const tBrPair* pBrPair = getAutoCompleteBrPair(nBrType);

    return (pszTextLeft[0] == pBrPair->leftBr[0] && pszTextRight[0] == pBrPair->rightBr[0]);
}

bool CXBracketsLogic::autoBracketsOverSelectionFunc(int nBracketType)
{
    const UINT uSelAutoBr = g_opt.getBracketsSelAutoBr();
    if ( uSelAutoBr == CXBracketsOptions::sabNone )
        return false; // nothing to do - not processed

    const tBrPair* pBrPair = getAutoCompleteBrPair(nBracketType);
    if ( pBrPair == nullptr )
        return false; // nothing to do - not processed

    if ( pBrPair->leftBr.length() != 1 || pBrPair->rightBr.length() != 1 )
        return false; // not supported - not processed

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
    unsigned int nBrPairLen = 2;
    int nBrAltType = nBracketType;

    // getting the selected text
    const Sci_Position nSelLen = sciMsgr.getSelText(nullptr);
    std::vector<char> vSelText(nSelLen + nBrPairLen + 1);
    sciMsgr.getSelText(vSelText.data() + 1); // always starting from pSelText[1]

    sciMsgr.beginUndoAction();

    if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemove ||
         uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter )
    {
        vSelText[0] = nSelPos != 0 ? sciMsgr.getCharAt(nSelPos - 1) : 0; // previous character (before the selection)
        for (unsigned int i = 0; i < nBrPairLen - 1; i++)
        {
            vSelText[nSelLen + 1 + i] = sciMsgr.getCharAt(nSelPos + nSelLen + i); // next characters (after the selection)
        }
        vSelText[nSelLen + nBrPairLen] = 0;

        if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
        {
            // already in brackets/quotes : ["text"] ; excluding them
            //if ( nBrAltType != nBracketType )
            //{
            //    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            //}
            vSelText[nSelLen - nBrPairLen + 2] = 0;
            sciMsgr.replaceSelText(vSelText.data() + 2);
            sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
        }
        else if ( uSelAutoBr == CXBracketsOptions::sabEncloseRemoveOuter &&
                  nSelPos != 0 &&
                  isEnclosedInBrackets(vSelText.data(), vSelText.data() + nSelLen + 1, &nBrAltType, false) )
        {
            // already in brackets/quotes : "[text]" ; excluding them
            //if ( nBrAltType != nBracketType )
            //{
            //    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
            //}
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
            vSelText[0] = pBrPair->leftBr[0];
            lstrcpyA(vSelText.data() + nSelLen + 1, pBrPair->rightBr.c_str());
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }
    else
    {
        vSelText[0] = pBrPair->leftBr[0];

        if ( uSelAutoBr == CXBracketsOptions::sabEncloseAndSel )
        {
            if ( isEnclosedInBrackets(vSelText.data() + 1, vSelText.data() + nSelLen - nBrPairLen + 2, &nBrAltType, true) )
            {
                // already in brackets/quotes; exclude them
                //if ( nBrAltType != nBracketType )
                //{
                //    nBrPairLen = lstrlenA(strBrackets[nBrAltType]);
                //}
                vSelText[nSelLen - nBrPairLen + 2] = 0;
                sciMsgr.replaceSelText(vSelText.data() + 2);
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen - nBrPairLen);
            }
            else
            {
                // enclose in brackets/quotes
                lstrcpyA(vSelText.data() + nSelLen + 1, pBrPair->rightBr.c_str());
                sciMsgr.replaceSelText(vSelText.data());
                sciMsgr.setSel(nSelPos, nSelPos + nSelLen + nBrPairLen);
            }
        }
        else // CXBracketsOptions::sabEnclose
        {
            lstrcpyA(vSelText.data() + nSelLen + 1, pBrPair->rightBr.c_str());
            sciMsgr.replaceSelText(vSelText.data());
            sciMsgr.setSel(nSelPos + 1, nSelPos + nSelLen + 1);
        }
    }

    sciMsgr.endUndoAction();

    InvalidateCachedBrackets(icbfAutoRightBr);

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
            if ( *pszExt == 0 )
            {
                if ( pFileExt )
                {
                    pFileExt->clear();
                }
                return uType;
            }
        }

        ::CharLower(pszExt);

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
