#include "XBracketsOptions.h"

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
                result = isExtBoundary(exts, pos) && isExtBoundary(exts, next_pos);
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

CXBracketsOptions::CXBracketsOptions()
{
    // initial values
    m_uFlags = 0;
    m_uFlags0 = 0;
    m_bSaveFileExtsRule = false;
    m_sHtmlFileExts = _T("htm; xml; php");
}

CXBracketsOptions::~CXBracketsOptions()
{
}

bool CXBracketsOptions::MustBeSaved() const
{
    return ( m_bSaveFileExtsRule ||
             m_uFlags != m_uFlags0 ||
             lstrcmpi(m_sHtmlFileExts.c_str(), m_sHtmlFileExts0.c_str()) != 0 );
}

bool CXBracketsOptions::IsHtmlCompatible(const TCHAR* szExt) const
{
    return isExtPartiallyInExts(szExt, m_sHtmlFileExts);
}

bool CXBracketsOptions::IsEscapedFileExt(const TCHAR* szExt) const
{
    return true; // isExtInExts(szExt, m_sEscapedFileExts);
}

bool CXBracketsOptions::IsSingleQuoteFileExt(const TCHAR* szExt) const
{
    return true; // isExtInExts(szExt, m_sSingleQuoteFileExts);
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

void CXBracketsOptions::ReadOptions(const TCHAR* szIniFilePath)
{
    const TCHAR* NOKEYSTR = _T("%*@$^!~#");
    TCHAR szTempExts[STR_FILEEXTS_SIZE];

    m_uFlags0 = ::GetPrivateProfileInt( _T("Options"), _T("Flags"), -1, szIniFilePath );
    if ( m_uFlags0 != (UINT) -1 )
    {
        m_uFlags = m_uFlags0;
    }

    szTempExts[0] = 0;
    ::GetPrivateProfileString( _T("Options"), _T("HtmlFileExts"),
        m_sHtmlFileExts.c_str(), szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    m_sHtmlFileExts = szTempExts;
    m_sHtmlFileExts0 = m_sHtmlFileExts;

    szTempExts[0] = 0;
    ::GetPrivateProfileString( _T("Options"), _T("FileExtsRule"),
        NOKEYSTR, szTempExts, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    if ( lstrcmp(szTempExts, NOKEYSTR) == 0 )
    {
        szTempExts[0] = 0;
        m_bSaveFileExtsRule = true;
    }

    int i = 0;
    while ( isTabSpace(szTempExts[i]) )  ++i;
    if ( szTempExts[i] )
    {
        ::CharLower(szTempExts);
        m_sFileExtsRule = &szTempExts[i];
    }
}

void CXBracketsOptions::SaveOptions(const TCHAR* szIniFilePath)
{
    TCHAR szNum[10];

    ::wsprintf(szNum, _T("%u"), m_uFlags);
    if ( ::WritePrivateProfileString(_T("Options"), _T("Flags"), szNum, szIniFilePath) )
        m_uFlags0 = m_uFlags;

    if ( ::WritePrivateProfileString(_T("Options"), _T("HtmlFileExts"), m_sHtmlFileExts.c_str(), szIniFilePath) )
        m_sHtmlFileExts0 = m_sHtmlFileExts;

    if ( m_bSaveFileExtsRule )
    {
        if ( ::WritePrivateProfileString(_T("Options"), _T("FileExtsRule"), m_sFileExtsRule.c_str(), szIniFilePath) )
            m_bSaveFileExtsRule = false;
    }
}
