#include "XBracketsOptions.h"
#include "json11/json11.hpp"
#include <algorithm>
#include <map>

using namespace XBrackets;

namespace
{
    inline bool isTabSpace(const TCHAR ch)
    {
        return ( ch == _T(' ') || ch == _T('\t') );
    }

    bool isExtBoundary(const tstr& exts, size_t pos)
    {
        if ( pos == 0 || pos == exts.length() )
            return true; // very beginning or end of the string

        switch ( exts[pos] )
        {
        case _T(';'):
        case _T(','):
        case _T(' '):
            return true; // separator character
        }

        return false;
    }

    bool isExtInExts(const TCHAR* szExt, const tstr& exts)
    {
        bool result = false;

        if ( szExt )
        {
            size_t ext_len = static_cast<size_t>(lstrlen(szExt));
            size_t pos = 0;

            for ( ; ; )
            {
                pos = exts.find(szExt, pos, ext_len);
                if ( pos == tstr::npos )
                    break; // not found

                size_t next_pos = pos + ext_len;
                result = isExtBoundary(exts, pos != 0 ? pos - 1 : 0) && isExtBoundary(exts, next_pos);
                if ( result )
                    break; // found

                pos = next_pos + 1;
            }
        }

        return result;
    }

    bool isExtPartiallyInExts(const TCHAR* szExt, const tstr& exts)
    {
        bool result = false;

        if ( szExt )
        {
            size_t pos = 0;
            tstr ext;
            tstr search_ext(szExt);

            for ( ; ; )
            {
                size_t next_pos = exts.find_first_of(_T(";, "), pos);
                size_t ext_len = (next_pos != tstr::npos) ? (next_pos - pos) : (exts.length() - pos);
                ext.assign(exts, pos, ext_len);
                result = (search_ext.find(ext) != tstr::npos);
                if ( result )
                    break; // found

                if ( next_pos == tstr::npos )
                    break; // not found

                pos = next_pos + 1;
                while ( pos < exts.length() && exts[pos] == _T(' ') )
                    ++pos; // skip spaces
            }
        }

        return result;
    }
}

static const TCHAR INI_SECTION_OPTIONS[] = _T("Options");
static const TCHAR INI_OPTION_FLAGS[] = _T("Flags");
static const TCHAR INI_OPTION_HTMLFILEEXTS[] = _T("HtmlFileExts");
static const TCHAR INI_OPTION_ESCAPEDFILEEXTS[] = _T("EscapedFileExts");
static const TCHAR INI_OPTION_SGLQUOTEFILEEXTS[] = _T("SingleQuoteFileExts");
static const TCHAR INI_OPTION_FILEEXTSRULE[] = _T("FileExtsRule");

CXBracketsOptions::CXBracketsOptions() :
  // default values:
  m_uFlags(OPTF_AUTOCOMPLETE),
  m_uSelAutoBr(sabNone),
  m_sNextCharOK(_T(".,!?:;</")),
  m_sPrevCharOK(_T("([{<=")),
  m_pDefaultFileSyntax(nullptr)
{
}

CXBracketsOptions::~CXBracketsOptions()
{
}

bool CXBracketsOptions::MustBeSaved() const
{
    return false;
}

bool CXBracketsOptions::IsSupportedFile(const TCHAR* szExt) const
{
    if ( m_sFileExtsRule.empty() || !szExt )
        return true;

    bool            bExclude = false;
    tstr::size_type n = 0;

    if ( m_sFileExtsRule[0] == _T('-') )
    {
        bExclude = true;
        n = 1;
    }
    else if ( m_sFileExtsRule[0] == _T('+') )
    {
        n = 1;
    }

    const tstr::size_type len = m_sFileExtsRule.length();
    while ( n < len && isTabSpace(m_sFileExtsRule[n]) )  ++n;
    if ( n == len )
        return true;

    const unsigned int lenExt = lstrlen(szExt);
    tstr::size_type pos = m_sFileExtsRule.find(szExt, n, lenExt);
    while ( pos != tstr::npos )
    {
        if ( (pos == n) || isTabSpace(m_sFileExtsRule[pos - 1]) )
        {
            if ( (pos + lenExt == len) || isTabSpace(m_sFileExtsRule[pos + lenExt]) )
                return !bExclude;
        }
        pos = m_sFileExtsRule.find(szExt, pos + 1, lenExt);
    }

    return bExclude;
}

static tBrPair readBrPairItem(const json11::Json& pairItem, bool isKindRequired)
{
    tBrPair brPair;

    for ( const auto& elem : pairItem.array_items() )
    {
        if ( elem.is_string() )
        {
            const std::string& val = elem.string_value();

            eBrPairKind kind = bpkNone;
            if ( isKindRequired )
            {
                if ( brPair.kind == bpkNone )
                {
                    if ( val == "single-line-brackets" )
                        kind = bpkSgLnBrackets;
                    else if ( val == "single-line-brackets-noinnerspace" )
                        kind = bpkSgLnBracketsNoInnerSpace;
                    else if ( val == "multi-line-brackets" )
                        kind = bpkMlLnBrackets;
                    else if ( val == "multi-line-brackets-linestart" )
                        kind = bpkMlLnBracketsLineStart;
                    else if ( val == "single-line-quotes" )
                        kind = bpkSgLnQuotes;
                    else if ( val == "single-line-quotes-noinnerspace" )
                        kind = bpkSgLnQuotesNoInnerSpace;
                    else if ( val == "multi-line-quotes" )
                        kind = bpkMlLnQuotes;
                    else if ( val == "multi-line-quotes-linestart" )
                        kind = bpkMlLnQuotesLineStart;
                    else if ( val == "single-line-comment" )
                        kind = bpkSgLnComm;
                    else if ( val == "single-line-comment-linestart" )
                        kind = bpkSgLnCommLineStart;
                    else if ( val == "multi-line-comment" )
                        kind = bpkMlLnComm;
                    else if ( val == "multi-line-comment-linestart" )
                        kind = bpkMlLnCommLineStart;
                    else if ( val == "quote-escape-char" )
                        kind = bpkQtEsqChar;
                }
            }

            if ( kind != bpkNone )
                brPair.kind = kind;
            else if ( brPair.leftBr.empty() )
                brPair.leftBr = val;
            else if ( brPair.rightBr.empty() && brPair.kind != bpkQtEsqChar )
                brPair.rightBr = val;
        }
    }

    return brPair;
}

static void sort_brpairs_by_len(std::vector<tBrPair>& brpairs)
{
    if ( brpairs.empty() )
        return;

    // longer pairs are placed first to avoid intersections with
    // characters of shorter pairs
    std::sort(brpairs.begin(), brpairs.end(),
        [](const tBrPair& item1, const tBrPair& item2)
        {
            const auto lenLeftBr1 = item1.leftBr.length();
            const auto lenLeftBr2 = item2.leftBr.length();
            return (lenLeftBr1 > lenLeftBr2 || 
                (lenLeftBr1 == lenLeftBr2 && item1.rightBr.length() > item2.rightBr.length()));
        }
    );
}

static tFileSyntax readFileSyntaxItem(const json11::Json& syntaxItem)
{
    tFileSyntax fileSyntax;

    for ( const auto& propItem : syntaxItem.object_items() )
    {
        if ( propItem.first == "name" )
        {
            if ( propItem.second.is_string() )
            {
                fileSyntax.name = propItem.second.string_value();
            }
        }
        else if ( propItem.first == "parent" )
        {
            if ( propItem.second.is_string() )
            {
                fileSyntax.parent = propItem.second.string_value();
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
            else if ( propItem.second.is_null() )
            {
                fileSyntax.uFlags |= fsfNullFileExt;
            }
        }
        else if ( propItem.first == "syntax" )
        {
            if ( propItem.second.is_array() )
            {
                for ( const auto& elementItem : propItem.second.array_items() )
                {
                    if ( elementItem.is_array() )
                    {
                        tBrPair brPair = readBrPairItem(elementItem, true);

                        if ( brPair.kind != bpkNone )
                        {
                            if ( brPair.kind != bpkQtEsqChar )
                                fileSyntax.pairs.push_back(std::move(brPair));
                            else
                                fileSyntax.qtEsc.push_back(std::move(brPair.leftBr));
                        }
                    }
                }
                sort_brpairs_by_len(fileSyntax.pairs);
            }
        }
        else if ( propItem.first == "autocomplete" )
        {
            if ( propItem.second.is_array() )
            {
                for ( const auto& autocomplItem : propItem.second.array_items() )
                {
                    if ( autocomplItem.is_array() )
                    {
                        tBrPair brPair = readBrPairItem(autocomplItem, false);

                        if ( !brPair.leftBr.empty() && !brPair.rightBr.empty() )
                        {
                            fileSyntax.autocomplete.push_back(std::move(brPair));
                        }
                    }
                }
                sort_brpairs_by_len(fileSyntax.autocomplete);
            }
        }
    }

    return fileSyntax;
}

static void postprocessSyntaxes(std::list<tFileSyntax>& fileSyntaxes, const tFileSyntax** ppDefaultFileSyntax)
{
    std::map<std::string, const tFileSyntax*> parentSyntaxes;

    *ppDefaultFileSyntax = nullptr;

    for ( const auto& fileSyntax : fileSyntaxes )
    {
        parentSyntaxes[fileSyntax.name] = &fileSyntax;
    }

    for ( auto& fileSyntax : fileSyntaxes )
    {
        if ( fileSyntax.fileExtensions.empty() )
            *ppDefaultFileSyntax = &fileSyntax;

        if ( fileSyntax.parent.empty() )
            continue;

        const auto itrParent = parentSyntaxes.find(fileSyntax.parent);
        if ( itrParent == parentSyntaxes.end() )
            continue;

        const tFileSyntax* pParentSyntax = itrParent->second;

        if ( !pParentSyntax->pairs.empty() )
        {
            std::vector<tBrPair> brPairs;
            std::swap(fileSyntax.pairs, brPairs);
            fileSyntax.pairs = pParentSyntax->pairs;
            for ( const tBrPair& brPair : brPairs )
            {
                const auto itrBegin = fileSyntax.pairs.begin();
                const auto itrEnd = fileSyntax.pairs.end();
                auto itr = std::find_if(itrBegin, itrEnd, 
                    [&brPair](const tBrPair& brp){
                    return brp.leftBr == brPair.leftBr && brp.rightBr == brPair.rightBr;
                }
                );
                if ( itr != itrEnd )
                    itr->kind = brPair.kind;
                else
                    fileSyntax.pairs.push_back(brPair);
            }
            sort_brpairs_by_len(fileSyntax.pairs);
        }

        if ( !pParentSyntax->autocomplete.empty() )
        {
            std::vector<tBrPair> autocompl;
            std::swap(fileSyntax.autocomplete, autocompl);
            fileSyntax.autocomplete = pParentSyntax->autocomplete;
            fileSyntax.autocomplete.insert(fileSyntax.autocomplete.end(), autocompl.begin(), autocompl.end());
            sort_brpairs_by_len(fileSyntax.autocomplete);
        }

        if ( !pParentSyntax->qtEsc.empty() )
        {
            if ( fileSyntax.qtEsc.empty() )
                fileSyntax.qtEsc = pParentSyntax->qtEsc;
        }
    }
}

void CXBracketsOptions::readConfigSettingsItem(const void* pContext)
{
    const auto pJsonItem = reinterpret_cast<const json11::Json*>(pContext);
    if ( !pJsonItem->is_object() )
        return;

    for ( const auto& settingItem : pJsonItem->object_items() )
    {
        const auto& settingName = settingItem.first;
        const auto& settingVal = settingItem.second;
        if ( settingName == "AutoComplete" )
        {
            if ( settingVal.is_bool() )
                setBracketsAutoComplete(settingVal.bool_value());
        }
        else if ( settingName == "AutoComplete_WhenRightBrExists" )
        {
            if ( settingVal.is_bool() )
                setBracketsRightExistsOK(settingVal.bool_value());
        }
        else if ( settingName == "Sel_AutoBr" )
        {
            if ( settingVal.is_number() )
                m_uSelAutoBr = settingVal.int_value();
        }
        else if ( settingName == "Next_Char_OK" )
        {
            if ( settingVal.is_string() )
                m_sNextCharOK = string_to_tstr(settingVal.string_value());
        }
        else if ( settingName == "Prev_Char_OK" )
        {
            if ( settingVal.is_string() )
                m_sPrevCharOK = string_to_tstr(settingVal.string_value());
        }
        else if ( settingName == "FileExtsRule" )
        {
            if ( settingVal.is_string() )
                m_sFileExtsRule = string_to_tstr_changecase(settingVal.string_value(), scLower);
        }
    }
}

tstr CXBracketsOptions::ReadConfig(const tstr& cfgFilePath)
{
    std::list<tFileSyntax> fileSyntaxes;
    const tFileSyntax* pDefaultFileSyntax = nullptr;

    const auto jsonBuf = readFile(cfgFilePath.c_str());
    if ( jsonBuf.empty() )
    {
        tstr msg(_T("Could not read the config file:\r\n\r\n"));
        msg += cfgFilePath;
        return msg;
    }

    std::string err;
    const auto jsonObj = json11::Json::parse(jsonBuf.data(), err, json11::COMMENTS);
    if ( jsonObj.is_null() )
    {
        tstr msg(_T("Failed to parse the JSON config:\r\n\r\n"));
        msg += string_to_tstr(err);
        return msg;
    }

    if ( !jsonObj.is_object() )
    {
        return tstr(_T("The JSON config is not a valid JSON object"));
    }

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
                        fileSyntaxes.push_back(readFileSyntaxItem(syntaxItem));
                    }
                }
            }
        }
        else if ( item.first == "settings" )
        {
            readConfigSettingsItem(&item.second);
        }
    }

    postprocessSyntaxes(fileSyntaxes, &pDefaultFileSyntax);

    // OK, finally:
    std::swap(m_fileSyntaxes, fileSyntaxes);
    std::swap(m_pDefaultFileSyntax, pDefaultFileSyntax);

    return tstr();
}

void CXBracketsOptions::ReadOptions(const TCHAR* szIniFilePath)
{
    (szIniFilePath);
}

void CXBracketsOptions::SaveOptions(const TCHAR* szIniFilePath)
{
    (szIniFilePath);
}
