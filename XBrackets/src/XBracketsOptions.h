#ifndef _xbrackets_npp_options_h_
#define _xbrackets_npp_options_h_
//---------------------------------------------------------------------------
#include "core/base.h"
#include <string>

typedef std::basic_string<TCHAR> tstr;

class CXBracketsOptions
{
    public:
        CXBracketsOptions();
        ~CXBracketsOptions();

        bool MustBeSaved() const;
        bool IsHtmlCompatible(const TCHAR* szExt) const;
        bool IsSupportedFile(const TCHAR* szExt) const;
        void ReadOptions(const TCHAR* szIniFilePath);
        void SaveOptions(const TCHAR* szIniFilePath);

    protected:
        UINT getOptFlags() const;

    public:
        enum eConsts {
            MAX_EXT             = 50,
            STR_FILEEXTS_SIZE   = 500
        };

        bool  m_bBracketsAutoComplete;
        bool  m_bBracketsRightExistsOK;
        bool  m_bBracketsDoSingleQuote;
        bool  m_bBracketsDoTag;
        bool  m_bBracketsDoTag2;
        bool  m_bBracketsDoTagIf;
        bool  m_bBracketsSkipEscaped;
        TCHAR m_szHtmlFileExts[STR_FILEEXTS_SIZE];
        tstr  m_sFileExtsRule;

    protected:
        enum eOptConsts {
            OPTF_AUTOCOMPLETE   = 0x0001,
            OPTF_RIGHTBRACKETOK = 0x0008,
            OPTF_DOSINGLEQUOTE  = 0x0100,
            OPTF_DOTAG          = 0x0200,
            OPTF_DOTAGIF        = 0x0400,
            OPTF_DOTAG2         = 0x0800,
            OPTF_SKIPESCAPED    = 0x1000,
        };

        UINT  m_uFlags;
        UINT  m_uFlags0;
        bool  m_bSaveFileExtsRule;
        TCHAR m_szHtmlFileExts0[STR_FILEEXTS_SIZE];
};

//---------------------------------------------------------------------------
#endif
