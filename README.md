# npp-XBracketsLite

XBrackets Lite for Notepad++ allows to autocomplete brackets `([{""}])` i.e. it inserts the corresponding right bracket when a left bracket is typed.  
Starting from the version 2.0, user-defined multi-character brackets and quotes are supported (such as `/* */`, `""" """` and so on).  
The plugin uses "smart" brackets autocompletion:
- next character is analysed for `([{` brackets;
- next & previous characters are analysed for `"` and `'` quotes.

XBrackets Lite, starting from the version 2.0, adds the following functions:
* Go To Matching Bracket: when the caret is at a bracket or quote character, jumps to the pair bracket or quote, e.g. `|( )` becomes `( )|`.
* Sel To Matching Brackets: when the caret is at a bracket or quote character, selects the pair of the brackets or quotes, e.g. `|( )` becomes `|( )|`.
* Go To Nearest Bracket: when the caret is between bracket or quote characters, jumps to the nearest surrounding bracket or quote, e.g. `"ab|cd"` becomes `"|abcd"`.
* Sel To Nearest Brackets: when the caret is between bracket or quote characters, selects the text within the surrounding brackets or quotes, e.g. `"ab|cd"` becomes `"|abcd|"`.

The plugin's configuration is defined by the file "XBrackets_Config.json" that must be placed near the "XBrackets.dll".  
This configuration file contains both the plugin's settings and the file type-specific (language-specific) syntaxes and rules.  
For example, the `Sel_AutoBr` property allows to enclose (or disclose) the selected text with brackets.  
See "XBrackets.txt" for details.  

Here is the user-configuration file "XBrackets_UserConfig.json", as of version 2.0:

<img width="698" height="536" alt="XBrackets_Config" src="https://github.com/user-attachments/assets/8cc6d122-6d2b-4b7e-8c19-91bc935fbd89" />
