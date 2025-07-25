#ifndef _xbrackets_npp_options_h_
#define _xbrackets_npp_options_h_
//---------------------------------------------------------------------------
#include "core/base.h"
#include "XBracketsCommon.h"
#include <list>

class CXBracketsOptions
{
    public:
        using tstr = XBrackets::tstr;
        using tFileSyntax = XBrackets::tFileSyntax;

    public:
        CXBracketsOptions();
        ~CXBracketsOptions();

        bool MustBeSaved() const;
        bool IsHtmlCompatible(const TCHAR* szExt) const;
        bool IsEscapedFileExt(const TCHAR* szExt) const;
        bool IsSingleQuoteFileExt(const TCHAR* szExt) const;
        bool IsSupportedFile(const TCHAR* szExt) const;
        tstr ReadConfig(const tstr& cfgFilePath);
        void ReadOptions(const TCHAR* szIniFilePath);
        void SaveOptions(const TCHAR* szIniFilePath);

    protected:
        enum eOptConsts {
            // flags
            OPTF_AUTOCOMPLETE     = 0x000001,
            OPTF_RIGHTBRACKETOK   = 0x000008,
            OPTF_DOSINGLEQUOTE    = 0x000100,
            OPTF_DOTAG            = 0x000200,
            OPTF_DOTAGIF          = 0x000400,
            OPTF_DOTAG2           = 0x000800,
            OPTF_SKIPESCAPED      = 0x001000,
            OPTF_DOSINGLEQUOTEIF  = 0x002000,
            OPTF_DONOTDOUBLEQUOTE = 0x004000
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

        enum eSelAutoBr {
            sabNone = 0,
            sabEnclose,
            sabEncloseAndSel,
            sabEncloseRemove,
            sabEncloseRemoveOuter
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

        bool getBracketsDoDoubleQuote() const
        {
            return !getBoolFlag(OPTF_DONOTDOUBLEQUOTE);
        }

        void setBracketsDoDoubleQuote(bool bBracketsDoDoubleQuote)
        {
            setBoolFlag(OPTF_DONOTDOUBLEQUOTE, !bBracketsDoDoubleQuote);
        }

        bool getBracketsDoSingleQuote() const
        {
            return getBoolFlag(OPTF_DOSINGLEQUOTE);
        }

        void setBracketsDoSingleQuote(bool bBracketsDoSingleQuote)
        {
            setBoolFlag(OPTF_DOSINGLEQUOTE, bBracketsDoSingleQuote);
        }

        bool getBracketsDoSingleQuoteIf() const
        {
            return getBoolFlag(OPTF_DOSINGLEQUOTEIF);
        }

        void setBracketsDoSingleQuoteIf(bool bBracketsDoSingleQuoteIf)
        {
            setBoolFlag(OPTF_DOSINGLEQUOTEIF, bBracketsDoSingleQuoteIf);
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

        UINT getBracketsSelAutoBr() const // one of eSelAutoBr
        {
            return m_uSelAutoBr;
        }

        const tstr& getHtmlFileExts() const
        {
            return m_sHtmlFileExts;
        }

        void setHtmlFileExts(const TCHAR* cszHtmlFileExts)
        {
            m_sHtmlFileExts = cszHtmlFileExts;
        }

        const tstr& getEscapedFileExts() const
        {
            return m_sEscapedFileExts;
        }

        void setEscapedFileExts(const TCHAR* cszEscapedFileExts)
        {
            m_sEscapedFileExts = cszEscapedFileExts;
        }

        const tstr& getSglQuoteFileExts() const
        {
            return m_sSglQuoteFileExts;
        }

        void setSglQuoteFileExts(const TCHAR* cszSglQuoteFileExts)
        {
            m_sSglQuoteFileExts = cszSglQuoteFileExts;
        }

        const tstr& getNextCharOK() const
        {
            return m_sNextCharOK;
        }

        const tstr& getPrevCharOK() const
        {
            return m_sPrevCharOK;
        }

        const std::list<tFileSyntax>& getFileSyntaxes() const
        {
            return m_fileSyntaxes;
        }

        const tFileSyntax* getDefaultFileSyntax() const
        {
            return m_pDefaultFileSyntax;
        }

    protected:
        UINT  m_uFlags;
        UINT  m_uSelAutoBr; // one of eSelAutoBr
        bool  m_bSaveFileExtsRule;
        tstr  m_sHtmlFileExts;
        tstr  m_sHtmlFileExts0;
        tstr  m_sEscapedFileExts;
        tstr  m_sEscapedFileExts0;
        tstr  m_sSglQuoteFileExts;
        tstr  m_sSglQuoteFileExts0;
        tstr  m_sFileExtsRule;
        tstr  m_sNextCharOK;
        tstr  m_sPrevCharOK;

        std::list<tFileSyntax> m_fileSyntaxes;
        const tFileSyntax* m_pDefaultFileSyntax;
};

//---------------------------------------------------------------------------
#endif
