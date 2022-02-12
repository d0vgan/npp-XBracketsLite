#ifndef _sci_messager_h_
#define _sci_messager_h_
//---------------------------------------------------------------------------
#include "base.h"
#include "npp_stuff/Scintilla.h"

class CSciMessager
{
    protected:
        HWND m_hSciWnd;

    public:
        CSciMessager(HWND hSciWnd = NULL);
        virtual ~CSciMessager();

        LRESULT SendSciMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0);
        LRESULT SendSciMsg(UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0) const;
        
        void          beginUndoAction();
        void          endUndoAction();
        unsigned char getCharAt(INT_PTR pos) const;
        unsigned int  getCodePage() const; // 0 (non-Unicode), SC_CP_UTF8, DBCS etc.
        INT_PTR       getCurrentPos() const;
        LRESULT       getDocPointer() const; // identifies the document
        HWND          getSciWnd() const  { return m_hSciWnd; }
        int           getSelectionMode() const; // SC_SEL_STREAM, SC_SEL_RECTANGLE, SC_SEL_LINES
        INT_PTR       getSelectionEnd() const;
        INT_PTR       getSelectionStart() const;
        INT_PTR       getSelText(char* pText) const;
        INT_PTR       getText(INT_PTR len, char* pText) const;
        INT_PTR       getTextLength() const;
        INT_PTR       getTextRange(INT_PTR pos1, INT_PTR pos2, char* pText) const;
        void          goToPos(INT_PTR pos);
        bool          isModified() const;
        bool          isSelectionRectangle() const;
        void          setCodePage(unsigned int codePage);
        void          setSciWnd(HWND hSciWnd)  { m_hSciWnd = hSciWnd; }
        void          setSel(INT_PTR anchorPos, INT_PTR currentPos);
        void          setSelectionMode(int mode); // SC_SEL_STREAM, SC_SEL_RECTANGLE, SC_SEL_LINES
        void          setSelectionEnd(INT_PTR pos);
        void          setSelectionStart(INT_PTR pos);
        void          setSelText(const char* pText);
        void          setText(const char* pText);
};

//---------------------------------------------------------------------------
#endif
