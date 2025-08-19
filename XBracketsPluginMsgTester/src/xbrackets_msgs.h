/*
This file is part of XBrackets Lite
Copyright (C) 2025 DV <dvv81 (at) ukr (dot) net>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _xbrackets_plugin_msgs_h_
#define _xbrackets_plugin_msgs_h_
//---------------------------------------------------------------------------
#include <windows.h>


/*

Reference to "Notepad_plus_msgs.h":
 
  #define NPPM_MSGTOPLUGIN (NPPMSG + 47)
  //BOOL NPPM_MSGTOPLUGIN(TCHAR *destModuleName, CommunicationInfo *info)
  // return value is TRUE when the message arrive to the destination plugins.
  // if destModule or info is NULL, then return value is FALSE
  struct CommunicationInfo {
    long internalMsg;
    const TCHAR * srcModuleName;
    void * info; // defined by plugin
  };
 
The following messages can be used as internalMsg parameter.

 */


typedef struct sXBracketsPairStruct {
    const char*  pszLeftBr;   // [out] left bracket or quote, such as "(", "/*", etc.
    const char*  pszRightBr;  // [out] right bracket or quote, such as ")", "*/", etc.
    Sci_Position nLeftBrLen;  // [out] length of the left bracket = strlen(pszLeftBr)
    Sci_Position nRightBrLen; // [out] length of the right bracket = strlen(pszRightBr)
    Sci_Position nLeftBrPos;  // [out] position inside (after) the left bracket: (|
    Sci_Position nRightBrPos; // [out] position inside (before) the right bracket: |)
    Sci_Position nPos;        // [in] input position of a character
} tXBracketsPairStruct;


#define  XBRM_GETMATCHINGBRACKETS       0x0201  // message
  /*
  Searches for the matching brackets at the given nPos.
  Fills the output members of tXBracketsPairStruct.

  Example:

  const TCHAR* cszMyPlugin = _T("my_plugin");
  tXBracketsPairStruct brPair{};
  brPair.nPos = ::SendMessage(hSciWnd, SCI_GETCURRENTPOS, 0, 0);
  CommunicationInfo ci = { XBRM_GETMATCHINGBRACKETS, cszMyPlugin, (void *) &brPair };
  ::SendMessage( hNppWnd, NPPM_MSGTOPLUGIN, (WPARAM) _T("XBrackets.dll"), (LPARAM) &ci );

  Possible results:
  1) brPair.pszLeftBr is not nullptr - matching brackets found;
  2) brPair.pszLeftBr is nullptr - no matching brackets found.

  char str[100];
  if ( brPair.pszLeftBr != nullptr )
  {
      wsprintfA( str, "leftBr = %s\nrightBr = %s\nleftBrPos = %d\nrightBrPos = %d",
          brPair.pszLeftBr, brPair.pszRightBr, brPair.nLeftBrPos, brPair.nRightBrPos );
  }
  else
  {
      wsprintfA( str, "no matching brackets found at pos %d", brPair.nPos );
  }
  MessageBoxA(NULL, str, "XBrackets Result", MB_OK);

  */


#define  XBRM_GETNEARESTBRACKETS       0x0202  // message
/*
  Searches for the nearest (surrounding) brackets around the given nPos.
  Fills the output members of tXBracketsPairStruct.

  Example:

  const TCHAR* cszMyPlugin = _T("my_plugin");
  tXBracketsPairStruct brPair{};
  brPair.nPos = ::SendMessage(hSciWnd, SCI_GETCURRENTPOS, 0, 0);
  CommunicationInfo ci = { XBRM_GETNEARESTBRACKETS, cszMyPlugin, (void *) &brPair };
  ::SendMessage( hNppWnd, NPPM_MSGTOPLUGIN, (WPARAM) _T("XBrackets.dll"), (LPARAM) &ci );

  Possible results:
  1) brPair.pszLeftBr is not nullptr - nearest brackets found;
  2) brPair.pszLeftBr is nullptr - no nearest brackets found.

  char str[100];
  if ( brPair.pszLeftBr != nullptr )
  {
      wsprintfA( str, "leftBr = %s\nrightBr = %s\nleftBrPos = %d\nrightBrPos = %d",
          brPair.pszLeftBr, brPair.pszRightBr, brPair.nLeftBrPos, brPair.nRightBrPos );
  }
  else
  {
      wsprintfA( str, "no nearest brackets found at pos %d", brPair.nPos );
  }
  MessageBoxA(NULL, str, "XBrackets Result", MB_OK);

  */


//---------------------------------------------------------------------------
#endif
