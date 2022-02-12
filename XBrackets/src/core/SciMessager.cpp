#include "SciMessager.h"

CSciMessager::CSciMessager(HWND hSciWnd )
{
    m_hSciWnd = hSciWnd;
}

CSciMessager::~CSciMessager()
{
}

LRESULT CSciMessager::SendSciMsg(UINT uMsg, WPARAM wParam , LPARAM lParam )
{
    return ::SendMessage(m_hSciWnd, uMsg, wParam, lParam);
}

LRESULT CSciMessager::SendSciMsg(UINT uMsg, WPARAM wParam , LPARAM lParam ) const
{
    return ::SendMessage(m_hSciWnd, uMsg, wParam, lParam);
}

void CSciMessager::beginUndoAction()
{
    SendSciMsg(SCI_BEGINUNDOACTION);
}

void CSciMessager::endUndoAction()
{
    SendSciMsg(SCI_ENDUNDOACTION);
}

unsigned char CSciMessager::getCharAt(INT_PTR pos) const
{
    return (unsigned char) SendSciMsg(SCI_GETCHARAT, (WPARAM) pos);
}

unsigned int CSciMessager::getCodePage() const
{
    return (unsigned int) SendSciMsg(SCI_GETCODEPAGE);
}

INT_PTR CSciMessager::getCurrentPos() const
{
    return (INT_PTR) SendSciMsg(SCI_GETCURRENTPOS);
}

LRESULT CSciMessager::getDocPointer() const
{
    return SendSciMsg(SCI_GETDOCPOINTER);
}

int CSciMessager::getSelectionMode() const
{
    return (int) SendSciMsg(SCI_GETSELECTIONMODE);
}

INT_PTR CSciMessager::getSelectionEnd() const
{
    return (INT_PTR) SendSciMsg(SCI_GETSELECTIONEND);
}

INT_PTR CSciMessager::getSelectionStart() const
{
    return (INT_PTR) SendSciMsg(SCI_GETSELECTIONSTART);
}

INT_PTR CSciMessager::getSelText(char* pText) const
{
    return (INT_PTR) SendSciMsg( SCI_GETSELTEXT, 0, (LPARAM) pText );
}

INT_PTR CSciMessager::getText(INT_PTR len, char* pText) const
{
    return (INT_PTR) SendSciMsg( SCI_GETTEXT, (WPARAM) len, (LPARAM) pText );
}

INT_PTR CSciMessager::getTextLength() const
{
    return (INT_PTR) SendSciMsg(SCI_GETTEXTLENGTH);
}

INT_PTR CSciMessager::getTextRange(INT_PTR pos1, INT_PTR pos2, char* pText) const
{
    TextRange tr;
    tr.chrg.cpMin = pos1; // I believe the TextRange _will_ use INT_PTR
    tr.chrg.cpMax = pos2; // or LONG_PTR to support 64-bit range
    tr.lpstrText = pText;
    return (INT_PTR) SendSciMsg( SCI_GETTEXTRANGE, 0, (LPARAM) &tr );
}

void CSciMessager::goToPos(INT_PTR pos)
{
    SendSciMsg( SCI_GOTOPOS, (WPARAM) pos );
}

bool CSciMessager::isModified() const
{
    return SendSciMsg(SCI_GETMODIFY) ? true : false;
}

bool CSciMessager::isSelectionRectangle() const
{
    return SendSciMsg(SCI_SELECTIONISRECTANGLE) ? true : false;
}

void CSciMessager::setCodePage(unsigned int codePage)
{
    SendSciMsg( SCI_SETCODEPAGE, (WPARAM) codePage );
}

void CSciMessager::setSel(INT_PTR anchorPos, INT_PTR currentPos)
{
    SendSciMsg( SCI_SETSEL, (WPARAM) anchorPos, (LPARAM) currentPos );
}

void CSciMessager::setSelectionMode(int mode)
{
    SendSciMsg( SCI_SETSELECTIONMODE, (WPARAM) mode );
}

void CSciMessager::setSelectionEnd(INT_PTR pos)
{
    SendSciMsg( SCI_SETSELECTIONEND, (WPARAM) pos );
}

void CSciMessager::setSelectionStart(INT_PTR pos)
{
    SendSciMsg( SCI_SETSELECTIONSTART, (WPARAM) pos );
}

void CSciMessager::setSelText(const char* pText)
{
    SendSciMsg( SCI_REPLACESEL, 0, (LPARAM) pText );
}

void CSciMessager::setText(const char* pText)
{
    SendSciMsg( SCI_SETTEXT, 0, (LPARAM) pText );
}
