#include "XBracketsOptions.h"


CXBracketsOptions::CXBracketsOptions()
{
    // initial values
    m_bBracketsAutoComplete = false;
    m_bBracketsRightExistsOK = false;
    m_bBracketsDoSingleQuote = false;
    m_bBracketsDoTag = false;
    m_bBracketsDoTag2 = false;
    m_bBracketsDoTagIf = false;
    m_bBracketsSkipEscaped = false;
    lstrcpy( m_szHtmlFileExts, _T("htm; xml; php") );
    m_sFileExtsRule.clear();
    m_bSaveFileExtsRule = false;
}

CXBracketsOptions::~CXBracketsOptions()
{
}

UINT CXBracketsOptions::getOptFlags() const
{
    UINT uFlags = 0;

    if ( m_bBracketsAutoComplete )
        uFlags |= OPTF_AUTOCOMPLETE;
    if ( m_bBracketsRightExistsOK )
        uFlags |= OPTF_RIGHTBRACKETOK;
    if ( m_bBracketsDoSingleQuote )
        uFlags |= OPTF_DOSINGLEQUOTE;
    if ( m_bBracketsDoTag )
        uFlags |= OPTF_DOTAG;
    if ( m_bBracketsDoTag2 )
        uFlags |= OPTF_DOTAG2;
    if ( m_bBracketsDoTagIf )
        uFlags |= OPTF_DOTAGIF;
    if ( m_bBracketsSkipEscaped )
        uFlags |= OPTF_SKIPESCAPED;

    return uFlags;
}

bool CXBracketsOptions::MustBeSaved() const
{
    return ( m_bSaveFileExtsRule ||
             (getOptFlags() != m_uFlags0) ||
             (lstrcmpi(m_szHtmlFileExts, m_szHtmlFileExts0) != 0) );
}

static int str_unsafe_subcmp(const TCHAR* str, const TCHAR* substr)
{
    while ( (*str) && (*str == *substr) )
    {
        ++str;
        ++substr;
    }
    return (*substr) ? (*str - *substr) : 0;
}

bool CXBracketsOptions::IsHtmlCompatible(const TCHAR* szExt) const
{
    TCHAR szHtmlExt[MAX_EXT];
    int   n = 0;
    int   i = 0;
    int   len = lstrlen(m_szHtmlFileExts);

    while ( i <= len )
    {
        if ( (m_szHtmlFileExts[i]) &&
             (m_szHtmlFileExts[i] != _T(';')) &&
             (m_szHtmlFileExts[i] != _T(',')) &&
             (m_szHtmlFileExts[i] != _T(' ')) )
        {
            szHtmlExt[n++] = m_szHtmlFileExts[i];
        }
        else
        {
            if ( n > 0 )
            {
                szHtmlExt[n] = 0;
                n = 0;
                while ( szExt[n] )
                {
                    if ( str_unsafe_subcmp(szExt + n, szHtmlExt) == 0 )
                        return true;
                    else
                        ++n;
                }
            }
            n = 0;
        }
        ++i;
    }
    return false;
}

static inline bool isTabSpace(const TCHAR ch)
{
    return ( ch == _T(' ') || ch == _T('\t') );
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
        m_bBracketsAutoComplete = (m_uFlags0 & OPTF_AUTOCOMPLETE) ? true : false;
        m_bBracketsRightExistsOK = (m_uFlags0 & OPTF_RIGHTBRACKETOK) ? true : false;
        m_bBracketsDoSingleQuote = (m_uFlags0 & OPTF_DOSINGLEQUOTE) ? true : false;
        m_bBracketsDoTag = (m_uFlags0 & OPTF_DOTAG) ? true : false;
        m_bBracketsDoTag2 = (m_uFlags0 & OPTF_DOTAG2) ? true : false;
        m_bBracketsDoTagIf = (m_uFlags0 & OPTF_DOTAGIF) ? true : false;
        m_bBracketsSkipEscaped = (m_uFlags0 & OPTF_SKIPESCAPED) ? true : false;
    }

    ::GetPrivateProfileString( _T("Options"), _T("HtmlFileExts"), 
        m_szHtmlFileExts, m_szHtmlFileExts0, STR_FILEEXTS_SIZE - 1, szIniFilePath );
    lstrcpy( m_szHtmlFileExts, m_szHtmlFileExts0 );

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
    UINT  uFlags = getOptFlags();
    
    ::wsprintf(szNum, _T("%u"), uFlags);
    if ( ::WritePrivateProfileString(_T("Options"), _T("Flags"), szNum, szIniFilePath) )
        m_uFlags0 = uFlags;

    if ( ::WritePrivateProfileString(_T("Options"), _T("HtmlFileExts"), m_szHtmlFileExts, szIniFilePath) )
        lstrcpy(m_szHtmlFileExts0, m_szHtmlFileExts);

    if ( m_bSaveFileExtsRule )
    {
        if ( ::WritePrivateProfileString(_T("Options"), _T("FileExtsRule"), m_sFileExtsRule.c_str(), szIniFilePath) )
            m_bSaveFileExtsRule = false;
    }
}
