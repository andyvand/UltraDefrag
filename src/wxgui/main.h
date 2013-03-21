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

#ifndef _UDEFRAG_GUI_MAIN_H_
#define _UDEFRAG_GUI_MAIN_H_

// =======================================================================
//                               Headers
// =======================================================================

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/artprov.h>
#include <wx/confbase.h>
#include <wx/dir.h>
#include <wx/display.h>
#include <wx/dynlib.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/gbsizer.h>
#include <wx/hyperlink.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/mstream.h>
#include <wx/splitter.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/textfile.h>
#include <wx/thread.h>
#include <wx/toolbar.h>
#include <wx/uri.h>

#if wxUSE_UNICODE
#define wxCharStringFmtSpec "%ls"
#else
#define wxCharStringFmtSpec "%hs"
#endif

/*
* Next definition is very important for mingw:
* _WIN32_IE must be no less than 0x0400
* to include some important constant definitions.
*/
#ifndef _WIN32_IE
#define _WIN32_IE 0x0400
#endif
#include <commctrl.h>

#include "../include/version.h"
#include "../dll/zenwinx/zenwinx.h"
#include "../dll/udefrag/udefrag.h"

// =======================================================================
//                          Macro definitions
// =======================================================================

/* converts pixels from 96 DPI to the current one */
#define DPI(x) ((int)((double)x * g_scaleFactor))

// =======================================================================
//                            Declarations
// =======================================================================

class Log: public wxLog {
public:
    Log()  { delete SetActiveTarget(this); };
    ~Log() { SetActiveTarget(NULL); };

    virtual void DoLog(wxLogLevel level,
        const wxChar *msg,time_t timestamp);
};

class StatThread: public wxThread {
public:
    StatThread() : wxThread(wxTHREAD_JOINABLE) { Create(); Run(); }
    ~StatThread() { Wait(); }

    virtual void *Entry();
};

class App: public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();

private:
    void Cleanup();
    Log *m_log;
    StatThread *m_statThread;
};

class BtdThread: public wxThread {
public:
    BtdThread() : wxThread(wxTHREAD_JOINABLE) { m_stop = false; Create(); }
    ~BtdThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
};

class CrashInfoThread: public wxThread {
public:
    CrashInfoThread() : wxThread(wxTHREAD_JOINABLE) {
        m_stop = false; Create(); Run();
    }
    ~CrashInfoThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
};

class ConfigThread: public wxThread {
public:
    ConfigThread() : wxThread(wxTHREAD_JOINABLE) {
        m_stop = false; Create(); Run();
    }
    ~ConfigThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
};

class ListThread: public wxThread {
public:
    ListThread() : wxThread(wxTHREAD_JOINABLE) {
        m_stop = false; m_rescan = true; Create(); Run();
    }
    ~ListThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
    bool m_rescan;
};

class UpgradeThread: public wxThread {
public:
    UpgradeThread(int level) : wxThread(wxTHREAD_JOINABLE) {
        m_stop = false; m_level = level; m_check = true; Create(); Run();
    }
    ~UpgradeThread() { m_stop = true; Wait(); }

    virtual void *Entry();
    int ParseVersionString(const char *version);

    bool m_stop;
    bool m_check;
    int m_level;
};

class MainFrame: public wxFrame {
public:
    MainFrame();
    ~MainFrame();

    // file menu handlers
    void OnAnalyze(wxCommandEvent& event);
    void OnDefrag(wxCommandEvent& event);
    void OnQuickOpt(wxCommandEvent& event);
    void OnFullOpt(wxCommandEvent& event);
    void OnMftOpt(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);

    void OnRepeat(wxCommandEvent& event);

    void OnSkipRem(wxCommandEvent& event);
    void OnRescan(wxCommandEvent& event);

    void OnRepair(wxCommandEvent& event);

    void OnWhenDoneNone(wxCommandEvent& event);
    void OnWhenDoneExit(wxCommandEvent& event);
    void OnWhenDoneStandby(wxCommandEvent& event);
    void OnWhenDoneHibernate(wxCommandEvent& event);
    void OnWhenDoneLogoff(wxCommandEvent& event);
    void OnWhenDoneReboot(wxCommandEvent& event);
    void OnWhenDoneShutdown(wxCommandEvent& event);

    void OnExit(wxCommandEvent& event);

    // report menu handlers
    void OnShowReport(wxCommandEvent& event);

    // settings menu handlers
    void OnLangOpenTransifex(wxCommandEvent& event);
    void OnLangOpenFolder(wxCommandEvent& event);

    void OnGuiFont(wxCommandEvent& event);
    void OnGuiOptions(wxCommandEvent& event);

    void OnBootEnable(wxCommandEvent& event);
    void OnBootScript(wxCommandEvent& event);

    void OnReportOptions(wxCommandEvent& event);

    void OnSortCriteriaChange(wxCommandEvent& event);
    void OnSortOrderChange(wxCommandEvent& event);

    // help menu handlers
    void OnHelpContents(wxCommandEvent& event);
    void OnHelpBestPractice(wxCommandEvent& event);
    void OnHelpFaq(wxCommandEvent& event);
    void OnHelpLegend(wxCommandEvent& event);

    void OnDebugLog(wxCommandEvent& event);
    void OnDebugSend(wxCommandEvent& event);

    void OnHelpUpgrade(wxCommandEvent& event);
    void OnHelpAbout(wxCommandEvent& event);

    // event handlers
    void ReadUserPreferences(wxCommandEvent& event);
    void SetWindowTitle(wxCommandEvent& event);

    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnSplitChanged(wxSplitterEvent& event);

    void OnListSize(wxSizeEvent& event);
    void AdjustListColumns(wxCommandEvent& event);
    void AdjustListHeight(wxCommandEvent& event);
    void PopulateList(wxCommandEvent& event);
    void UpdateVolumeInformation(wxCommandEvent& event);

    void OnBootChange(wxCommandEvent& event);
    void OnLocaleChange(wxCommandEvent& event);

    void OnShowUpgradeDialog(wxCommandEvent& event);

    bool m_repeat;
    bool m_skipRem;

private:
    void ReadAppConfiguration();
    void SaveAppConfiguration();
    void ReadUserPreferences();
    bool CheckOption(const wxString& name);

    void InitLocale();
    void SetLocale(int id);
    bool GetLocaleFolder(wxString& CurrentLocaleDir);

    void InitMenu();
    void InitToolbar();

    void InitMap();
    void InitVolList();

    int  m_x;
    int  m_y;
    int  m_width;
    int  m_height;
    bool m_saved;
    bool m_maximized;
    int  m_separatorPosition;

    // precise list column widths
    double m_w1;
    double m_w2;
    double m_w3;
    double m_w4;
    double m_w5;
    double m_w6;

    // list height
    int m_vListHeight;

    wxString   *m_title;
    wxToolBar  *m_toolBar;
    wxMenuBar  *m_menuBar;
    wxMenuItem *m_subMenuWhenDone;
    wxMenuItem *m_subMenuLanguage;
    wxMenuItem *m_subMenuGUIconfig;
    wxMenuItem *m_subMenuBootConfig;
    wxMenuItem *m_subMenuSortingConfig;
    wxMenuItem *m_subMenuDebug;
    wxMenuItem *m_subMenuUpgrade;
    wxMenu     *m_menuLanguage;

    wxSplitterWindow *m_splitter;
    wxListView       *m_vList;
    wxStaticText     *m_cMap;

    bool m_btdEnabled;
    BtdThread *m_btdThread;

    ConfigThread    *m_configThread;
    CrashInfoThread *m_crashInfoThread;
    ListThread      *m_listThread;
    UpgradeThread   *m_upgradeThread;

    DECLARE_EVENT_TABLE()
};

class Utils {
public:
    static bool CheckAdminRights(void);
    static wxString DownloadFile(const wxString& url);
    static void GaRequest(const wxString& path);
    static wxBitmap *LoadPngResource(const wchar_t *name);
    static int MessageDialog(wxFrame *parent,
        const wxString& caption, const wxArtID& icon,
        const wxString& text1, const wxString& text2,
        const wxChar* format, ...);
    static void OpenHandbook(const wxString& page,
        const wxString& anchor = wxEmptyString);
    static void ShowError(const wxChar* format, ...);
};

// =======================================================================
//                              Constants
// =======================================================================

enum {
    // file menu identifiers
    ID_Analyze = 1,
    ID_Defrag,
    ID_QuickOpt,
    ID_FullOpt,
    ID_MftOpt,
    ID_Pause,
    ID_Stop,

    ID_Repeat,

    ID_SkipRem,
    ID_Rescan,

    ID_Repair,

    ID_WhenDoneNone,
    ID_WhenDoneExit,
    ID_WhenDoneStandby,
    ID_WhenDoneHibernate,
    ID_WhenDoneLogoff,
    ID_WhenDoneReboot,
    ID_WhenDoneShutdown,

    ID_Exit,

    // report menu identifiers
    ID_ShowReport,

    // settings menu identifiers
    ID_LangShowLog,
    ID_LangShowReport,
    ID_LangSubmit,
    // items above share the Transifex event handler
    ID_LangOpenFolder,

    ID_GuiFont,
    ID_GuiOptions,

    ID_BootEnable,
    ID_BootScript,

    ID_ReportOptions,

    // event registration must be modified to match range
    ID_SortByPath,
    ID_SortBySize,
    ID_SortByCreationDate,
    ID_SortByModificationDate,
    ID_SortByLastAccessDate,

    ID_SortAscending,
    ID_SortDescending,

    // help menu identifiers
    ID_HelpContents,
    ID_HelpBestPractice,
    ID_HelpFaq,
    ID_HelpLegend,

    ID_DebugLog,
    ID_DebugSend,

    ID_HelpUpgradeNone,
    ID_HelpUpgradeStable,
    ID_HelpUpgradeAll,
    ID_HelpUpgradeCheck,

    ID_HelpAbout,

    // event identifiers
    ID_BootChange,
    ID_ShowUpgradeDialog,
    ID_PopulateList,
    ID_UpdateVolumeInformation,
    ID_AdjustListColumns,
    ID_AdjustListHeight,
    ID_ReadUserPreferences,
    ID_SetWindowTitle,

    // language selection menu item, must always be last in the list
    ID_LocaleChange
};

#define MAIN_WINDOW_DEFAULT_WIDTH  640
#define MAIN_WINDOW_DEFAULT_HEIGHT 480
#define MAIN_WINDOW_MIN_WIDTH      500
#define MAIN_WINDOW_MIN_HEIGHT     375
#define DEFAULT_LIST_HEIGHT        130
#define MIN_PANEL_HEIGHT            40

#define DEFAULT_DRY_RUN          0
#define DEFAULT_FREE_COLOR_R   255
#define DEFAULT_FREE_COLOR_G   255
#define DEFAULT_FREE_COLOR_B   255
#define DEFAULT_GRID_COLOR_R     0
#define DEFAULT_GRID_COLOR_G     0
#define DEFAULT_GRID_COLOR_B     0
#define DEFAULT_GRID_LINE_WIDTH  1
#define DEFAULT_MAP_BLOCK_SIZE   4
#define DEFAULT_MINIMIZE_TO_SYSTEM_TRAY          0
#define DEFAULT_SECONDS_FOR_SHUTDOWN_REJECTION  60
#define DEFAULT_SHOW_MENU_ICONS                  1
#define DEFAULT_SHOW_PROGRESS_IN_TASKBAR         1
#define DEFAULT_SHOW_TASKBAR_ICON_OVERLAY        1

/* user defined language IDs
   Important: never change their order when adding new translations
   or the selection of the user will be broken */
enum {
    wxUD_LANGUAGE_BOSNIAN = wxLANGUAGE_USER_DEFINED+1,
    wxUD_LANGUAGE_ILOKO,
    wxUD_LANGUAGE_KAPAMPANGAN,
    wxUD_LANGUAGE_NORWEGIAN,
    wxUD_LANGUAGE_WARAY_WARAY,
    wxUD_LANGUAGE_LAST          // must always be last in the list
};

// =======================================================================
//                           Global variables
// =======================================================================

extern MainFrame *g_mainFrame;
extern wxLocale *g_locale;
extern double g_scaleFactor;
extern int g_iconSize;

#endif /* _UDEFRAG_GUI_MAIN_H_ */
