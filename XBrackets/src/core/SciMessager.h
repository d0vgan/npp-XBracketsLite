#ifndef _sci_messager_h_
#define _sci_messager_h_
//---------------------------------------------------------------------------
#include "base.h"
#include "npp_files/Scintilla.h"

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
        unsigned char getCharAt(Sci_Position pos) const;
        unsigned int  getCodePage() const; // 0 (non-Unicode), SC_CP_UTF8, DBCS etc.
        Sci_Position  getCurrentPos() const;
        LRESULT       getDocPointer() const; // identifies the document
        Sci_Position  getLine(Sci_Position line, char* pText) const; // returns the number of characters copied to the buffer
        Sci_Position  getLineCount() const; // number of lines in the document
        Sci_Position  getLineEndPos(Sci_Position line) const;
        Sci_Position  getLineFromPosition(Sci_Position pos) const;
        Sci_Position  getLineLength(Sci_Position line) const;
        Sci_Position  getPositionFromLine(Sci_Position line) const; // start of the line
        HWND          getSciWnd() const  { return m_hSciWnd; }
        int           getSelectionMode() const; // SC_SEL_STREAM, SC_SEL_RECTANGLE, SC_SEL_LINES
        Sci_Position  getSelectionEnd() const;
        Sci_Position  getSelectionStart() const;
        int           getSelections() const; // number of selections, there's always at least 1
        Sci_Position  getSelText(char* pText) const;
        int           getStyleIndexAt(Sci_Position pos) const;
        Sci_Position  getText(Sci_Position len, char* pText) const;
        Sci_Position  getTextLength() const;
        Sci_Position  getTextRange(Sci_Position pos1, Sci_Position pos2, char* pText) const;
        void          goToPos(Sci_Position pos);
        bool          isModified() const;
        bool          isSelectionRectangle() const;
        void          replaceSelText(const char* pText);
        void          setCodePage(unsigned int codePage);
        void          setSciWnd(HWND hSciWnd)  { m_hSciWnd = hSciWnd; }
        void          setSel(Sci_Position anchorPos, Sci_Position currentPos);
        void          setSelectionMode(int mode); // SC_SEL_STREAM, SC_SEL_RECTANGLE, SC_SEL_LINES
        void          setSelectionEnd(Sci_Position pos);
        void          setSelectionStart(Sci_Position pos);
        void          setText(const char* pText);
};

//---------------------------------------------------------------------------
#endif
