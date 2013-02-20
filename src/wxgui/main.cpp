//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
//  Copyright (c) 2010-2013 Stefan Pendl (stefanpe@users.sourceforge.net).
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

#if !defined(__GNUC__)
#include <new.h> // for _set_new_handler
#endif

// =======================================================================
//                            Global variables
// =======================================================================

MainFrame *g_mainFrame = NULL;
double g_scaleFactor = 1.0f;
int g_iconSize; // small icon size

// =======================================================================
//                             Web statistics
// =======================================================================

void *StatThread::Entry()
{
    bool enabled = true; wxString s;
    if(wxGetEnv(wxT("UD_DISABLE_USAGE_TRACKING"),&s))
        if(s.Cmp(wxT("1")) == 0) enabled = false;

    if(enabled){
#ifndef _WIN64
        Utils::GaRequest(wxT("/appstat/gui-x86.html"));
#else
    #if defined(_IA64_)
        Utils::GaRequest(wxT("/appstat/gui-ia64.html"));
    #else
        Utils::GaRequest(wxT("/appstat/gui-x64.html"));
    #endif
#endif
    }

    return NULL;
}

// =======================================================================
//                    Application startup and shutdown
// =======================================================================

#if !defined(__GNUC__)
static int out_of_memory_handler(size_t n)
{
    int choice = MessageBox(
        g_mainFrame ? (HWND)g_mainFrame->GetHandle() : NULL,
        wxT("Try to release some memory by closing\n")
        wxT("other applications and click Retry then\n")
        wxT("or click Cancel to terminate the program."),
        wxT("UltraDefrag: out of memory!"),
        MB_RETRYCANCEL | MB_ICONHAND);
    if(choice == IDCANCEL){
        winx_flush_dbg_log(FLUSH_IN_OUT_OF_MEMORY);
        exit(3); return 0;
    }
    return 1;
}
#endif

/**
 * @brief Initializes the application.
 */
bool App::OnInit()
{
    // initialize wxWidgets
    SetAppName(wxT("UltraDefrag"));
    wxInitAllImageHandlers();
    if(!wxApp::OnInit())
        return false;

    // initialize zenwinx library
    if(::winx_init_library() < 0){
        wxLogError(wxT("Initialization failed!"));
        return false;
    }

    // set out of memory handler
#if !defined(__GNUC__)
    winx_set_killer(out_of_memory_handler);
    _set_new_handler(out_of_memory_handler);
    _set_new_mode(1);
#endif

    // initialize debug log
    wxFileName logpath(wxT(".\\logs\\ultradefrag.log"));
    logpath.Normalize();
    wxSetEnv(wxT("UD_LOG_FILE_PATH"),logpath.GetFullPath());
    ::udefrag_set_log_file_path();

    // initialize logging
    m_log = new Log();

    // start web statistics
    m_statThread = new StatThread();

    // check for administrative rights
    if(!Utils::CheckAdminRights()){
        wxMessageDialog dlg(NULL,
            wxT("Administrative rights are needed to run the program!"),
            wxT("UltraDefrag"),wxOK | wxICON_ERROR);
        dlg.ShowModal(); Cleanup();
        return false;
    }

    // use global config object for internal settings
    wxFileConfig *cfg = new wxFileConfig(wxT(""),wxT(""),
        wxT("gui.ini"),wxT(""),wxCONFIG_USE_RELATIVE_PATH);
    wxConfigBase::Set(cfg);

    // keep things DPI-aware
    HDC hdc = ::GetDC(NULL);
    if(hdc){
        g_scaleFactor = (double)::GetDeviceCaps(hdc,LOGPIXELSX) / 96.0f;
        ::ReleaseDC(NULL,hdc);
    }
    g_iconSize = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
    if(g_iconSize < 20) g_iconSize = 16;
    else if(g_iconSize < 24) g_iconSize = 20;
    else if(g_iconSize < 32) g_iconSize = 24;
    else g_iconSize = 32;

    // create main window
    g_mainFrame = new MainFrame();
    g_mainFrame->Show(true);
    SetTopWindow(g_mainFrame);
    return true;
}

/**
 * @brief Frees application resources.
 */
void App::Cleanup()
{
    // save internal settings
    delete wxConfigBase::Set(NULL);

    // stop web statistics
    delete m_statThread;

    // deinitialize logging
    ::winx_flush_dbg_log(0);
    delete m_log;
}

/**
 * @brief Deinitializes the application.
 */
int App::OnExit()
{
    Cleanup();
    return wxApp::OnExit();
}

IMPLEMENT_APP(App)

// =======================================================================
//                             Main window
// =======================================================================

/**
 * @brief Initializes main window.
 */
MainFrame::MainFrame()
    :wxFrame(NULL,wxID_ANY,wxT("UltraDefrag"))
{
    g_mainFrame = this;

    // set main window icon
    SetIcons(wxICON(appicon));

    // set main window title
    wxString *instdir = new wxString();
    if(wxGetEnv(wxT("UD_INSTALL_DIR"),instdir)){
        wxStandardPaths stdpaths;
        wxFileName exepath(stdpaths.GetExecutablePath());
        wxString cd = exepath.GetPath();
        itrace("current directory: %ls",cd.wc_str());
        itrace("installation directory: %ls",instdir->wc_str());
        if(cd.CmpNoCase(*instdir) == 0){
            itrace("current directory matches "
                "installation location, so it isn't portable");
            m_title = new wxString(wxT(VERSIONINTITLE));
        } else {
            itrace("current directory differs from "
                "installation location, so it is portable");
            m_title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
        }
    } else {
        m_title = new wxString(wxT(VERSIONINTITLE_PORTABLE));
    }
    SetTitle(*m_title);
    delete instdir;

    // set main window size and position
    wxConfigBase *cfg = wxConfigBase::Get();
    bool saved = cfg->HasGroup(wxT("MainFrame"));
    m_x = (int)cfg->Read(wxT("/MainFrame/x"),0l);
    m_y = (int)cfg->Read(wxT("/MainFrame/y"),0l);
    m_width = (int)cfg->Read(wxT("/MainFrame/width"),
        DPI(MAIN_WINDOW_DEFAULT_WIDTH));
    m_height = (int)cfg->Read(wxT("/MainFrame/height"),
        DPI(MAIN_WINDOW_DEFAULT_HEIGHT));
    // validate width and height
    wxDisplay display;
    if(m_width < DPI(MAIN_WINDOW_MIN_WIDTH)) m_width = DPI(MAIN_WINDOW_MIN_WIDTH);
    if(m_width > display.GetClientArea().width) m_width = DPI(MAIN_WINDOW_DEFAULT_WIDTH);
    if(m_height < DPI(MAIN_WINDOW_MIN_HEIGHT)) m_height = DPI(MAIN_WINDOW_MIN_HEIGHT);
    if(m_height > display.GetClientArea().height) m_height = DPI(MAIN_WINDOW_DEFAULT_HEIGHT);
    // validate x and y
    if(m_x < 0) m_x = 0; if(m_y < 0) m_y = 0;
    if(m_x > display.GetClientArea().width - 130)
        m_x = display.GetClientArea().width - 130;
    if(m_y > display.GetClientArea().height - 50)
        m_y = display.GetClientArea().height - 50;
    // now the window is surely inside of the screen
    SetSize(m_width,m_height);
    if(!saved){
        CenterOnScreen();
        GetPosition(&m_x,&m_y);
    }
    Move(m_x,m_y);
    if(cfg->Read(wxT("/MainFrame/maximized"),0l)){
        Maximize(true);
    }

    SetMinSize(wxSize(DPI(MAIN_WINDOW_MIN_WIDTH),DPI(MAIN_WINDOW_MIN_HEIGHT)));

    // i18n support
    InitLocale();

    // create menu
    InitMenu();

    // create tool bar
    InitToolbar();

    // create status bar
    CreateStatusBar();
    SetStatusText(wxT("Welcome to wxUltraDefrag!"));

    // create list of volumes and cluster map
    // don't use live update style to avoid horizontal scrollbar appearance on list resizing
    m_splitter = new wxSplitterWindow(this,wxID_ANY,
        wxDefaultPosition,wxDefaultSize,
        wxSP_3D/* | wxSP_LIVE_UPDATE*/ | wxCLIP_CHILDREN);
    m_splitter->SetMinimumPaneSize(DPI(MIN_PANEL_HEIGHT));

    m_vList = new wxListView(m_splitter,wxID_ANY,wxDefaultPosition,wxDefaultSize,
        wxLC_REPORT | wxLC_NO_SORT_HEADER | wxLC_HRULES | wxLC_VRULES | wxBORDER_NONE);
    LONG_PTR style = ::GetWindowLongPtr((HWND)m_vList->GetHandle(),GWL_STYLE);
    style |= LVS_SHOWSELALWAYS; ::SetWindowLongPtr((HWND)m_vList->GetHandle(),GWL_STYLE,style);

    m_cMap = new wxStaticText(m_splitter,wxID_ANY,wxT("The cluster map will be here"));

    m_splitter->SplitHorizontally(m_vList,m_cMap);

    int pos = (int)cfg->Read(wxT("/MainFrame/SeparatorPosition"),
        (long)DPI(DEFAULT_LIST_HEIGHT));
    int width, height; GetClientSize(&width,&height);
    int maxPanelHeight = height - DPI(MIN_PANEL_HEIGHT) - m_splitter->GetSashSize();
    if(pos < DPI(MIN_PANEL_HEIGHT)) pos = DPI(MIN_PANEL_HEIGHT);
    else if(pos > maxPanelHeight) pos = maxPanelHeight;
    m_splitter->SetSashPosition(pos);

    // update frame layout so we'll be
    // able to initialize list of volumes
    // and cluster map properly
    wxSizeEvent evt(wxSize(m_width,m_height));
    ProcessEvent(evt); m_splitter->UpdateSize();

    InitVolList(); InitMap();

    // populate list of volumes
    m_listThread = new ListThread();

    // check the boot time defragmenter presence
    wxFileName btdFile(wxT("%SystemRoot%\\system32\\defrag_native.exe"));
    btdFile.Normalize();
    bool btd = btdFile.FileExists();
    m_menuBar->FindItem(ID_BootEnable)->Enable(btd);
    m_menuBar->FindItem(ID_BootScript)->Enable(btd);
    m_toolBar->EnableTool(ID_BootEnable,btd);
    m_toolBar->EnableTool(ID_BootScript,btd);
    if(btd && ::winx_bootex_check(L"defrag_native") > 0){
        m_menuBar->FindItem(ID_BootEnable)->Check(true);
        m_toolBar->ToggleTool(ID_BootEnable,true);
        m_btdEnabled = true;
    } else {
        m_btdEnabled = false;
    }

    // launch threads for the time consuming operations
    m_crashInfoThread = new CrashInfoThread();

    int ulevel = (int)cfg->Read(wxT("/Upgrade/Level"),1);
    wxMenuItem *item = m_menuBar->FindItem(ID_HelpUpgradeNone + ulevel);
    if(item) item->Check();

    m_upgradeThread = new UpgradeThread(ulevel);

    m_btdThread = new BtdThread();
    m_btdThread->m_stop = btd ? false : true;
    m_btdThread->Run();

    // set localized text
    wxCommandEvent event;
    event.SetId(ID_LocaleChange+g_locale->GetLanguage());
    OnLocaleChange(event);
}

/**
 * @brief Deinitializes main window.
 */
MainFrame::~MainFrame()
{
    // save main window size and position
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Write(wxT("/MainFrame/x"),(long)m_x);
    cfg->Write(wxT("/MainFrame/y"),(long)m_y);
    cfg->Write(wxT("/MainFrame/width"),(long)m_width);
    cfg->Write(wxT("/MainFrame/height"),(long)m_height);
    cfg->Write(wxT("/MainFrame/maximized"),(long)IsMaximized());
    cfg->Write(wxT("/MainFrame/SeparatorPosition"),
        (long)m_splitter->GetSashPosition());

    cfg->Write(wxT("/DrivesList/width1"),m_w1);
    cfg->Write(wxT("/DrivesList/width2"),m_w2);
    cfg->Write(wxT("/DrivesList/width3"),m_w3);
    cfg->Write(wxT("/DrivesList/width4"),m_w4);
    cfg->Write(wxT("/DrivesList/width5"),m_w5);
    cfg->Write(wxT("/DrivesList/width6"),m_w6);

    cfg->Write(wxT("/Language/Selected"),(long)g_locale->GetLanguage());

    cfg->Write(wxT("/Algorithm/RepeatAction"),m_repeat);
    cfg->Write(wxT("/Algorithm/SkipRemovableMedia"),m_skipRem);

    cfg->Write(wxT("/Upgrade/Level"),(long)m_upgradeThread->m_level);

    // terminate threads
    delete m_listThread;
    delete m_crashInfoThread;
    delete m_upgradeThread;
    delete m_btdThread;

    // free resources
    delete m_title;
}

// =======================================================================
//                             Event table
// =======================================================================

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
    EVT_MENU_RANGE(ID_LangShowLog, ID_LangSubmit,
        MainFrame::OnLangOpenTransifex)
    EVT_MENU(ID_LangOpenFolder, MainFrame::OnLangOpenFolder)

    EVT_MENU_RANGE(ID_LocaleChange, ID_LocaleChange + \
        wxUD_LANGUAGE_LAST, MainFrame::OnLocaleChange)

    EVT_MENU(ID_GuiFont, MainFrame::OnGuiFont)
    EVT_MENU(ID_GuiOptions, MainFrame::OnGuiOptions)

    EVT_MENU(ID_BootEnable, MainFrame::OnBootEnable)
    EVT_MENU(ID_BootScript, MainFrame::OnBootScript)

    EVT_MENU(ID_ReportOptions, MainFrame::OnReportOptions)

    EVT_MENU_RANGE(ID_SortByPath,
        ID_SortByLastAccessDate, MainFrame::OnSortCriteriaChange)

    EVT_MENU_RANGE(ID_SortAscending,
        ID_SortDescending, MainFrame::OnSortOrderChange)

    // help menu
    EVT_MENU(ID_HelpContents, MainFrame::OnHelpContents)
    EVT_MENU(ID_HelpBestPractice, MainFrame::OnHelpBestPractice)
    EVT_MENU(ID_HelpFaq, MainFrame::OnHelpFaq)
    EVT_MENU(ID_HelpLegend, MainFrame::OnHelpLegend)

    EVT_MENU(ID_DebugLog, MainFrame::OnDebugLog)
    EVT_MENU(ID_DebugSend, MainFrame::OnDebugSend)

    EVT_MENU_RANGE(ID_HelpUpgradeNone,
        ID_HelpUpgradeCheck, MainFrame::OnHelpUpgrade)
    EVT_MENU(ID_HelpAbout, MainFrame::OnHelpAbout)

    // event handlers
    EVT_MOVE(MainFrame::OnMove)
    EVT_SIZE(MainFrame::OnSize)

    EVT_MENU(ID_BootChange, MainFrame::OnBootChange)
    EVT_MENU(ID_ShowUpgradeDialog, MainFrame::OnShowUpgradeDialog)
    EVT_MENU(ID_AdjustListColumns, MainFrame::AdjustListColumns)
    EVT_MENU(ID_AdjustListHeight, MainFrame::AdjustListHeight)
    EVT_MENU(ID_PopulateList, MainFrame::PopulateList)
END_EVENT_TABLE()

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnMove(wxMoveEvent& event)
{
    if(!IsMaximized() && !IsIconized()){
        GetPosition(&m_x,&m_y);
        GetSize(&m_width,&m_height);
    }
    event.Skip();
}

void MainFrame::OnSize(wxSizeEvent& event)
{
    if(!IsMaximized() && !IsIconized())
        GetSize(&m_width,&m_height);
    event.Skip();
}

// =======================================================================
//                            Menu handlers
// =======================================================================

// file menu handlers
void MainFrame::OnAnalyze(wxCommandEvent& WXUNUSED(event))
{
    Utils::ShowError(wxT("Cannot open the file!"));
}

void MainFrame::OnDefrag(wxCommandEvent& WXUNUSED(event))
{
    wxFileName file(Utils::DownloadFile(
        wxT("http://ultradefrag.sourceforge.net/version.ini")));
    file.Normalize();
    wxString path = file.GetFullPath();
    if(!wxLaunchDefaultBrowser(path))
        Utils::ShowError(wxT("Cannot open %ls!"),path.wc_str());
    //wxRemoveFile(path);
}

void MainFrame::OnQuickOpt(wxCommandEvent& WXUNUSED(event))
{
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_ShowUpgradeDialog);
    event.SetString(wxT(VERSIONINTITLE));
    wxPostEvent(this,event);
}

void MainFrame::OnFullOpt(wxCommandEvent& WXUNUSED(event))
{
    // test out of memory condition
    for(int i = 0; i < 1000000000; i++){
        //char *p = new char[1024];
        //char *p = (char *)malloc(1024);
        char *p = (char *)winx_malloc(1024);
        *p = 0x1;
    }
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
    m_repeat = m_repeat ? false : true;
    m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);
    m_toolBar->ToggleTool(ID_Repeat,m_repeat);
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
void MainFrame::OnGuiFont(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnGuiOptions(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnBootScript(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnReportOptions(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnSortCriteriaChange(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnSortOrderChange(wxCommandEvent& WXUNUSED(event))
{
}

// help menu handlers
void MainFrame::OnHelpContents(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("index.html"));
}

void MainFrame::OnHelpBestPractice(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("Tips.html"));
}

void MainFrame::OnHelpFaq(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("FAQ.html"));
}

void MainFrame::OnHelpLegend(wxCommandEvent& WXUNUSED(event))
{
    Utils::OpenHandbook(wxT("GUI.html"),wxT("cluster_map_legend"));
}

/** @} */
