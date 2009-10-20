/* file:Console.cpp
Creator : Girish Bharadwaj.
Desc:This   class   will  contain  all  the   stuff  we  need  from  the
Console.Following is the list I can think of 1. Create a new console and
use  it  to  Write  and read  console.  This  will be using AllocConsole
(),WriteConsole()  and ReadConsole (). This will  be via the win32 APIs.

2. Use the scanf ,printf,cout ,cin and cerr ( this will be using the CRT
functionality.)  Using the WDJ technique given  there by Andrew Tucker &
using  the KB "How to Spawn a Console App and Redirect Standard Handles"
Article ID: Q126628.

3.  Redirect  the  output  from a console  app  to  a windows GUI app. (
"Redirection  Issues on Windows 95 MS-DOS  Apps and Batch Files" Article
ID:  Q150956).This will be the fun part. What  I would like to see is to
provide  an  API which takes the  console  process name as the argument.
That  will  redirect the stuff to a  CString..  which can be read by the
creator.

4.  I am also planning to do  a somesort of poor man's debugger.i.e. You
will have an API similar to scanf which takes the input from the user at
run  time with a default value to  fall back to after sometime specified
by  u.  if you want to change  a particular variable and see the effect,
You can use this.
*/

#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <wincon.h>
#include "Console.hpp"

#include <iostream>
#include <fstream>

#ifndef _USE_OLD_IOSTREAMS
using namespace std;
#endif

BOOL CConsole::sm_bConsole = FALSE;

CConsole::CConsole()
    {
    //default constructor.
    m_bRedirected = FALSE; // this is the right place to set this before this
    m_sNumColumns = 0;
    m_sNumLines = 0;
    m_sMaxLines = 0;
    m_wAttrib = 0;
    }

CConsole::CConsole(BOOL bCreateConsole)
    {
    m_bRedirected = FALSE; // this is the right place to set this before this
    m_sNumColumns = 0;
    m_sNumLines = 0;
    m_sMaxLines = 0;
    m_wAttrib = 0;
    if (bCreateConsole)
        CreateConsole ();
    //I dont see any reason for not having bCreateConsole false But eh
    }


CConsole::~CConsole()
    {
    DestroyConsole (); //We need to remove the Console
    }

BOOL CConsole::CreateConsole ()
    {
    if (sm_bConsole == TRUE) // we have already created a console
        {
        return FALSE;
        }
    // I am not allocating the console if there is already one.

    if (!AllocConsole ()) //Just call the Console allocation API
        {
        sm_bConsole = FALSE;
        m_dwError = GetLastError (); //Lets get the error and store it away.
        return FALSE;
        }
    else
        {
        sm_bConsole = TRUE; //To make sure we wont allocate again
        //Lets keep all the stuff around
        m_sNumLines = (short)GetSettings (SC_LINES);
        m_sNumColumns = (short)GetSettings (SC_COLUMNS);
        m_sMaxLines  = (short) GetSettings (SC_MAXLINES);
        m_wAttrib = GetSettings (SC_ATTRIB);
        m_dwError = 0; // Lets keep this zero for the time being.
        return TRUE;
        }

    }

BOOL CConsole::DestroyConsole ()
    {
    if (sm_bConsole == FALSE) //There is no console to destroy
        return TRUE; //as Well return asif we deleted
    if (!FreeConsole ())
        {
        sm_bConsole = TRUE;
        m_dwError = GetLastError ();//Lets keep the error here if someone wants
        return FALSE;
        }
    else
        {
        sm_bConsole = FALSE;
        return TRUE;
        }
    }
short CConsole::GetNumberOfLines()
    {
    return (short) GetSettings (SC_LINES);
    }

short CConsole::SetNumberOfLines (short sLines)
    {
    short sRes = m_sNumLines;
    m_sNumLines = sLines;
    SetupConsole (SC_LINES);
    return sRes;
    }

short CConsole::SetNumberOfColumns (short sColumns)
    {
    short sOld = m_sNumColumns;
    m_sNumColumns = sColumns;
    SetupConsole (SC_COLUMNS);
    return sOld;
    }

short CConsole::GetNumberOfColumns ()
    {
    return (short)GetSettings (SC_COLUMNS);
    }

WORD CConsole::GetAttributes ()
    {
    return (short) GetSettings (SC_ATTRIB);
    }
WORD CConsole::SetAttributes (WORD wAttrib, short NumChars)
    {
    WORD wOld = m_wAttrib;
    m_wAttrib = wAttrib;
    SetupConsole (SC_ATTRIB);
    ApplyAttrib(NumChars);
    return wOld;
    }
short CConsole::SetMaxLinesInWindow (short maxLines)
    {
    short sOld = m_sMaxLines;
    m_sMaxLines = maxLines;
    SetupConsole (SC_MAXLINES);
    return sOld;
    }
short CConsole::GetMaxLinesInWindow ()
    {
    return (short) GetSettings (SC_MAXLINES);
    }

//Now here we have the basic beginning traits of a CConsole.
//But it has to do more than Allocing and Freeing consoles.
//So here it is..


void CConsole::RedirectToConsole (WORD wFlags)
    {
    int                        hConHandle;
    long                       lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE                       *fp;
    //First lets see if we already have allocated a console
    if (!sm_bConsole)
        CreateConsole();

    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo((HANDLE)lStdHandle,&coninfo);
    SetupConsole (SC_COLUMNS|SC_LINES|SC_ATTRIB|SC_MAXLINES);


    // redirect unbuffered STDOUT to the console

    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stdout = *fp;
    setvbuf( stdout, NULL, _IONBF, 0 );

    // redirect unbuffered STDIN to the console

    lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "r" );
    *stdin = *fp;
    setvbuf( stdin, NULL, _IONBF, 0 );

    // redirect unbuffered STDERR to the console

    lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
    hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
    fp = _fdopen( hConHandle, "w" );
    *stderr = *fp;
    setvbuf( stderr, NULL, _IONBF, 0 );
    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    ios::sync_with_stdio();
    m_bRedirected = TRUE; //Whether the Console is redirected
    }

/*
This will be function which will actually set up the console to the user
settings.
*/
BOOL CConsole::SetupConsole(WORD wFlags)
    {

    long                       lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    if (!sm_bConsole)
        return FALSE; // There aint no console to set up, Duh

    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo((HANDLE)lStdHandle,&coninfo);


    if (wFlags & SC_COLUMNS ||wFlags & SC_LINES) //Set up only the columns
        {
        //Number of Columns to be set
        if (m_sNumColumns)
            coninfo.dwSize.X = m_sNumColumns;
        // number of lines to be set
        if (m_sNumLines)
            coninfo.dwSize.Y = m_sNumLines;

        //Set the screen buffer size
        SetConsoleScreenBufferSize((HANDLE)lStdHandle,coninfo.dwSize);

        }
    if (wFlags & SC_ATTRIB)
        {
        //Attributes as specified
        if (m_wAttrib)
            coninfo.wAttributes = m_wAttrib;
        //Set the Text attributes
        SetConsoleTextAttribute ((HANDLE)lStdHandle,coninfo.wAttributes);
        }

    if (wFlags & SC_MAXLINES)
        {
        SMALL_RECT rect;

        //Maximum Size of the window
        if (m_sMaxLines)
            rect.Bottom= m_sMaxLines;
        else
            rect.Bottom = coninfo.dwMaximumWindowSize.Y;

        if (m_sNumColumns)
            rect.Right = m_sNumColumns;
        else
            rect.Right = coninfo.dwMaximumWindowSize.X;
        rect.Top = rect.Left = 0;

        //Set the console window maximum size
        SetConsoleWindowInfo ((HANDLE)lStdHandle,TRUE,
                &rect);
        }
    return TRUE;

    }

HANDLE CConsole::GetHandle (DWORD dwFlag)
    {
    if (!sm_bConsole)
        return (HANDLE) NULL;       
    return GetStdHandle (dwFlag);
    }

BOOL CConsole::Clear ()
    {
   COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                       cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize;                 /* number of character cells in
                                       the current buffer */

   if (!sm_bConsole)
       return FALSE;
   HANDLE hConsole = GetHandle (STD_OUTPUT_HANDLE);

   /* get the number of character cells in the current buffer */


   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   if (!bSuccess) return bSuccess;

   /* fill the entire screen with blanks */

   bSuccess = FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',
      dwConSize, coordScreen, &cCharsWritten );
   if (!bSuccess) return bSuccess;

   /* get the current text attribute */

   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );
   if (!bSuccess) return bSuccess;

   /* now set the buffer's attributes accordingly */

   bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
      dwConSize, coordScreen, &cCharsWritten );
   if (!bSuccess) return bSuccess;

   /* put the cursor at (0, 0) */

   bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
   if (!bSuccess) return bSuccess;
   return TRUE;
    }

BOOL CConsole::ApplyAttrib (short NumChars)
    {
   COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                       cursor */
   BOOL bSuccess;
   DWORD cCharsWritten;
   CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
   DWORD dwConSize;                 /* number of character cells in
                                       the current buffer */
   if (!sm_bConsole)
       return FALSE;
   HANDLE hConsole = GetHandle (STD_OUTPUT_HANDLE);
   bSuccess = GetConsoleScreenBufferInfo( hConsole, &csbi );

   if (!bSuccess) return bSuccess;

   dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
   bSuccess = FillConsoleOutputAttribute( hConsole, csbi.wAttributes,
       NumChars?NumChars:dwConSize, csbi.dwCursorPosition, &cCharsWritten );

    return  bSuccess;
    }

WORD CConsole::GetSettings (WORD wFlags)
    {
    long                       lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    if (!sm_bConsole)
        return FALSE; // There aint no console to set up, Duh

    lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo((HANDLE)lStdHandle,&coninfo);

    switch (wFlags)
        {
        case SC_ATTRIB:
            return coninfo.wAttributes;
            break;
        case SC_LINES:
            return coninfo.dwSize.Y;
            break;
        case SC_MAXLINES:
            return coninfo.dwMaximumWindowSize.Y;
            break;
        case SC_COLUMNS:
            return coninfo.dwSize.X;
            break;
        }
    return 0;
    }

