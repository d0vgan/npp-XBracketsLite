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
        bpkSgLnBrackets, // single-line brackets pair
        bpkMlLnBrackets, // multi-line brackets pair
        bpkSgLnQuotes,   // sinle-line quotes pair
        bpkMlLnQuotes,   // multi-line quotes pair
        bpkSgLnComm,     // single-line comment
        bpkMlLnComm,     // multi-line comment pair
        bpkQtEsqChar     // escape character in quotes
    };

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
        Sci_Position nParentLeftBrPos{-1};
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
        tfmNone           = 0x0000,
        tfmComment1       = 0x0001,
        tfmHtmlCompatible = 0x0002,
        tfmEscaped1       = 0x0004,
        tfmSingleQuote    = 0x0008,
        tfmIsSupported    = 0x1000
    };

    // helpers
    bool isExistingFile(const TCHAR* filePath) noexcept;
    bool isExistingFile(const tstr& filePath) noexcept;
    std::vector<char> readFile(const TCHAR* filePath);
    tstr string_to_tstr(const std::string& str);
};

//---------------------------------------------------------------------------
#endif
