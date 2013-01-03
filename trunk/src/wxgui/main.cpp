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
 * @file main.cpp
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
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

    EVT_MENU(wxID_EXIT, MainFrame::OnExit)

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
    EVT_MENU(wxID_ABOUT, MainFrame::OnHelpAbout)
END_EVENT_TABLE()

IMPLEMENT_APP(App)

// =======================================================================
//                            Global variables
// =======================================================================

MainFrame *g_MainFrame = NULL;
double g_fScale = 1.0f;

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
        wxMessageBox(msg,wxT("UltraDefrag"),
            wxOK | wxICON_HAND,g_MainFrame);
        break;
    case wxLOG_ShowWarning:
        ::udefrag_dbg_print(0,DEBUG_FMT,msg);
        wxMessageBox(msg,wxT("UltraDefrag"),
            wxOK | wxICON_EXCLAMATION,g_MainFrame);
        break;
    case wxLOG_ShowMessage:
        ::udefrag_dbg_print(0,INFO_FMT,msg);
        wxMessageBox(msg,wxT("UltraDefrag"),
            wxOK | wxICON_INFORMATION,g_MainFrame);
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
    ::wxInitAllImageHandlers();
    if(!wxApp::OnInit())
        return false;

    // initialize udefrag.dll library
    if(::udefrag_init_library() < 0){
        ::wxLogError(wxT("Initialization failed!"));
        return false;
    }

    // initialize logging
    m_Log = new Log();
    
    // use global config object
    // for internal settings
    wxFileConfig *cfg = new wxFileConfig(wxT(""),wxT(""),
        wxT("gui.ini"),wxT(""),wxCONFIG_USE_RELATIVE_PATH);
    if(!cfg){
        ::wxLogShowError(wxT("Configuration failed!"));
        Cleanup();
        return false;
    }
    wxConfigBase::Set(cfg);
    
    // keep things DPI-aware
    HDC hdc = ::GetDC(NULL);
    if(hdc){
        g_fScale = (double)::GetDeviceCaps(hdc,LOGPIXELSX) / 96.0f;
        ::ReleaseDC(NULL,hdc);
    }

    // create main window
    g_MainFrame = new MainFrame();
    if(!g_MainFrame){
        Cleanup();
        return false;
    }
    g_MainFrame->Show(true);
    SetTopWindow(g_MainFrame);
    return true;
}

/**
 * @brief Frees application resources.
 */
void App::Cleanup()
{
    // deinitialize logging
    delete m_Log;

    // free udefrag.dll library
    ::udefrag_unload_library();
}

/**
 * @brief Deinitializes the application.
 */
int App::OnExit()
{
    Cleanup();
    return wxApp::OnExit();
}

// =======================================================================
//                             Main window
// =======================================================================

/**
 * @brief Initializes main window.
 */
MainFrame::MainFrame()
    :wxFrame(NULL,wxID_ANY,wxT("UltraDefrag"))
{
    // set main window icon
    wxIconBundle icon(wxT("res/app.ico"),wxBITMAP_TYPE_ICO);
    SetIcons(icon);
    
    // set main window title
    wxString *instdir = new wxString();
    if(wxGetEnv(wxT("UD_INSTALL_DIR"),instdir)){
        wxStandardPaths stdpaths;
        wxFileName exepath(stdpaths.GetExecutablePath());
        wxString cd = exepath.GetPath();
        wxLogMessage(wxT("Current directory: %ls"),cd.wc_str());
        wxLogMessage(wxT("Installation directory: %ls"),instdir->wc_str());
        if(cd.CmpNoCase(*instdir) == 0){
            wxLogMessage(wxT("Current directory matches ")
                wxT("installation location, so it isn't portable"));
            m_Title = new wxString(wxT(VERSIONINTITLE));
        } else {
            wxLogMessage(wxT("Current directory differs from ")
                wxT("installation location, so it is portable"));
            m_Title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
        }
    } else {
        m_Title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
    }
    SetTitle(*m_Title);
    delete instdir;

    // set main window size and position
    wxConfigBase *cfg = wxConfigBase::Get();
    bool saved = cfg->HasGroup(wxT("MainFrame"));
    int x = (int)cfg->Read(wxT("/MainFrame/x"),0l);
    int y = (int)cfg->Read(wxT("/MainFrame/y"),0l);
    int width = (int)cfg->Read(wxT("/MainFrame/width"),DPI(640));
    int height = (int)cfg->Read(wxT("/MainFrame/height"),DPI(480));
    SetSize(width,height);
    if(!saved){
        CenterOnScreen();
        GetPosition(&x,&y);
    }
    Move(x,y);
    if(cfg->Read(wxT("/MainFrame/maximized"),0l)){
        Maximize(true);
    }

    // create menu
    wxMenu *menuAction = new wxMenu;
    menuAction->Append(ID_Analyze,wxT("&Analyze\tF5"),wxT("Analyze selected drives"));
    menuAction->AppendSeparator();
    menuAction->Append(ID_Repeat,wxT("Re&peat action\tShift+R"),wxT("Repeat until finished"));
    menuAction->Append(wxID_EXIT);

    wxMenu *menuReport = new wxMenu;
    menuReport->Append(ID_ShowReport,wxT("&Show report\tF8"),wxT("Show fragmented files report"));

    wxMenu *menuSettings = new wxMenu;
    menuSettings->Append(ID_ReportOptions,wxT("&Reports\tCtrl+R"),wxT("Select report options"));

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuAction,   wxT("&Action")  );
    menuBar->Append(menuReport,   wxT("&Report")  );
    menuBar->Append(menuSettings, wxT("&Settings"));
    menuBar->Append(menuHelp,     wxT("&Help")    );

    SetMenuBar(menuBar);

    // create status bar
    CreateStatusBar();
    SetStatusText(wxT("Welcome to wxUltraDefrag!"));
}

/**
 * @brief Deinitializes main window.
 */
MainFrame::~MainFrame()
{
    // save main window size and position
    wxConfigBase *cfg = wxConfigBase::Get();
    int x,y,width,height;
    bool maximized = IsMaximized();
    Hide(); Maximize(false);
    GetPosition(&x,&y);
    GetSize(&width,&height);
    cfg->Write(wxT("/MainFrame/x"),(long)x);
    cfg->Write(wxT("/MainFrame/y"),(long)y);
    cfg->Write(wxT("/MainFrame/width"),(long)width);
    cfg->Write(wxT("/MainFrame/height"),(long)height);
    cfg->Write(wxT("/MainFrame/maximized"),(long)maximized);
    
    // free resources
    delete m_Title;
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
    Close(true);
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
    wxAboutDialogInfo info;

    info.SetVersion(wxT(wxUD_ABOUT_VERSION));
    info.SetCopyright(wxT("(C) 2007-2013 UltraDefrag development team"));
    info.SetDescription(wxT("Open source defragmentation utility.\n")
        wxT("Credits and Licenses are listed in the handbook."));

    wxAboutBox(info);
}

/** @} */
