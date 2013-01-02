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

#ifndef _UDEFRAG_GUI_MAIN_H_
#define _UDEFRAG_GUI_MAIN_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#if wxUSE_UNICODE
#define wxCharStringFmtSpec "%ls"
#else
#define wxCharStringFmtSpec "%s"
#endif

#include "../dll/udefrag/udefrag.h"

enum {
    wxLOG_ShowError = wxLOG_User,
    wxLOG_ShowWarning,
    wxLOG_ShowMessage
};

#define wxLogShowError(format,...)   wxLogGeneric(wxLOG_ShowError,format,## __VA_ARGS__)
#define wxLogShowWarning(format,...) wxLogGeneric(wxLOG_ShowWarning,format,## __VA_ARGS__)
#define wxLogShowMessage(format,...) wxLogGeneric(wxLOG_ShowMessage,format,## __VA_ARGS__)

class Log: public wxLog {
public:
    Log();
    ~Log();
    virtual void DoLog(wxLogLevel level,
        const wxChar *msg,time_t timestamp);
private:
    void ShowMessage(const wxString& message,long style);
};

class App: public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();
private:
    Log *m_Log;
};

class MainFrame: public wxFrame {
public:
    MainFrame(const wxString& title,
        const wxPoint& pos,const wxSize &size,
        long style = wxDEFAULT_FRAME_STYLE);

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
    void OnLangShowLog(wxCommandEvent& event);
    void OnLangShowReport(wxCommandEvent& event);
    void OnLangOpenFolder(wxCommandEvent& event);
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
private:
    DECLARE_EVENT_TABLE()
};

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
    ID_HelpAbout
};

#endif /* _UDEFRAG_GUI_MAIN_H_ */
