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
static const TCHAR INI_OPTION_SELAUTOBR[] = _T("Sel_AutoBr");
static const TCHAR INI_OPTION_NEXTCHAROK[] = _T("Next_Char_OK");
static const TCHAR INI_OPTION_PREVCHAROK[] = _T("Prev_Char_OK");
static const TCHAR INI_OPTION_HTMLFILEEXTS[] = _T("HtmlFileExts");
static const TCHAR INI_OPTION_ESCAPEDFILEEXTS[] = _T("EscapedFileExts");
static const TCHAR INI_OPTION_SGLQUOTEFILEEXTS[] = _T("SingleQuoteFileExts");
static const TCHAR INI_OPTION_FILEEXTSRULE[] = _T("FileExtsRule");

CXBracketsOptions::CXBracketsOptions() :
  // default values:
  m_uFlags(OPTF_AUTOCOMPLETE | OPTF_DOSINGLEQUOTE | OPTF_DOSINGLEQUOTEIF | OPTF_DOTAG | OPTF_DOTAGIF | OPTF_SKIPESCAPED),
  m_uFlags0(-1),
  m_uSelAutoBr(sabNone),
  m_uSelAutoBr0(-1),
  m_bSaveFileExtsRule(false),
  m_bSaveNextCharOK(false),
  m_bSavePrevCharOK(false),
  m_sHtmlFileExts(_T("htm; xml; php")),
  m_sEscapedFileExts(_T("cs; java; js; php; rc")),
  m_sSglQuoteFileExts(_T("js; pas; py; ps1; sh; htm; html; xml")),
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
    return ( m_bSaveFileExtsRule ||
             m_bSaveNextCharOK ||
             m_bSavePrevCharOK ||
             m_uFlags != m_uFlags0 ||
             m_uSelAutoBr != m_uSelAutoBr0 ||
             lstrcmpi(m_sHtmlFileExts.c_str(), m_sHtmlFileExts0.c_str()) != 0 ||
             lstrcmpi(m_sEscapedFileExts.c_str(), m_sEscapedFileExts0.c_str()) != 0 ||
             lstrcmpi(m_sSglQuoteFileExts.c_str(), m_sSglQuoteFileExts0.c_str()) != 0
        );
}

bool CXBracketsOptions::IsHtmlCompatible(const TCHAR* szExt) const
{
    return isExtPartiallyInExts(szExt, m_sHtmlFileExts);
}

bool CXBracketsOptions::IsEscapedFileExt(const TCHAR* szExt) const
{
    return isExtInExts(szExt, m_sEscapedFileExts);
}

bool CXBracketsOptions::IsSingleQuoteFileExt(const TCHAR* szExt) const
{
    return isExtInExts(szExt, m_sSglQuoteFileExts);
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
                    else if ( val == "multi-line-brackets" )
                        kind = bpkMlLnBrackets;
                    else if ( val == "single-line-quotes" )
                        kind = bpkSgLnQuotes;
                    else if ( val == "multi-line-quotes" )
                        kind = bpkMlLnQuotes;
                    else if ( val == "single-line-comment" )
                        kind = bpkSgLnComm;
                    else if ( val == "multi-line-comment" )
                        kind = bpkMlLnComm;
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
        }

        if ( !pParentSyntax->autocomplete.empty() )
        {
            std::vector<tBrPair> autocompl;
            std::swap(fileSyntax.autocomplete, autocompl);
            fileSyntax.autocomplete = pParentSyntax->autocomplete;
            fileSyntax.autocomplete.insert(fileSyntax.autocomplete.end(), autocompl.begin(), autocompl.end());
        }

        if ( !pParentSyntax->qtEsc.empty() )
        {
            if ( fileSyntax.qtEsc.empty() )
                fileSyntax.qtEsc = pParentSyntax->qtEsc;
        }
    }
}

void CXBracketsOptions::ReadConfig(const tstr& cfgFilePath)
{
    std::list<tFileSyntax> fileSyntaxes;
    const tFileSyntax* pDefaultFileSyntax = nullptr;

    const auto jsonBuf = readFile(cfgFilePath.c_str());
    if ( jsonBuf.empty() )
        return; // TODO: show an error "Could not open the file"

    std::string err;
    const auto jsonObj = json11::Json::parse(jsonBuf.data(), err);
    if ( jsonObj.is_null() )
        return; // TODO: show an error "Failed to parse JSON, the error is: ..."

    if ( !jsonObj.is_object() )
        return; // TODO: show an error "JSON config is not a valid JSON object"

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
            // TODO: read the settings here
        }
    }

    postprocessSyntaxes(fileSyntaxes, &pDefaultFileSyntax);

    // OK, finally:
    std::swap(m_fileSyntaxes, fileSyntaxes);
    std::swap(m_pDefaultFileSyntax, pDefaultFileSyntax);
}

void CXBracketsOptions::ReadOptions(const TCHAR* szIniFilePath)
{
    const TCHAR NOKEYSTR[] = _T("=:=%*@$^!~#");
    TCHAR szTempExts[STR_FILEEXTS_SIZE];

    m_uFlags0 = ::GetPrivateProfileInt( INI_SECTION_OPTIONS, INI_OPTION_FLAGS, -1, szIniFilePath );
    if ( m_uFlags0 != (UINT) (-1) )
    {
        m_uFlags = m_uFlags0;
    }

    m_uSelAutoBr0 = ::GetPrivateProfileInt( INI_SECTION_OPTIONS, INI_OPTION_SELAUTOBR, -1, szIniFilePath );
    if ( m_uSelAutoBr0 != (UINT) (-1) )
    {
        m_uSelAutoBr = m_uSelAutoBr0;
    }

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_HTMLFILEEXTS,
        m_sHtmlFileExts.c_str(), szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    ::CharLower(szTempExts);
    m_sHtmlFileExts = szTempExts;
    m_sHtmlFileExts0 = m_sHtmlFileExts;

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_ESCAPEDFILEEXTS,
        m_sEscapedFileExts.c_str(), szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    ::CharLower(szTempExts);
    m_sEscapedFileExts = szTempExts;
    m_sEscapedFileExts0 = m_sEscapedFileExts;

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_SGLQUOTEFILEEXTS,
        m_sSglQuoteFileExts.c_str(), szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    ::CharLower(szTempExts);
    m_sSglQuoteFileExts = szTempExts;
    m_sSglQuoteFileExts0 = m_sSglQuoteFileExts;

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_FILEEXTSRULE,
        NOKEYSTR, szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    if ( lstrcmp(szTempExts, NOKEYSTR) == 0 )
    {
        m_bSaveFileExtsRule = true;
    }
    else
    {
        int i = 0;
        while ( isTabSpace(szTempExts[i]) )  ++i;
        if ( szTempExts[i] )
        {
            ::CharLower(szTempExts);
            m_sFileExtsRule = &szTempExts[i];
        }
    }

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_NEXTCHAROK,
        NOKEYSTR, szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    if ( lstrcmp(szTempExts, NOKEYSTR) == 0 )
    {
        m_bSaveNextCharOK = true;
    }
    else
    {
        m_sNextCharOK = szTempExts;
    }

    szTempExts[0] = 0;
    ::GetPrivateProfileString( INI_SECTION_OPTIONS, INI_OPTION_PREVCHAROK,
        NOKEYSTR, szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    if ( lstrcmp(szTempExts, NOKEYSTR) == 0 )
    {
        m_bSavePrevCharOK = true;
    }
    else
    {
        m_sPrevCharOK = szTempExts;
    }
}

void CXBracketsOptions::SaveOptions(const TCHAR* szIniFilePath)
{
    TCHAR szNum[10];

    ::wsprintf(szNum, _T("%u"), m_uFlags);
    if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_FLAGS, szNum, szIniFilePath) )
        m_uFlags0 = m_uFlags;

    if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_HTMLFILEEXTS, m_sHtmlFileExts.c_str(), szIniFilePath) )
        m_sHtmlFileExts0 = m_sHtmlFileExts;

    if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_ESCAPEDFILEEXTS, m_sEscapedFileExts.c_str(), szIniFilePath) )
        m_sEscapedFileExts0 = m_sEscapedFileExts;

    if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_SGLQUOTEFILEEXTS, m_sSglQuoteFileExts.c_str(), szIniFilePath) )
        m_sSglQuoteFileExts0 = m_sSglQuoteFileExts;

    if ( m_bSaveFileExtsRule )
    {
        if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_FILEEXTSRULE, m_sFileExtsRule.c_str(), szIniFilePath) )
            m_bSaveFileExtsRule = false;
    }

    if ( m_uSelAutoBr != m_uSelAutoBr0 )
    {
        ::wsprintf(szNum, _T("%u"), m_uSelAutoBr);
        if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_SELAUTOBR, szNum, szIniFilePath) )
            m_uSelAutoBr0 = m_uSelAutoBr;
    }

    if ( m_bSaveNextCharOK )
    {
        if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_NEXTCHAROK, m_sNextCharOK.c_str(), szIniFilePath) )
            m_bSaveNextCharOK = false;
    }

    if ( m_bSavePrevCharOK )
    {
        if ( ::WritePrivateProfileString(INI_SECTION_OPTIONS, INI_OPTION_PREVCHAROK, m_sPrevCharOK.c_str(), szIniFilePath) )
            m_bSavePrevCharOK = false;
    }
}
