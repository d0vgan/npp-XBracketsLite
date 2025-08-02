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

    enum eBrPairKind {
        bpkNone = 0,
        bpkSgLnBrackets,           // single-line brackets pair
        bpkMlLnBrackets,           // multi-line brackets pair
        bpkSgLnQuotes,             // single-line quotes pair
        bpkSgLnQuotesNoInnerSpace, // single-line quotes pair that can't contain a space
        bpkMlLnQuotes,             // multi-line quotes pair
        bpkSgLnComm,               // single-line comment
        bpkMlLnComm,               // multi-line comment pair
        bpkQtEsqChar               // escape character in quotes
    };

    static inline bool isBrKind(eBrPairKind kind)
    {
        return (kind == bpkSgLnBrackets || kind == bpkMlLnBrackets);
    }

    static inline bool isQtKind(eBrPairKind kind)
    {
        return (kind == bpkSgLnQuotes || kind == bpkSgLnQuotesNoInnerSpace || kind == bpkMlLnQuotes);
    }

    static inline bool isSgLnBrQtKind(eBrPairKind kind)
    {
        return (kind == bpkSgLnBrackets || kind == bpkSgLnQuotes || kind == bpkSgLnQuotesNoInnerSpace);
    }

    static inline bool isMlLnBrQtKind(eBrPairKind kind)
    {
        return (kind == bpkMlLnBrackets || kind == bpkMlLnQuotes);
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
    };

    struct tFileSyntax {
        std::string name;
        std::string parent;
        std::set<tstr> fileExtensions;
        std::vector<tBrPair> pairs;        // pairs (syntax)
        std::vector<tBrPair> autocomplete; // user-defined pairs for auto-completion
        std::vector<std::string> qtEsc;    // escape characters in quotes
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
