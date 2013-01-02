//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//////////////////////////////////////////////////////////////////////////

/**
 * @file main.c
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
// declarations
// =======================================================================

#include "main.h"

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    // file menu
    EVT_MENU(ID_Analyze, MainFrame::OnAnalyze)
    EVT_MENU(ID_Defrag, MainFrame::OnDefrag)
    EVT_MENU(ID_QuickOpt, MainFrame::OnQuickOpt)
    EVT_MENU(ID_FullOpt, MainFrame::OnFullOpt)
    EVT_MENU(ID_MftOpt, MainFrame::OnMftOpt)
    EVT_MENU(ID_Pause, MainFrame::OnPause)
    EVT_MENU(ID_Stop, MainFrame::OnStop)
    
    EVT_MENU(ID_Repeat, MainFrame::OnRepeat)
    
    EVT_MENU(ID_SkipRem, MainFrame::OnSkipRem)
    EVT_MENU(ID_Rescan, MainFrame::OnRescan)

    EVT_MENU(ID_Repair, MainFrame::OnRepair)
    
    EVT_MENU(ID_WhenDoneNone, MainFrame::OnWhenDoneNone)
    EVT_MENU(ID_WhenDoneExit, MainFrame::OnWhenDoneExit)
    EVT_MENU(ID_WhenDoneStandby, MainFrame::OnWhenDoneStandby)
    EVT_MENU(ID_WhenDoneHibernate, MainFrame::OnWhenDoneHibernate)
    EVT_MENU(ID_WhenDoneLogoff, MainFrame::OnWhenDoneLogoff)
    EVT_MENU(ID_WhenDoneReboot, MainFrame::OnWhenDoneReboot)
    EVT_MENU(ID_WhenDoneShutdown, MainFrame::OnWhenDoneShutdown)
    
    EVT_MENU(ID_Exit, MainFrame::OnExit)
    
    // report menu
    EVT_MENU(ID_ShowReport, MainFrame::OnShowReport)
    
    // settings menu
    EVT_MENU(ID_LangShowLog, MainFrame::OnLangShowLog)
    EVT_MENU(ID_LangShowReport, MainFrame::OnLangShowReport)
    EVT_MENU(ID_LangOpenFolder, MainFrame::OnLangOpenFolder)
    EVT_MENU(ID_LangSubmit, MainFrame::OnLangSubmit)
    
    EVT_MENU(ID_GuiFont, MainFrame::OnGuiFont)
    EVT_MENU(ID_GuiOptions, MainFrame::OnGuiOptions)
    
    EVT_MENU(ID_BootEnable, MainFrame::OnBootEnable)
    EVT_MENU(ID_BootScript, MainFrame::OnBootScript)
    
    EVT_MENU(ID_ReportOptions, MainFrame::OnReportOptions)
    
    // help menu
    EVT_MENU(ID_HelpContents, MainFrame::OnHelpContents)
    EVT_MENU(ID_HelpBestPractice, MainFrame::OnHelpBestPractice)
    EVT_MENU(ID_HelpFaq, MainFrame::OnHelpFaq)
    EVT_MENU(ID_HelpLegend, MainFrame::OnHelpLegend)
    
    EVT_MENU(ID_DebugLog, MainFrame::OnDebugLog)
    EVT_MENU(ID_DebugSend, MainFrame::OnDebugSend)
    
    EVT_MENU(ID_HelpUpdate, MainFrame::OnHelpUpdate)
    EVT_MENU(ID_HelpAbout, MainFrame::OnHelpAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(App)

// =======================================================================
//                             Global variables
// =======================================================================

MainFrame *g_MainFrame = NULL;

// =======================================================================
//                                Logging
// =======================================================================

/**
 * @brief Sets up custom logging.
 */
Log::Log()
{
    delete SetActiveTarget(this);
}

/**
 * @brief Restores default logging.
 */
Log::~Log()
{
    SetActiveTarget(NULL);
}

/**
 * @brief Shows a message on the screen.
 */
void Log::ShowMessage(const wxString& message,long style)
{
    wxMessageDialog dlg(g_MainFrame,
        message,wxT("UltraDefrag"),style);
    dlg.ShowModal();
}

/**
 * @brief Performs logging.
 */
void Log::DoLog(wxLogLevel level,const wxChar *msg,time_t timestamp)
{
    #define INFO_FMT  (I wxCharStringFmtSpec)
    #define DEBUG_FMT (D wxCharStringFmtSpec)
    #define ERROR_FMT (E wxCharStringFmtSpec)
    
    switch(level){
    // common levels used by wx and ourselves
    // will append a message to the log file
    case wxLOG_FatalError:
        // XXX: fatal errors pass by actually
        ::udefrag_dbg_print(0,ERROR_FMT,msg);
        ::udefrag_flush_dbg_log();
        break;
    case wxLOG_Error:
        ::udefrag_dbg_print(0,ERROR_FMT,msg);
        break;
    case wxLOG_Warning:
    case wxLOG_Info:
        ::udefrag_dbg_print(0,DEBUG_FMT,msg);
        break;
    case wxLOG_Message:
    case wxLOG_Status:
    case wxLOG_Progress:
        ::udefrag_dbg_print(0,INFO_FMT,msg);
        break;
    // custom levels used by ourselves
    // will do something special
    case wxLOG_ShowError:
        ::udefrag_dbg_print(0,ERROR_FMT,msg);
        ShowMessage(msg,wxOK | wxICON_HAND);
        break;
    case wxLOG_ShowWarning:
        ::udefrag_dbg_print(0,DEBUG_FMT,msg);
        ShowMessage(msg,wxOK | wxICON_EXCLAMATION);
        break;
    case wxLOG_ShowMessage:
        ::udefrag_dbg_print(0,INFO_FMT,msg);
        ShowMessage(msg,wxOK | wxICON_INFORMATION);
        break;
    default:
        ::udefrag_dbg_print(0,INFO_FMT,msg);
        break;
    }
}

// =======================================================================
//                    Application startup and shutdown
// =======================================================================

/**
 * @brief Initializes the application.
 */
bool App::OnInit()
{
    // initialize wxWidgets
    SetAppName(wxT("UltraDefrag"));
    if(!wxApp::OnInit())
        return false;

    // initialize udefrag.dll library
    if(::udefrag_init_library() < 0){
        ::wxLogError(wxT("Initialization failed!"));
        return false;
    }
    
    // initialize logging
    m_Log = new Log();
    
    ::wxLogMessage(wxT("Hello, everything's ok!"));
    ::wxLogError(wxT("What's the hell?"));
    //::wxLogFatalError(wxT("Welcome to hell!"));

    ::wxLogShowMessage(wxT("Mary had a little lamb."));
    ::wxLogShowError(wxT("COMPUTER MALFUNCTION!"));
    
    g_MainFrame = new MainFrame(wxT("UltraDefrag"),
        wxPoint(200,200),wxSize(450,300));
    if(!g_MainFrame)
        return false;
    g_MainFrame->Show(true);
    SetTopWindow(g_MainFrame);

    ::wxLogShowMessage(wxT("Mary had a little lamb."));
    ::wxLogShowError(wxT("COMPUTER MALFUNCTION!"));
    
    return true;
}

/**
 * @brief Deinitializes the application.
 */
int App::OnExit()
{
    // deinitialize logging
    delete m_Log;
    
    // free udefrag.dll library
    ::udefrag_unload_library();
    return wxApp::OnExit();
}

// =======================================================================
//                             Main window
// =======================================================================

/**
 * @brief Initializes main window.
 */
MainFrame::MainFrame(const wxString& title,
    const wxPoint& pos,const wxSize& size,long style)
       : wxFrame(NULL,wxID_ANY,title,pos,size,style)
{
}

// =======================================================================
//                            Menu handlers
// =======================================================================

// file menu handlers
void MainFrame::OnAnalyze(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnDefrag(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnQuickOpt(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnFullOpt(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnMftOpt(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnPause(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnStop(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnRepeat(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnSkipRem(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnRescan(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnRepair(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneNone(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneExit(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneStandby(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneHibernate(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneLogoff(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneReboot(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnWhenDoneShutdown(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
}

// report menu handlers
void MainFrame::OnShowReport(wxCommandEvent& WXUNUSED(event))
{
}

// settings menu handlers
void MainFrame::OnLangShowLog(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnLangShowReport(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnLangOpenFolder(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnLangSubmit(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnGuiFont(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnGuiOptions(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnBootEnable(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnBootScript(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnReportOptions(wxCommandEvent& WXUNUSED(event))
{
}

// help menu handlers
void MainFrame::OnHelpContents(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnHelpBestPractice(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnHelpFaq(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnHelpLegend(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnDebugLog(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnDebugSend(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnHelpUpdate(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event))
{
}

/** @} */
