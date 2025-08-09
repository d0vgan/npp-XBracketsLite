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
        bool IsSupportedFile(const TCHAR* szExt) const;
        tstr ReadConfig(const tstr& cfgFilePath);
        void ReadOptions(const TCHAR* szIniFilePath);
        void SaveOptions(const TCHAR* szIniFilePath);

    protected:
        enum eOptConsts {
            // flags
            OPTF_AUTOCOMPLETE     = 0x000001,
            OPTF_RIGHTBRACKETOK   = 0x000008
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

        void readConfigSettingsItem(const void* pContext);

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

        UINT getBracketsSelAutoBr() const // one of eSelAutoBr
        {
            return m_uSelAutoBr;
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

        const bool getUpdateTreeAllowed() const
        {
            // buildTree() takes less than a second for a 1 MB file even on a
            // 10-years old notebook.
            // updateTree() is called on each and every character typed or
            // deleted, potentially slowing down the entire reaction to typing.
            // As buildTree() is fast enough, we can just call invalidateTree()
            // instead of updateTree().
            return false;
        }

    protected:
        UINT  m_uFlags;
        UINT  m_uSelAutoBr; // one of eSelAutoBr
        tstr  m_sFileExtsRule;
        tstr  m_sNextCharOK;
        tstr  m_sPrevCharOK;

        std::list<tFileSyntax> m_fileSyntaxes;
        const tFileSyntax* m_pDefaultFileSyntax;
};

//---------------------------------------------------------------------------
#endif
