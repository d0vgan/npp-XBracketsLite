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
        bpkfBr     = 0x0001,
        bpkfQt     = 0x0002,
        bpkfComm   = 0x0004,
        bpkfEsqCh  = 0x0008,
        bpkfMlLn   = 0x0100,
        bpkfNoInSp = 0x1000,
        bpkfLnSt   = 0x2000
    };

    enum eBrPairKind {
        bpkNone = 0,
        bpkSgLnBrackets             = (bpkfBr),                         // single-line brackets pair
        bpkSgLnBracketsNoInnerSpace = (bpkfBr | bpkfNoInSp),            // single-line brackets pair that can't contain a space
        bpkMlLnBrackets             = (bpkfBr | bpkfMlLn),              // multi-line brackets pair
        bpkMlLnBracketsLineStart    = (bpkfBr | bpkfMlLn | bpkfLnSt),   // multi-line brackets pair that starts at the beginning of the line
        bpkSgLnQuotes               = (bpkfQt),                         // single-line quotes pair
        bpkSgLnQuotesNoInnerSpace   = (bpkfQt | bpkfNoInSp),            // single-line quotes pair that can't contain a space
        bpkMlLnQuotes               = (bpkfQt | bpkfMlLn),              // multi-line quotes pair
        bpkMlLnQuotesLineStart      = (bpkfQt | bpkfMlLn | bpkfLnSt),   // multi-line quotes pair that starts at the beginning of the line
        bpkSgLnComm                 = (bpkfComm),                       // single-line comment
        bpkSgLnCommLineStart        = (bpkfComm | bpkfLnSt),            // single-line comment that starts at the beginning of the line
        bpkMlLnComm                 = (bpkfComm | bpkfMlLn),            // multi-line comment pair
        bpkMlLnCommLineStart        = (bpkfComm | bpkfMlLn | bpkfLnSt), // multi-line comment pair that starts at the beginning of the line
        bpkQtEsqChar                = (bpkfEsqCh)                       // escape character in quotes
    };

    static inline bool isBrKind(eBrPairKind kind)
    {
        return ((kind & bpkfBr) != 0);
    }

    static inline bool isQtKind(eBrPairKind kind)
    {
        return ((kind & bpkfQt) != 0);
    }

    static inline bool isSgLnBrKind(eBrPairKind kind)
    {
        return ((kind & (bpkfBr | bpkfMlLn)) == bpkfBr);
    }

    static inline bool isMlLnBrKind(eBrPairKind kind)
    {
        return ((kind & (bpkfBr | bpkfMlLn)) == (bpkfBr | bpkfMlLn));
    }

    static inline bool isSgLnQtKind(eBrPairKind kind)
    {
        return ((kind & (bpkfQt | bpkfMlLn)) == bpkfQt);
    }

    static inline bool isMlLnQtKind(eBrPairKind kind)
    {
        return ((kind & (bpkfQt | bpkfMlLn)) == (bpkfQt | bpkfMlLn));
    }

    static inline bool isSgLnBrQtKind(eBrPairKind kind)
    {
        return ((kind & (bpkfBr | bpkfQt)) != 0 && (kind & bpkfMlLn) == 0);
    }

    static inline bool isMlLnBrQtKind(eBrPairKind kind)
    {
        return ((kind & (bpkfBr | bpkfQt)) != 0 && (kind & bpkfMlLn) != 0);
    }

    static inline bool isSgLnCommKind(eBrPairKind kind)
    {
        return ((kind & (bpkfComm | bpkfMlLn)) == bpkfComm);
    }

    static inline bool isMlLnCommKind(eBrPairKind kind)
    {
        return ((kind & (bpkfComm | bpkfMlLn)) == (bpkfComm | bpkfMlLn));
    }

    static inline bool isNoInnerSpaceKind(eBrPairKind kind)
    {
        return ((kind & bpkfNoInSp) != 0);
    }

    static inline bool isLineStartKind(eBrPairKind kind)
    {
        return ((kind & bpkfLnSt) != 0);
    }

    enum eConsts {
        MAX_ESCAPED_PREFIX  = 20
    };

    struct tBrPair { // brackets, quotes or multi-line comments pair
        std::string leftBr;
        std::string rightBr;
        eBrPairKind kind{bpkNone};
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
