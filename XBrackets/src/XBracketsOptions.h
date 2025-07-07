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
        enum eOptConsts {
            // flags
            OPTF_AUTOCOMPLETE   = 0x0001,
            OPTF_RIGHTBRACKETOK = 0x0008,
            OPTF_DOSINGLEQUOTE  = 0x0100,
            OPTF_DOTAG          = 0x0200,
            OPTF_DOTAGIF        = 0x0400,
            OPTF_DOTAG2         = 0x0800,
            OPTF_SKIPESCAPED    = 0x1000,
        };

        inline bool getBoolFlag(UINT uWhich) const
        {
            return (m_uFlags & uWhich) != 0;
        }

        inline void setBoolFlag(UINT uWhich, bool isEnabled)
        {
            if ( isEnabled )
                m_uFlags |= uWhich;
            else if ( (m_uFlags & uWhich) != 0 )
                m_uFlags ^= uWhich;
        }

    public:
        enum eConsts {
            MAX_EXT             = 50,
            STR_FILEEXTS_SIZE   = 500
        };

        bool getBracketsAutoComplete() const
        {
            return getBoolFlag(OPTF_AUTOCOMPLETE);
        }

        void setBracketsAutoComplete(bool bBracketsAutoComplete)
        {
            setBoolFlag(OPTF_AUTOCOMPLETE, bBracketsAutoComplete);
        }

        bool getBracketsRightExistsOK() const
        {
            return getBoolFlag(OPTF_RIGHTBRACKETOK);
        }

        void setBracketsRightExistsOK(bool bBracketsRightExistsOK)
        {
            setBoolFlag(OPTF_RIGHTBRACKETOK, bBracketsRightExistsOK);
        }

        bool getBracketsDoSingleQuote() const
        {
            return getBoolFlag(OPTF_DOSINGLEQUOTE);
        }

        void setBracketsDoSingleQuote(bool bBracketsDoSingleQuote)
        {
            setBoolFlag(OPTF_DOSINGLEQUOTE, bBracketsDoSingleQuote);
        }

        bool getBracketsDoTag() const
        {
            return getBoolFlag(OPTF_DOTAG);
        }

        void setBracketsDoTag(bool bBracketsDoTag)
        {
            setBoolFlag(OPTF_DOTAG, bBracketsDoTag);
        }

        bool getBracketsDoTag2() const
        {
            return getBoolFlag(OPTF_DOTAG2);
        }

        void setBracketsDoTag2(bool bBracketsDoTag2)
        {
            setBoolFlag(OPTF_DOTAG2, bBracketsDoTag2);
        }

        bool getBracketsDoTagIf() const
        {
            return getBoolFlag(OPTF_DOTAGIF);
        }

        void setBracketsDoTagIf(bool bBracketsDoTagIf)
        {
            setBoolFlag(OPTF_DOTAGIF, bBracketsDoTagIf);
        }

        bool getBracketsSkipEscaped() const
        {
            return getBoolFlag(OPTF_SKIPESCAPED);
        }

        void setBracketsSkipEscaped(bool bBracketsSkipEscaped)
        {
            setBoolFlag(OPTF_SKIPESCAPED, bBracketsSkipEscaped);
        }

        const tstr& getHtmlFileExts() const
        {
            return m_sHtmlFileExts;
        }

        void setHtmlFileExts(const TCHAR* cszHtmlFileExts)
        {
            m_sHtmlFileExts = cszHtmlFileExts;
        }

    protected:
        UINT  m_uFlags;
        UINT  m_uFlags0;
        bool  m_bSaveFileExtsRule;
        tstr  m_sHtmlFileExts;
        tstr  m_sHtmlFileExts0;
        tstr  m_sFileExtsRule;
};

//---------------------------------------------------------------------------
#endif
