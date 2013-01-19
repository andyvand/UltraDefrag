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

#include "../include/version.h"
#include "../dll/udefrag/udefrag.h"

// =======================================================================
//                          Macro definitions
// =======================================================================

/* converts pixels from 96 DPI to the current one */
#define DPI(x) ((int)((double)x * g_fScale))

// =======================================================================
//                            Declarations
// =======================================================================

class Log: public wxLog {
public:
    Log();
    ~Log();

    virtual void DoLog(wxLogLevel level,
        const wxChar *msg,time_t timestamp);
};

class App: public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();

private:
    void Cleanup();
    Log *m_log;
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
    CrashInfoThread() : wxThread(wxTHREAD_JOINABLE) { m_stop = false; Create(); }
    ~CrashInfoThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
};

class UpgradeThread: public wxThread {
public:
    UpgradeThread() : wxThread(wxTHREAD_JOINABLE) { m_stop = false; Create(); }
    ~UpgradeThread() { m_stop = true; Wait(); }

    virtual void *Entry();

    bool m_stop;
    bool m_check;
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
    void OnLangSubmit(wxCommandEvent& event);

    void OnGuiFont(wxCommandEvent& event);
    void OnGuiOptions(wxCommandEvent& event);

    void OnBootEnable(wxCommandEvent& event);
    void OnBootScript(wxCommandEvent& event);

    void OnReportOptions(wxCommandEvent& event);

    // help menu handlers
    void OnHelpContents(wxCommandEvent& event);
    void OnHelpBestPractice(wxCommandEvent& event);
    void OnHelpFaq(wxCommandEvent& event);
    void OnHelpLegend(wxCommandEvent& event);

    void OnDebugLog(wxCommandEvent& event);
    void OnDebugSend(wxCommandEvent& event);

    void OnHelpUpdate(wxCommandEvent& event);
    void OnHelpAbout(wxCommandEvent& event);

    // event handlers
    void OnMove(wxMoveEvent& event);
    void OnSize(wxSizeEvent& event);

    void OnBootChange(wxCommandEvent& event);
    void OnLocaleChange(wxCommandEvent& event);

    void OnShowUpgradeDialog(wxCommandEvent& event);

private:
    void InitToolbar();
    void InitMenu();
    void InitLocale();
    void SetLocale(int id);
    void InitVolList();
    void InitMap();

    int m_x;
    int m_y;
    int m_width;
    int m_height;

    wxString   *m_title;
    wxToolBar  *m_toolBar;
    wxLocale   *m_locale;
    wxMenuBar  *m_menuBar;
    wxMenuItem *m_subMenuWhenDone;
    wxMenuItem *m_subMenuLanguage;
    wxMenuItem *m_subMenuGUIconfig;
    wxMenuItem *m_subMenuBootConfig;
    wxMenuItem *m_subMenuDebug;
    wxMenu     *m_menuLanguage;

    wxSplitterWindow *m_splitter;
    wxListView *m_vList;
    wxStaticText *m_cMap;

    bool m_repeat;
    bool m_skipRem;

    bool m_btdEnabled;
    BtdThread *m_btdThread;

    CrashInfoThread *m_crashInfoThread;
    UpgradeThread *m_upgradeThread;

    DECLARE_EVENT_TABLE()
};

class Utils {
public:
    static wxString DownloadFile(const wxString& url);
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
    ID_LangOpenFolder,
    ID_LangSubmit,

    ID_GuiFont,
    ID_GuiOptions,

    ID_BootEnable,
    ID_BootScript,

    ID_ReportOptions,

    // help menu identifiers
    ID_HelpContents,
    ID_HelpBestPractice,
    ID_HelpFaq,
    ID_HelpLegend,

    ID_DebugLog,
    ID_DebugSend,

    ID_HelpUpdate,
    ID_HelpAbout,

    // event identifiers
    ID_BootChange,
    ID_ShowUpgradeDialog,

    // language selection menu item, must always be last in the list
    ID_LocaleChange
};

#define MAIN_WINDOW_DEFAULT_WIDTH  640
#define MAIN_WINDOW_DEFAULT_HEIGHT 480

/* user defined language IDs
   Important: never change their order when adding new translations
   or the selection of the user will be broken */
enum {
    wxUD_LANGUAGE_BOSNIAN = wxLANGUAGE_USER_DEFINED+1,
    wxUD_LANGUAGE_ILOKO,
    wxUD_LANGUAGE_INDONESIAN_BAHASA,
    wxUD_LANGUAGE_KAPAMPANGAN,
    wxUD_LANGUAGE_WARAY_WARAY,
    wxUD_LANGUAGE_LAST          // must always be last in the list
};

// =======================================================================
//                           Global variables
// =======================================================================

extern MainFrame *g_MainFrame;
extern double g_fScale;

#endif /* _UDEFRAG_GUI_MAIN_H_ */
