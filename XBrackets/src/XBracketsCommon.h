#ifndef _xbracketscommon_npp_plugin_h_
#define _xbracketscommon_npp_plugin_h_
//---------------------------------------------------------------------------
#include "core/base.h"
#include "core/NppMessager.h"
#include "core/SciMessager.h"
#include <set>
#include <string>
#include <vector>

namespace XBrackets
{
    typedef std::basic_string<TCHAR> tstr;

    enum eBrPairKindFlags {
        bpkfBr      = 0x0001, // (kind) a pair of brackets
        bpkfQt      = 0x0002, // (kind) a pair of quotes
        bpkfComm    = 0x0004, // (kind) a comment
        bpkfEscCh   = 0x0008, // (kind) an escape character in quotes
        bpkfMlLn    = 0x0100, // (attr) multiline
        bpkfOpnLdSp = 0x0200, // (attr) leading spaces are allowed for bpkfOpnLnSt
        bpkfClsLdSp = 0x0400, // (attr) leading spaces are allowed for bpkfClsLnSt
        bpkfNoInSp  = 0x1000, // (attr) can't contain a space
        bpkfOpnLnSt = 0x2000, // (attr) the left (opening) bracket starts at the beginning of the line
        bpkfClsLnSt = 0x4000  // (attr) the right (closing) bracket starts at the beginning of the line
    };

    enum eBrPairAutoKindFlags {
        bpakfNoDup    = 0x0001, // ignore Prev_Char_OK when left bracket is the same as right bracket
        bpakfWtSp     = 0x0002, // white-space is needed before an opening bracket
        bpakfDelim    = 0x0004, // delimiter is needed before an opening bracket
        bpakfAddSpSel = 0x0080, // add a space character between the brackets and select this character
        bpakfAddTwoSp = 0x0100  // add two space characters between the brackets and place the caret between them
    };

    static inline bool isBrKind(unsigned int kind)
    {
        return ((kind & bpkfBr) != 0);
    }

    static inline bool isQtKind(unsigned int kind)
    {
        return ((kind & bpkfQt) != 0);
    }

    static inline bool isSgLnBrKind(unsigned int kind)
    {
        return ((kind & (bpkfBr | bpkfMlLn)) == bpkfBr);
    }

    static inline bool isMlLnBrKind(unsigned int kind)
    {
        return ((kind & (bpkfBr | bpkfMlLn)) == (bpkfBr | bpkfMlLn));
    }

    static inline bool isSgLnQtKind(unsigned int kind)
    {
        return ((kind & (bpkfQt | bpkfMlLn)) == bpkfQt);
    }

    static inline bool isMlLnQtKind(unsigned int kind)
    {
        return ((kind & (bpkfQt | bpkfMlLn)) == (bpkfQt | bpkfMlLn));
    }

    static inline bool isSgLnBrQtKind(unsigned int kind)
    {
        return ((kind & (bpkfBr | bpkfQt)) != 0 && (kind & bpkfMlLn) == 0);
    }

    static inline bool isMlLnBrQtKind(unsigned int kind)
    {
        return ((kind & (bpkfBr | bpkfQt)) != 0 && (kind & bpkfMlLn) != 0);
    }

    static inline bool isSgLnCommKind(unsigned int kind)
    {
        return ((kind & (bpkfComm | bpkfMlLn)) == bpkfComm);
    }

    static inline bool isMlLnCommKind(unsigned int kind)
    {
        return ((kind & (bpkfComm | bpkfMlLn)) == (bpkfComm | bpkfMlLn));
    }

    static inline bool isNoInSpKind(unsigned int kind)
    {
        return ((kind & bpkfNoInSp) != 0);
    }

    static inline bool isOpnLnStKind(unsigned int kind)
    {
        return ((kind & bpkfOpnLnSt) != 0);
    }

    static inline bool isOpnLdSpKind(unsigned int kind)
    {
        return ((kind & bpkfOpnLdSp) != 0);
    }

    static inline bool isClsLnStKind(unsigned int kind)
    {
        return ((kind & bpkfClsLnSt) != 0);
    }

    static inline bool isClsLdSpKind(unsigned int kind)
    {
        return ((kind & bpkfClsLdSp) != 0);
    }

    enum eConsts {
        MAX_ESCAPED_PREFIX  = 20
    };

    struct tBrPair { // brackets, quotes or multi-line comments pair
        std::string leftBr;
        std::string rightBr;
        unsigned int kind{0};
    };

    struct tBrPairItem
    {
        Sci_Position nLeftBrPos{-1};  // (| 
        Sci_Position nRightBrPos{-1}; // |)
        Sci_Position nLine{-1}; // only for internal comparison
        Sci_Position nParentIdx{-1};
        const tBrPair* pBrPair{};

        bool isComplete() const
        {
            return (nLeftBrPos != -1 && nRightBrPos != -1);
        }

        bool isIncomplete() const
        {
            return (nLeftBrPos == -1 || nRightBrPos == -1);
        }

        bool isOpenLeftBr() const
        {
            return (nLeftBrPos != -1 && nRightBrPos == -1);
        }

        bool isOpenRightBr() const
        {
            return (nLeftBrPos == -1 && nRightBrPos != -1);
        }
    };

    enum eFileSyntaxFlags {
        fsfNullFileExt = 0x0001  // do not process file extensions
    };

    struct tFileSyntax {
        std::string name;
        std::string parent;
        std::set<tstr> fileExtensions;
        std::vector<tBrPair> pairs;        // pairs (syntax)
        std::vector<tBrPair> autocomplete; // user-defined pairs for auto-completion
        std::vector<std::string> qtEsc;    // escape characters in quotes
        unsigned int uFlags{0};            // see eFileSyntaxFlags
    };

    enum TFileType2 {
        tfmIsSupported    = 0x1000
    };

    enum eStringCase {
        scUpper = 1,
        scLower = 2
    };

    // helpers
    bool isExistingFile(const TCHAR* filePath) noexcept;
    bool isExistingFile(const tstr& filePath) noexcept;
    std::vector<char> readFile(const TCHAR* filePath);
    tstr string_to_tstr(const std::string& str);
    tstr string_to_tstr_changecase(const std::string& str, eStringCase strCase);
};

//---------------------------------------------------------------------------
#endif
