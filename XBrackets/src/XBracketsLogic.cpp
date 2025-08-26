#include "XBracketsLogic.h"
#include "XBracketsOptions.h"
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

#ifdef _DEBUG
#include <assert.h>
#endif


using namespace XBrackets;

// can be _T(x), but _T(x) may be incompatible with ANSI mode
#define _TCH(x)  (x)


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
        case '\t': // 0x09
        case '\n': // 0x0A
        case '\r': // 0x0D
        case ' ':  // 0x20
            return true;
        }

        return false;
    }

    inline bool isTabSpace(const char ch)
    {
        return (ch == ' ' || ch == '\t');
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

static void incIsQuoted(const unsigned int kind, unsigned int& isQuoted, unsigned int& isSgLnQuoted)
{
    const bool isSgLnQt = isSgLnQtKind(kind);
    if ( isSgLnQt )
    {
        ++isSgLnQuoted;
    }
    if ( isSgLnQt || isMlLnQtKind(kind) )
    {
        ++isQuoted;
    }
}

static void decIsQuoted(const unsigned int kind, unsigned int& isQuoted, unsigned int& isSgLnQuoted)
{
    const bool isSgLnQt = isSgLnQtKind(kind);
    if ( isSgLnQt )
    {
    #ifdef _DEBUG
        assert(isSgLnQuoted != 0);
    #endif
        --isSgLnQuoted;
    }
    if ( isSgLnQt || isMlLnQtKind(kind) )
    {
    #ifdef _DEBUG
        assert(isQuoted != 0);
    #endif
        --isQuoted;
    }
}

static void completeChildQuotedBrackets(std::vector<tBrPairItem>& bracketsTree, const Sci_Position nParentIdx, const Sci_Position nEndIdx,
    unsigned int& isQuoted, unsigned int& isSgLnQuoted, bool isInnerCall = false)
{
    for ( Sci_Position nLeftIdx = nParentIdx + 1; nLeftIdx < nEndIdx; ++nLeftIdx )
    {
        tBrPairItem& leftItem = bracketsTree[nLeftIdx];
        if ( leftItem.isOpenLeftBr() )
        {
            for ( Sci_Position nRightIdx = nLeftIdx + 1; nRightIdx < nEndIdx; ++nRightIdx )
            {
                tBrPairItem& rightItem = bracketsTree[nRightIdx];
                if ( rightItem.isOpenRightBr() && leftItem.pBrPair->leftBr == rightItem.pBrPair->leftBr )
                {
                    leftItem.nRightBrPos = rightItem.nRightBrPos;
                    rightItem.nRightBrPos = -1;
                    decIsQuoted(leftItem.pBrPair->kind, isQuoted, isSgLnQuoted);
                    completeChildQuotedBrackets(bracketsTree, nLeftIdx, nRightIdx, isQuoted, isSgLnQuoted, true);
                    if ( !isInnerCall )
                    {
                        for ( Sci_Position nChildIdx = nLeftIdx + 1; nChildIdx < nRightIdx; ++nChildIdx )
                        {
                            tBrPairItem& childItem = bracketsTree[nChildIdx];
                            if ( childItem.isIncomplete() )
                            {
                                childItem.nLeftBrPos = -1;
                                childItem.nRightBrPos = -1;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

void CBracketsTree::buildTree(CSciMessager& sciMsgr)
{
    if ( m_pFileSyntax == nullptr )
        return; // nothing to do

    std::vector<tBrPairItem> bracketsTree;

    bracketsTree.reserve(1000);

    Sci_Position nTextLen = sciMsgr.getTextLength();
    std::vector<char> vText(nTextLen + 1);
    sciMsgr.getText(nTextLen, vText.data());
    vText[nTextLen] = 0;

    Sci_Position nPos = 0;
    Sci_Position nCurrentLine = 0;
    Sci_Position nCurrentParentIdx = -1;
    Sci_Position nPrevLineTreeSize = 0;
    unsigned int isQuoted = 0;
    unsigned int isSgLnQuoted = 0;
    unsigned int uGetBrFlags = gbfLnSt;

    auto invalidateChildIncompleteBrackets = [&bracketsTree](Sci_Position nFoundItemIdx)
    {
        tBrPairItem* pItem = bracketsTree.data() + nFoundItemIdx + 1;
        const tBrPairItem* pEnd = bracketsTree.data() + bracketsTree.size();
        for ( ; pItem != pEnd; ++pItem )
        {
            if ( pItem->isIncomplete() && pItem->nParentIdx >= nFoundItemIdx )
            {
                pItem->nLeftBrPos = -1;
                pItem->nRightBrPos = -1;
            }
        }
    };

    auto getOpenParentIdx = [&bracketsTree](Sci_Position nParentIdx)
    {
        while ( nParentIdx != -1 )
        {
            const tBrPairItem& item = bracketsTree[nParentIdx];
            if ( item.isOpenLeftBr() )
                break;

            nParentIdx = item.nParentIdx;
        }
        return nParentIdx;
    };

    const char* const pEnd = vText.data() + nTextLen;
    for ( const char* p = vText.data(); p < pEnd; ++p, ++nPos )
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
            if ( nCurrentParentIdx != -1 && nCurrentParentIdx >= nPrevLineTreeSize )
            {
                Sci_Position nNewParentIdx = -1;
                Sci_Position nCommIdx = -1;
                Sci_Position nIdx = nCurrentParentIdx;

                for ( ; nIdx != -1; )
                {
                    const tBrPairItem& item = bracketsTree[nIdx];
                    if ( item.nLine != nCurrentLine )
                        break;

                    if ( isSgLnCommKind(item.pBrPair->kind) )
                    {
                        nCommIdx = nIdx;
                        nNewParentIdx = item.nParentIdx; // above the comment
                    }
                    else if ( nNewParentIdx == -1 &&
                              item.isOpenLeftBr() &&
                              !isSgLnBrQtKind(item.pBrPair->kind) )
                    {
                        nNewParentIdx = nIdx; // at an open left multi-line bracket
                    }

                    nIdx = item.nParentIdx;
                }

                if ( nNewParentIdx != -1 )
                    nCurrentParentIdx = nNewParentIdx;
                else
                    nCurrentParentIdx = nIdx;

                if ( nCommIdx != -1 )
                {
                    for ( Sci_Position idx = bracketsTree.size() - 1; idx >= nCommIdx; --idx )
                    {
                        tBrPairItem& item = bracketsTree[idx];
                        if ( idx == nCommIdx || item.isIncomplete() )
                        {
                            item.nLeftBrPos = -1;
                            item.nRightBrPos = -1;
                        }
                    }
                }

                if ( nCurrentParentIdx != -1 )
                {
                    const Sci_Position nEndIdx = static_cast<Sci_Position>(bracketsTree.size());
                    for ( nIdx = nCurrentParentIdx + 1; nIdx < nEndIdx; ++nIdx )
                    {
                        const tBrPairItem& rightItem = bracketsTree[nIdx];
                        if ( rightItem.isOpenRightBr() )
                        {
                            // potentially a right bracket after an incomplete single-line quote
                            for ( Sci_Position nLeftIdx = nCurrentParentIdx; nLeftIdx != -1; )
                            {
                                tBrPairItem& leftItem = bracketsTree[nLeftIdx];
                                if ( leftItem.isOpenLeftBr() )
                                {
                                    if ( leftItem.nLine != nCurrentLine && isSgLnBrQtKind(rightItem.pBrPair->kind) )
                                        break;

                                    if ( leftItem.pBrPair->leftBr == rightItem.pBrPair->leftBr )
                                    {
                                        leftItem.nRightBrPos = rightItem.nRightBrPos;
                                        nCurrentParentIdx = getOpenParentIdx(leftItem.nParentIdx);
                                        break;
                                    }
                                }
                                nLeftIdx = leftItem.nParentIdx;
                            }
                        }
                    }
                }
            }

        #ifdef _DEBUG
            assert(isQuoted >= isSgLnQuoted);
        #endif
            isQuoted -= isSgLnQuoted;
            isSgLnQuoted = 0;
            uGetBrFlags = gbfLnSt;
            nPrevLineTreeSize = bracketsTree.size();
            ++nCurrentLine;
            continue;
        }

        if ( isTabSpace(*p) )
        {
            uGetBrFlags |= gbfLdSp;
            for ( const char* pNext = p + 1; pNext < pEnd; ++pNext )
            {
                if ( isTabSpace(*pNext) )
                {
                    ++p;
                    ++nPos;
                }
                else
                    break;
            }
            if ( !bracketsTree.empty() )
            {
                const tBrPairItem& item = bracketsTree.back();
                if ( item.isOpenLeftBr() && isNoInSpKind(item.pBrPair->kind) )
                {
                    nCurrentParentIdx = getOpenParentIdx(item.nParentIdx);
                    if ( item.nLine != nCurrentLine )
                    {
                        --nPrevLineTreeSize;
                    }
                    bracketsTree.pop_back();
                }
            }
            continue;
        }

        std::vector<const tBrPair*> rightBrPairs;
        const tBrPair* pLeftBrPair = nullptr;
        const tBrPair* pRightBrPair = nullptr;
        const tBrPair* pBrPair = nullptr;
        if ( isQuoted == 0 || !isEscapedPos(vText.data(), nPos) )
        {
            pLeftBrPair = getLeftBrPair(p, nTextLen - nPos, uGetBrFlags);
            rightBrPairs = getRightBrPair(p, nTextLen - nPos, nPos, uGetBrFlags);
            if ( !rightBrPairs.empty() )
            {
                pRightBrPair = rightBrPairs.front();
            }
            pBrPair = (pLeftBrPair != nullptr && (pRightBrPair == nullptr || pRightBrPair->rightBr.length() <= pLeftBrPair->leftBr.length())) ? pLeftBrPair : pRightBrPair;
        }

        if ( (uGetBrFlags & gbfLnSt) != 0 )
            uGetBrFlags ^= gbfLnSt;

        if ( (uGetBrFlags & gbfLdSp) != 0 )
            uGetBrFlags ^= gbfLdSp;

        if ( pBrPair == nullptr )
            continue;

        if ( pBrPair == pLeftBrPair )
        {
            if ( pBrPair->leftBr == pBrPair->rightBr )
            {
                // either left or right bracket/quote
                if ( isQtKind(pBrPair->kind) || isMlLnCommKind(pBrPair->kind) )
                {
                    // this is an enquoting brackets pair
                    Sci_Position nFoundItemIdx = -1;
                    for ( Sci_Position nItemIdx = nCurrentParentIdx; nItemIdx != -1; )
                    {
                        const tBrPairItem& item = bracketsTree[nItemIdx];
                        if ( item.isOpenLeftBr() )
                        {
                            if ( isSgLnBrQtKind(pBrPair->kind) && item.nLine != nCurrentLine )
                                break;

                            for ( const tBrPair* pRightBrPair : rightBrPairs )
                            {
                                if ( item.pBrPair->leftBr == pRightBrPair->leftBr )
                                {
                                    pBrPair = pRightBrPair;
                                    nFoundItemIdx = nItemIdx;
                                    break;
                                }
                            }
                            if ( nFoundItemIdx != -1 )
                                break;

                            if ( isMlLnCommKind(pBrPair->kind) && isMlLnCommKind(item.pBrPair->kind) ) // TODO: verify this condition
                                break;
                        }
                        nItemIdx = item.nParentIdx;
                    }

                    if ( nFoundItemIdx != -1 )
                    {
                        tBrPairItem& item = bracketsTree[nFoundItemIdx];
                        // nPos is its right bracket
                        item.nRightBrPos = nPos; // |)
                        nCurrentParentIdx = getOpenParentIdx(item.nParentIdx);

                        if ( isQtKind(item.pBrPair->kind) )
                        {
                            decIsQuoted(item.pBrPair->kind, isQuoted, isSgLnQuoted);
                            completeChildQuotedBrackets(bracketsTree, nFoundItemIdx, static_cast<Sci_Position>(bracketsTree.size()), isQuoted, isSgLnQuoted);
                        }
                        invalidateChildIncompleteBrackets(nFoundItemIdx);
                    }
                    else
                    {
                        // left bracket
                        const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length()); // (|
                        bracketsTree.push_back({nLeftBrPos, -1, nCurrentLine, nCurrentParentIdx, pBrPair});
                        incIsQuoted(pBrPair->kind, isQuoted, isSgLnQuoted);
                        nCurrentParentIdx = static_cast<Sci_Position>(bracketsTree.size() - 1);
                    }
                }
                else
                {
                    // non-enquoting brackets pair
                    if ( !bracketsTree.empty() )
                    {
                        auto& item = bracketsTree.back();
                        if ( item.pBrPair->leftBr == pBrPair->leftBr &&
                             (item.nLine == nCurrentLine || !isSgLnBrQtKind(item.pBrPair->kind)) )
                        {
                            // item is an unmatched left bracket, nPos is its right bracket
                            item.nRightBrPos = nPos; // |)
                            nCurrentParentIdx = getOpenParentIdx(item.nParentIdx);
                        }
                        else
                        {
                            // left bracket, non-enquoting
                            const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length()); // (|
                            bracketsTree.push_back({nLeftBrPos, -1, nCurrentLine, nCurrentParentIdx, pBrPair});
                            nCurrentParentIdx = static_cast<Sci_Position>(bracketsTree.size() - 1);
                        }
                    }
                    else
                    {
                        // left bracket, non-enquoting
                        const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length()); // (|
                        bracketsTree.push_back({nLeftBrPos, -1, nCurrentLine, nCurrentParentIdx, pBrPair});
                        nCurrentParentIdx = static_cast<Sci_Position>(bracketsTree.size() - 1);
                    }
                }
            }
            else
            {
                // left bracket
                Sci_Position nFoundItemIdx = -1;
                if ( isMlLnCommKind(pBrPair->kind) )
                {
                    for ( Sci_Position nItemIdx = nCurrentParentIdx; nItemIdx != -1; )
                    {
                        const tBrPairItem& item = bracketsTree[nItemIdx];
                        const unsigned int itemKind = item.pBrPair->kind;
                        if ( isMlLnQtKind(itemKind) || (isMlLnCommKind(itemKind) && item.pBrPair->leftBr == pBrPair->leftBr) )
                        {
                            nFoundItemIdx = nItemIdx;
                            break;
                        }

                        nItemIdx = item.nParentIdx;
                    }
                }

                if ( nFoundItemIdx == -1 )
                {
                    const Sci_Position nLeftBrPos = nPos + static_cast<Sci_Position>(pBrPair->leftBr.length()); // (|
                    bracketsTree.push_back({nLeftBrPos, -1, nCurrentLine, nCurrentParentIdx, pBrPair});
                    incIsQuoted(pBrPair->kind, isQuoted, isSgLnQuoted);
                    nCurrentParentIdx = static_cast<Sci_Position>(bracketsTree.size() - 1);
                }
            }

            int nBrLen = static_cast<int>(pBrPair->leftBr.length());
            if ( nBrLen > 1 )
            {
                --nBrLen;
                p += nBrLen;
                nPos += nBrLen;
            }
        }
        else if ( pBrPair == pRightBrPair )
        {
            // right bracket
            Sci_Position nFoundItemIdx = -1;
            for ( Sci_Position nItemIdx = nCurrentParentIdx; nItemIdx != -1; )
            {
                const tBrPairItem& item = bracketsTree[nItemIdx];
                if ( item.isOpenLeftBr() )
                {
                    if ( isSgLnBrQtKind(pBrPair->kind) && item.nLine != nCurrentLine )
                        break;

                    if ( isSgLnQuoted == 0 || isMlLnCommKind(item.pBrPair->kind) )
                    {
                        for ( const tBrPair* pRightBrPair : rightBrPairs )
                        {
                            if ( item.pBrPair->leftBr == pRightBrPair->leftBr )
                            {
                                pBrPair = pRightBrPair;
                                nFoundItemIdx = nItemIdx;
                                break;
                            }
                        }
                        if ( nFoundItemIdx != -1 )
                            break;
                    }

                    if ( isMlLnCommKind(item.pBrPair->kind) && !isMlLnCommKind(pBrPair->kind) )
                        break;

                    if ( isMlLnQtKind(item.pBrPair->kind) )
                        break;

                    if ( item.nLine == nCurrentLine &&
                         (isSgLnCommKind(item.pBrPair->kind) || isSgLnQuoted) &&
                         !isMlLnCommKind(pBrPair->kind) && !isQtKind(pBrPair->kind) )
                    {
                        // adding a child right bracket for the further processing
                        bracketsTree.push_back({-1, nPos, nCurrentLine, nCurrentParentIdx, pBrPair});
                        break;
                    }
                }
                nItemIdx = item.nParentIdx;
            }

            if ( nFoundItemIdx != -1 )
            {
                tBrPairItem& item = bracketsTree[nFoundItemIdx];
                item.nRightBrPos = nPos; // |)
                nCurrentParentIdx = getOpenParentIdx(item.nParentIdx);

                if ( isQtKind(item.pBrPair->kind) )
                {
                    decIsQuoted(item.pBrPair->kind, isQuoted, isSgLnQuoted);
                    completeChildQuotedBrackets(bracketsTree, nFoundItemIdx, static_cast<Sci_Position>(bracketsTree.size()), isQuoted, isSgLnQuoted);
                }
                invalidateChildIncompleteBrackets(nFoundItemIdx);
            }

            int nBrLen = static_cast<int>(pBrPair->rightBr.length());
            if ( nBrLen > 1 )
            {
                --nBrLen;
                p += nBrLen;
                nPos += nBrLen;
            }
        }
    }

    std::vector<std::pair<size_t, size_t>> idxMapping;
    size_t idx = 0;

    m_bracketsTree.clear();
    m_bracketsTree.reserve(bracketsTree.size()/2);
    idxMapping.reserve(bracketsTree.size()/2);
    for ( auto& item : bracketsTree )
    {
        if ( item.isComplete() )
        {
            idxMapping.push_back({idx, m_bracketsTree.size()});
            if ( item.nParentIdx != -1 )
            {
                const auto itrEnd = idxMapping.end();
                const auto itrIdx = std::lower_bound(idxMapping.begin(), itrEnd, item.nParentIdx,
                    [](const auto& item, size_t idx) { return item.first < idx; });
                if ( itrIdx != itrEnd && itrIdx->first == item.nParentIdx )
                    item.nParentIdx = itrIdx->second;
                else
                    item.nParentIdx = -1;
            }
            m_bracketsTree.push_back(std::move(item));
        }
        ++idx;
    }

    std::vector<const tBrPairItem*> bracketsByRightBr;

    bracketsByRightBr.reserve(m_bracketsTree.size());
    for ( const auto& item : m_bracketsTree )
    {
        bracketsByRightBr.push_back(&item);
    }

    std::sort(bracketsByRightBr.begin(), bracketsByRightBr.end(),
        [](const tBrPairItem* pItem1, const tBrPairItem* pItem2){
            return pItem1->nRightBrPos < pItem2->nRightBrPos;
    });

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

    if ( pscn->modificationType & (SC_MOD_DELETETEXT | SC_MOD_BEFOREDELETE) )
    {
        invalidateTree();
        return; // part of a bracket may be deleted
    }

    CSciMessager sciMsgr(reinterpret_cast<HWND>(pscn->nmhdr.hwndFrom));
    if ( sciMsgr.getSelections() > 1 )
    {
        invalidateTree();
        return; // multiple selection
    }

    unsigned int nMaxBrLen = 0;
    bool needsInvalidate = false;

    for ( const auto& brPair : m_pFileSyntax->pairs )
    {
        unsigned int nBrLen = static_cast<unsigned int>(brPair.leftBr.length());
        if ( nMaxBrLen < nBrLen )
            nMaxBrLen = nBrLen;

        nBrLen = static_cast<unsigned int>(brPair.rightBr.length());
        if ( nMaxBrLen < nBrLen )
            nMaxBrLen = nBrLen;
    }

    if ( nMaxBrLen > 1 )
    {
        --nMaxBrLen; // e.g.  <!-|-  or  -|->  in case of  <!-- -->

        // 1. checking the document's text around pscn->position
        const Sci_Position nTextLen = sciMsgr.getTextLength();
        Sci_Position nPos1 = pscn->position >= static_cast<Sci_Position>(nMaxBrLen) ? pscn->position - nMaxBrLen : 0;
        Sci_Position nPos2 = pscn->position + static_cast<Sci_Position>(nMaxBrLen) < nTextLen ? pscn->position + nMaxBrLen : nTextLen;
        unsigned int nOffset = pscn->position >= static_cast<Sci_Position>(nMaxBrLen) ? 0 : nMaxBrLen - static_cast<unsigned int>(pscn->position);

        char szText[20]{};
        sciMsgr.getTextRange(nPos1, nPos2, szText + nOffset);

        for ( auto& item : m_pFileSyntax->pairs )
        {
            unsigned int nBrLen = static_cast<unsigned int>(item.leftBr.length());
            if ( nBrLen > 1 )
            {
                --nBrLen;
                const char* p = strstr(szText + nMaxBrLen - nBrLen, item.leftBr.c_str());
                if ( p != nullptr && p < szText + nMaxBrLen ) // inside the left bracket
                {
                    needsInvalidate = true;
                    break;
                }
            }

            nBrLen = static_cast<unsigned int>(item.rightBr.length());
            if ( nBrLen > 1 )
            {
                --nBrLen;
                const char* p = strstr(szText + nMaxBrLen - nBrLen, item.rightBr.c_str());
                if ( p != nullptr && p < szText + nMaxBrLen ) // inside the right bracket
                {
                    needsInvalidate = true;
                    break;
                }
            }
        }
    }

    if ( pscn->modificationType & SC_MOD_INSERTTEXT )
    {
        if ( !needsInvalidate )
        {
            // 2. checking for inserted new line
            const char* p = pscn->text;
            const char* const pEnd = p + pscn->length;

            for ( ; p != pEnd; ++p )
            {
                const char ch = *p;
                if ( isWhiteSpaceOrNulChar(ch) )
                {
                    // '\n' and '\r' can break single-line brackets and quotes
                    // ' ' and '\t' are significant for isNoInSpKind()
                    needsInvalidate = true;
                    break;
                }
            }
        }

        if ( !needsInvalidate )
        {
            // 3. checking for inserted brackets or quotes
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

    if ( pscn->modificationType & SC_MOD_INSERTTEXT )
    {
        const Sci_Position pos = pscn->position;
        const Sci_Position len = pscn->length;

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
    if ( pBrPair == nullptr )
        return nullptr;

    for ( Sci_Position nParentIdx = pBrPair->nParentIdx; nParentIdx != -1; )
    {
        const tBrPairItem& item = m_bracketsTree[nParentIdx];
        if ( item.isComplete() )
            return &item;

        nParentIdx = item.nParentIdx;
    }

    return nullptr;
}

const tBrPair* CBracketsTree::getLeftBrPair(const char* p, size_t nLen, unsigned int uGetBrFlags) const
{
    if ( m_pFileSyntax == nullptr )
        return nullptr;

    for ( const auto& brPair : m_pFileSyntax->pairs )
    {
        const size_t nLeftBrLen = brPair.leftBr.length();
        if ( nLeftBrLen <= nLen )
        {
            if ( memcmp(p, brPair.leftBr.c_str(), nLeftBrLen*sizeof(char)) == 0 )
            {
                if ( (!isOpnLnStKind(brPair.kind) || ((uGetBrFlags & gbfLnSt) != 0 && ((uGetBrFlags & gbfLdSp) == 0 || isOpnLdSpKind(brPair.kind)))) &&
                     (!isNoInSpKind(brPair.kind) || (nLeftBrLen != nLen && !isWhiteSpaceOrNulChar(*(p + nLeftBrLen)))) )
                {
                    return &brPair;
                }
            }
        }
    }

    return nullptr;
}

std::vector<const tBrPair*> CBracketsTree::getRightBrPair(const char* p, size_t nLen, size_t nCurrentOffset, unsigned int uGetBrFlags) const
{
    if ( m_pFileSyntax == nullptr )
        return {};

    std::vector<const tBrPair*> brPairs;

    for ( const auto& brPair : m_pFileSyntax->pairs )
    {
        const size_t nRightBrLen = brPair.rightBr.length();
        if ( !isSgLnCommKind(brPair.kind) && nRightBrLen <= nLen )
        {
            if ( memcmp(p, brPair.rightBr.c_str(), nRightBrLen*sizeof(char)) == 0 )
            {
                if ( (!isClsLnStKind(brPair.kind) || ((uGetBrFlags & gbfLnSt) != 0 && ((uGetBrFlags & gbfLdSp) == 0 || isClsLdSpKind(brPair.kind)))) &&
                     (!isNoInSpKind(brPair.kind) || (nCurrentOffset != 0 && !isWhiteSpaceOrNulChar(*(p - 1)))) )
                {
                    brPairs.push_back(&brPair);
                }
            }
        }
    }

    return brPairs;
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
            if ( p < prefix || memcmp(p, esqStr.c_str(), esqStr.length()*sizeof(char)) != 0 )
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
    if ( uInvalidateFlags & icbfAutoRightBr )
    {
        m_nAutoRightBracketPos = -1;
        m_nAutoRightBracketType = -1;
        m_nAutoRightBracketOffset = -1;
    }
    if ( uInvalidateFlags & icbfTree )
    {
        if ( !GetOptions().getUpdateTreeAllowed() ||
             pscn == nullptr || 
             (pscn->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREINSERT | SC_MOD_BEFOREDELETE)) == 0 )
        {
            m_bracketsTree.invalidateTree();
        }
        else
        {
            m_bracketsTree.updateTree(pscn);
        }
    }
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::OnCharPress(const int ch)
{
    if ( m_nAutoRightBracketType >= 0 && (m_nAutoRightBracketType & abtfTextJustAutoCompleted) != 0 )
    {
        m_nAutoRightBracketType ^= abtfTextJustAutoCompleted;
        if ( ch == VK_RETURN )
        {
            // the auto-completed text has just been accepted by pressing Enter
            return cprNone;
        }
    }

    const Sci_Position nAutoRightBrPos = m_nAutoRightBracketPos;
    const int nAutoRightBrType = m_nAutoRightBracketType;
    const int nAutoRightBrOffset = m_nAutoRightBracketOffset; // will be used below

    InvalidateCachedBrackets(icbfAutoRightBr);

    if ( !GetOptions().getBracketsAutoComplete() )
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
    return autoBracketsFunc(nLeftBracketType, aboCharPress);
}

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::OnTextAutoCompleted(const char* text, Sci_Position pos)
{
    (pos); // unused

    if ( text == nullptr || m_pFileSyntax == nullptr )
        return cprNone;

    int nLeftBrType = -1;
    int idx = 0;
    for ( const tBrPair& item : m_pFileSyntax->autocomplete )
    {
        if ( item.leftBr == text )
        {
            nLeftBrType = idx;
            break;
        }
        ++idx;
    }

    if ( nLeftBrType == -1 )
        return cprNone;

    return autoBracketsFunc(nLeftBrType, aboTextAutoCompleted);
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
    if ( !isSkipEscapedSupported() || nCharPos <= 0 )
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

    tBracketsJumpState state;
    state.nSelStart = sciMsgr.getSelectionStart(); // current selection
    state.nSelEnd = sciMsgr.getSelectionEnd();     // current selection

    if ( state.nSelStart != state.nSelEnd )
    {
        // something is selected
        state.nLeftBrPos = state.nSelStart;
        state.nRightBrPos = state.nSelEnd;

        bool isWidened = false;
        if ( nBrAction == baSelToNearest && (GetOptions().getSelToNearestFlags() & CXBracketsOptions::snbfWiden) != 0 )
        {
            if ( m_bracketsTree.isTreeEmpty() )
            {
                m_bracketsTree.buildTree(sciMsgr);
            }

            unsigned int uBrPosFlags = 0;
            auto pBrItem = m_bracketsTree.findPairByPos(state.nLeftBrPos, false, &uBrPosFlags);
            auto pBrItem2 = m_bracketsTree.findPairByPos(state.nRightBrPos, false, &uBrPosFlags);
            if ( pBrItem == nullptr ||
                 (pBrItem2 != nullptr && pBrItem2->nRightBrPos - pBrItem2->nLeftBrPos > pBrItem->nRightBrPos - pBrItem->nLeftBrPos) )
            {
                pBrItem = pBrItem2;
            }
            if ( pBrItem != nullptr )
            {
                state.nLeftBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->leftBr.length());
                state.nRightBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->rightBr.length());

                if ( (pBrItem->nLeftBrPos - state.nLeftBrLen < state.nLeftBrPos && pBrItem->nRightBrPos + state.nRightBrLen >= state.nRightBrPos) ||
                     (pBrItem->nLeftBrPos - state.nLeftBrLen <= state.nLeftBrPos && pBrItem->nRightBrPos + state.nRightBrLen > state.nRightBrPos) )
                {
                    // new selection
                    state.nSelStart = pBrItem->nLeftBrPos;
                    state.nSelEnd = pBrItem->nRightBrPos;
                    if ( state.nLeftBrPos < state.nSelStart ||
                         state.nRightBrPos > state.nSelEnd ||
                         (state.nLeftBrPos == state.nSelStart && state.nRightBrPos == state.nSelEnd) )
                    {
                        state.nSelStart -= state.nLeftBrLen;  //  (|  ->  |(
                        state.nSelEnd += state.nRightBrLen; //  |)  ->  )|
                    }
                    if ( sciMsgr.getCurrentPos() == state.nLeftBrPos )
                    {
                        // preserving the selection direction
                        std::swap(state.nSelEnd, state.nSelStart);
                    }
                    isWidened = true;
                }
            }
        }

        if ( !isWidened )
        {
            if ( sciMsgr.getCurrentPos() == state.nSelEnd )
            {
                // inverting the selection direction
                std::swap(state.nSelEnd, state.nSelStart); // new selection
            }
        }

        jumpToPairBracket(sciMsgr, state, false);
        return;
    }

    if ( m_bracketsTree.isTreeEmpty() )
    {
        m_bracketsTree.buildTree(sciMsgr);
    }

    const bool isExactPos = (nBrAction == baGoToMatching || nBrAction == baSelToMatching);
    unsigned int uBrPosFlags = 0;
    const auto pBrItem = m_bracketsTree.findPairByPos(state.nSelStart, isExactPos, &uBrPosFlags);
    if ( pBrItem == nullptr || pBrItem->nRightBrPos == -1 )
        return;

    state.nLeftBrPos = pBrItem->nLeftBrPos; // always (|
    state.nRightBrPos = pBrItem->nRightBrPos; // always |)
    state.nLeftBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->leftBr.length());
    state.nRightBrLen = static_cast<Sci_Position>(pBrItem->pBrPair->rightBr.length());

    Sci_Position nLeftBrPosToSet = state.nLeftBrPos;
    Sci_Position nRightBrPosToSet = state.nRightBrPos;

    if ( ((uBrPosFlags & CBracketsTree::bpfLeftBr) != 0 && state.nSelStart < nLeftBrPosToSet) ||
         ((uBrPosFlags & CBracketsTree::bpfRightBr) != 0 && state.nSelStart > nRightBrPosToSet) )
    {
        // real position
        nLeftBrPosToSet -= state.nLeftBrLen;  //  (|  ->  |(
        nRightBrPosToSet += state.nRightBrLen; //  |)  ->  )|
    }
    else if ( (nBrAction == baGoToNearest && (GetOptions().getGoToNearestFlags() & CXBracketsOptions::gnbfOuterPos) != 0) ||
              (nBrAction == baSelToNearest && (GetOptions().getSelToNearestFlags() & CXBracketsOptions::snbfOuterPos) != 0) )
    {
        // position to set
        nLeftBrPosToSet -= state.nLeftBrLen;  //  (|  ->  |(
        nRightBrPosToSet += state.nRightBrLen; //  |)  ->  )|
    }

    const bool isGoTo = (nBrAction == baGoToMatching || nBrAction == baGoToNearest);
    if ( isGoTo )
    {
        if ( state.nSelStart <= state.nLeftBrPos && state.nSelStart >= state.nLeftBrPos - state.nLeftBrLen )
        {
            // new selection
            state.nSelStart = nRightBrPosToSet;
            state.nSelEnd = nRightBrPosToSet;
        }
        else if ( state.nSelStart >= state.nRightBrPos && state.nSelStart <= state.nRightBrPos + state.nRightBrLen )
        {
            // new selection
            state.nSelStart = nLeftBrPosToSet;
            state.nSelEnd = nLeftBrPosToSet;
        }
        else if ( nBrAction == baGoToNearest )
        {
            if ( (GetOptions().getGoToNearestFlags() & CXBracketsOptions::gnbfLeftBr) != 0 )
            {
                // new selection
                state.nSelStart = nLeftBrPosToSet;
                state.nSelEnd = nLeftBrPosToSet;
            }
            else if ( (GetOptions().getGoToNearestFlags() & CXBracketsOptions::gnbfRightBr) != 0 )
            {
                // new selection
                state.nSelStart = nRightBrPosToSet;
                state.nSelEnd = nRightBrPosToSet;
            }
            else // CXBracketsOptions::gnbfAutoPos
            {
                if ( state.nSelStart - state.nLeftBrPos <= state.nRightBrPos - state.nSelStart )
                {
                    // new selection
                    state.nSelStart = nLeftBrPosToSet;
                    state.nSelEnd = nLeftBrPosToSet;
                }
                else
                {
                    // new selection
                    state.nSelStart = nRightBrPosToSet;
                    state.nSelEnd = nRightBrPosToSet;
                }
            }
        }
    }
    else // baSelToMatching || baSelToNearest
    {
        if ( state.nSelStart >= state.nRightBrPos && state.nSelStart <= state.nRightBrPos + state.nRightBrLen )
        {
            // new selection
            state.nSelStart = nRightBrPosToSet;
            state.nSelEnd = nLeftBrPosToSet;
        }
        else
        {
            // new selection
            state.nSelStart = nLeftBrPosToSet;
            state.nSelEnd = nRightBrPosToSet;
        }
    }

    jumpToPairBracket(sciMsgr, state, isGoTo);
}

const tBrPairItem* CXBracketsLogic::FindBracketsByPos(Sci_Position pos, bool isExactPos)
{
    if ( m_bracketsTree.isTreeEmpty() )
    {
        CSciMessager sciMsgr(m_nppMsgr.getCurrentScintillaWnd());
        m_bracketsTree.buildTree(sciMsgr);
    }

    unsigned int uBrPosFlags = 0;
    return m_bracketsTree.findPairByPos(pos, isExactPos, &uBrPosFlags);
}

static bool isChsAtBr(const char* chs, const std::string& br)
{
    if ( br.length() == 1 )
    {
        if ( br[0] == chs[1] ||  //  |(  or  |)
             br[0] == chs[0] )   //  (|  or  )|
        {
            return true;
        }
    }
    else
    {
        if ( br[0] == chs[1] ||                   //  |((  or  |))
             br.back() == chs[0] ||               //  ((|  or  ))|
             br.find(chs) != std::string::npos )  //  (|(  or  )|)
        {
            return true;
        }
    }
    return false;
}

bool CXBracketsLogic::IsAtBracketPos(const CSciMessager& sciMsgr, Sci_Position pos) const
{
    if ( m_pFileSyntax == nullptr )
        return false;

    char chs[3] = {
        static_cast<char>((pos > 0) ? sciMsgr.getCharAt(pos - 1) : 0), // prev_ch
        static_cast<char>(sciMsgr.getCharAt(pos)), // curr_ch
        0 // '\0'
    };

    for ( const tBrPair& brPair : m_pFileSyntax->pairs )
    {
        if ( isSgLnCommKind(brPair.kind) )
            continue;

        if ( chs[0] != 0 )
        {
            if ( isChsAtBr(chs, brPair.leftBr) ||  //  |(  (|  |((  ((|  (|(
                 isChsAtBr(chs, brPair.rightBr) )  //  |)  )|  |))  ))|  )|)
            {
                return true;
            }
        }
        else // chs[0] == 0
        {
            if ( brPair.leftBr[0] == chs[1] ||  // |((
                 brPair.rightBr[0] == chs[1] )  // |))
            {
                return true;
            }
        }
    }

    return false;
}

void CXBracketsLogic::jumpToPairBracket(CSciMessager& sciMsgr, const tBracketsJumpState& state, bool isGoTo)
{
    sciMsgr.setSel(state.nSelStart, state.nSelEnd);

    if ( GetOptions().getJumpPairLineDiff() < 0 || (GetOptions().getJumpLinesVisUp() <= 0 && GetOptions().getJumpLinesVisDown() <= 0) )
        return;

    const Sci_Position nLeftBrLine = sciMsgr.getLineFromPosition(state.nLeftBrPos);
    const Sci_Position nRightBrLine = sciMsgr.getLineFromPosition(state.nRightBrPos);
    if ( nRightBrLine - nLeftBrLine < GetOptions().getJumpPairLineDiff() )
        return;

    const Sci_Position nFirstVisibleLine = sciMsgr.getFirstVisibleLine();
    if ( (state.nSelStart <= state.nLeftBrPos && state.nSelStart >= state.nLeftBrPos - state.nLeftBrLen && isGoTo) ||
         (state.nSelStart >= state.nRightBrPos && state.nSelStart <= state.nRightBrPos + state.nRightBrLen && !isGoTo) )
    {
        //  {|  <--
        if ( GetOptions().getJumpLinesVisUp() > 0 )
        {
            const Sci_Position nVisLeftBrLine = sciMsgr.getVisibleFromDocLine(nLeftBrLine);
            const Sci_Position nWantLeftBrLine = (nVisLeftBrLine > GetOptions().getJumpLinesVisUp()) ? (nVisLeftBrLine - GetOptions().getJumpLinesVisUp()) : 0;
            if ( nWantLeftBrLine < nFirstVisibleLine )
            {
                sciMsgr.setFirstVisibleLine(nWantLeftBrLine);
            }
        }
    }
    else
    {
        //  -->  |}
        if ( GetOptions().getJumpLinesVisDown() > 0 )
        {
            const Sci_Position nWantRightBrLine = sciMsgr.getVisibleFromDocLine(nRightBrLine) + GetOptions().getJumpLinesVisDown();
            const Sci_Position nLastVisibleLine = nFirstVisibleLine + sciMsgr.getLinesOnScreen();
            const Sci_Position nLineDiff = nWantRightBrLine - nLastVisibleLine;
            if ( nLineDiff > 0 )
            {
                sciMsgr.setFirstVisibleLine(nFirstVisibleLine + nLineDiff);
            }
        }
    }
}

bool CXBracketsLogic::UpdateFileType(unsigned int uInvalidateAndUpdateFlags)
{
    tstr fileExtension;
    const unsigned int uFileType = detectFileType(&fileExtension);
    if ( (uInvalidateAndUpdateFlags & uftfConfigUpdated) == 0 )
    {
        if ( uFileType == m_uFileType && fileExtension == m_fileExtension && m_pFileSyntax != nullptr )
            return false; // file syntax remains the same
    }

    m_uFileType = uFileType;
    m_fileExtension = fileExtension;
    m_pFileSyntax = nullptr;

    if ( m_uFileType & tfmIsSupported )
    {
        for ( const auto& syntax : GetOptions().getFileSyntaxes() )
        {
            if ( (syntax.uFlags & fsfNullFileExt) == 0 &&
                 syntax.fileExtensions.find(fileExtension) != syntax.fileExtensions.end() )
            {
                m_pFileSyntax = &syntax;
                break;
            }
        }
        if ( m_pFileSyntax == nullptr )
            m_pFileSyntax = GetOptions().getDefaultFileSyntax();
    }

    m_bracketsTree.setFileSyntax(m_pFileSyntax);
    InvalidateCachedBrackets(uInvalidateAndUpdateFlags & icbfMask);

    return true; // file syntax changed
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

CXBracketsLogic::eCharProcessingResult CXBracketsLogic::autoBracketsFunc(int nBracketType, eAutoBracketOrigin origin)
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

    bool bPrevCharOK = true;
    bool bNextCharOK = false;
    const char next_ch = sciMsgr.getCharAt(nEditPos);

    if ( isWhiteSpaceOrNulChar(next_ch) ||
         GetOptions().getNextCharOK().find(static_cast<TCHAR>(next_ch)) != tstr::npos )
    {
        bNextCharOK = true;
    }

    int nRightBracketType = getAutocompleteRightBracketType(sciMsgr, next_ch);
    if ( nRightBracketType != -1 )
    {
        bNextCharOK = (nRightBracketType != nBracketType || GetOptions().getBracketsRightExistsOK());
    }

    const tBrPair* pBrPair = getAutoCompleteBrPair(nBracketType);
    if ( bNextCharOK )
    {
        const bool isCheckingDupPair = ((pBrPair->kind & bpakfNoDup) == 0 && pBrPair->leftBr == pBrPair->rightBr);
        const bool isCheckingFlags = ((pBrPair->kind & (bpakfWtSp | bpakfDelim)) != 0);

        Sci_Position nPrevCharPos = nEditPos - static_cast<Sci_Position>(pBrPair->leftBr.length());
        if ( origin == aboTextAutoCompleted )
        {
            nPrevCharPos -= 1;
        }

        if ( isCheckingDupPair || isCheckingFlags )
        {
            bool bDupPairOK = true;
            bool bFlagsOK = true;

            // previous character
            const char prev_ch = (nPrevCharPos >= 0) ? sciMsgr.getCharAt(nPrevCharPos) : 0;

            if ( isCheckingDupPair )
            {
                bDupPairOK = ( isWhiteSpaceOrNulChar(prev_ch) ||
                    GetOptions().getPrevCharOK().find(static_cast<TCHAR>(prev_ch)) != tstr::npos );
            }

            if ( isCheckingFlags && prev_ch != 0 )
            {
                bFlagsOK = ( ((pBrPair->kind & bpakfWtSp) != 0 && isWhiteSpaceOrNulChar(prev_ch)) ||
                     ((pBrPair->kind & bpakfDelim) != 0 && GetOptions().getDelimiters().find(static_cast<TCHAR>(prev_ch)) != tstr::npos) );
            }

            bPrevCharOK = (bDupPairOK && bFlagsOK);
        }

        if ( bPrevCharOK && isEscapedPos(sciMsgr, nPrevCharPos + 1) )
            bPrevCharOK = false;
    }

    if ( !bPrevCharOK || !bNextCharOK )
        return cprNone;

    nEditEndPos = nEditPos;

    size_t nBrLen = pBrPair->rightBr.length(); // the right bracket
    if ( origin == aboCharPress )
    {
        ++nBrLen; // the last character of the left bracket
        ++nEditPos; // placing the caret between the brackets
        ++nEditEndPos;
    }
    if ( (pBrPair->kind & bpakfAddSpSel) != 0 )
    {
        ++nBrLen; // the additional space
        ++nEditEndPos; // selecting the space between the brackets
    }
    else if ( (pBrPair->kind & bpakfAddTwoSp) != 0 )
    {
        nBrLen += 2; // two additional spaces
        ++nEditPos; // between the two spaces
        ++nEditEndPos;
    }

    std::string sBr;
    sBr.reserve(nBrLen);
    if ( origin == aboCharPress )
        sBr += pBrPair->leftBr.back(); // the last character of the left bracket
    if ( (pBrPair->kind & bpakfAddSpSel) != 0 )
        sBr += ' '; // the additional space
    else if ( (pBrPair->kind & bpakfAddTwoSp) != 0 )
        sBr += "  "; // two additional spaces
    sBr += pBrPair->rightBr; // the right bracket

    sciMsgr.beginUndoAction();
    // inserting the right bracket
    sciMsgr.replaceSelText(sBr.c_str());
    sciMsgr.setSel(nEditPos, nEditEndPos);
    sciMsgr.endUndoAction();

    if ( (pBrPair->kind & (bpakfAddSpSel | bpakfAddTwoSp)) == 0 )
    {
        m_nAutoRightBracketPos = nEditPos;
        m_nAutoRightBracketType = nBracketType;
        if ( origin == aboTextAutoCompleted )
        {
            m_nAutoRightBracketType |= abtfTextJustAutoCompleted;
        }
        m_nAutoRightBracketOffset = 0;
    }

    return cprBrAutoCompl;
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
    const UINT uSelAutoBr = GetOptions().getBracketsSelAutoBr();
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

        if ( GetOptions().IsSupportedFile(pszExt) )
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
