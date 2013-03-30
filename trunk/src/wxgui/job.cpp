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
 * @file job.cpp
 * @brief Volume processing.
 * @addtogroup VolumeProcessing
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define UD_EnableTool(id) { \
    wxMenuItem *item = m_menuBar->FindItem(id); \
    if(item) item->Enable(true); \
    if(m_toolBar->FindById(id)) \
        m_toolBar->EnableTool(id,true); \
}

#define UD_DisableTool(id) { \
    wxMenuItem *item = m_menuBar->FindItem(id); \
    if(item) item->Enable(false); \
    if(m_toolBar->FindById(id)) \
        m_toolBar->EnableTool(id,false); \
}

// =======================================================================
//                          Job startup thread
// =======================================================================

void *JobThread::Entry()
{
    while(!m_stop){
        if(m_launch){
            // do the job
            Sleep(5000);

            // complete the job
            wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_JobCompletion);
            wxPostEvent(g_mainFrame,event);
            m_launch = false;
        }
        Sleep(300);
    }

    return NULL;
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnStartJob(wxCommandEvent& event)
{
    if(m_busy) return;

    // if nothing is selected in the list return
    m_jobThread->m_counter = 0;
    long i = m_vList->GetFirstSelected();
    while(i != -1){
        m_jobThread->m_counter ++;
        i = m_vList->GetNextSelected(i);
    }
    if(!m_jobThread->m_counter) return;

    // lock everything till the job completion
    m_busy = true; m_paused = false;
    UD_DisableTool(ID_Analyze);
    UD_DisableTool(ID_Defrag);
    UD_DisableTool(ID_QuickOpt);
    UD_DisableTool(ID_FullOpt);
    UD_DisableTool(ID_MftOpt);
    UD_DisableTool(ID_Repeat);
    UD_DisableTool(ID_SkipRem);
    UD_DisableTool(ID_Rescan);
    UD_DisableTool(ID_Repair);
    UD_DisableTool(ID_ShowReport);
    m_subMenuSortingConfig->Enable(false);

    ReleasePause();

    SetSystemTrayIcon(wxT("tray_running"));

    // launch the job
    m_jobThread->m_launch = true;
}

void MainFrame::OnJobCompletion(wxCommandEvent& WXUNUSED(event))
{
    // unlock everything after the job completion
    UD_EnableTool(ID_Analyze);
    UD_EnableTool(ID_Defrag);
    UD_EnableTool(ID_QuickOpt);
    UD_EnableTool(ID_FullOpt);
    UD_EnableTool(ID_MftOpt);
    UD_EnableTool(ID_Repeat);
    UD_EnableTool(ID_SkipRem);
    UD_EnableTool(ID_Rescan);
    UD_EnableTool(ID_Repair);
    UD_EnableTool(ID_ShowReport);
    m_subMenuSortingConfig->Enable(true);
    m_busy = false;

    ReleasePause();

    SetSystemTrayIcon(wxT("tray"));

    // shutdown when requested
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_Shutdown);
    ProcessEvent(event);
}

void MainFrame::SetPause()
{
    m_menuBar->FindItem(ID_Pause)->Check(true);
    m_toolBar->ToggleTool(ID_Pause,true);

    Utils::SetProcessPriority(IDLE_PRIORITY_CLASS);

    SetSystemTrayIcon(m_busy ? wxT("tray_paused") : wxT("tray"));
}

void MainFrame::ReleasePause()
{
    m_menuBar->FindItem(ID_Pause)->Check(false);
    m_toolBar->ToggleTool(ID_Pause,false);

    Utils::SetProcessPriority(NORMAL_PRIORITY_CLASS);

    SetSystemTrayIcon(m_busy ? wxT("tray_running") : wxT("tray"));
}

void MainFrame::OnPause(wxCommandEvent& WXUNUSED(event))
{
    m_paused = m_paused ? false : true;
    if(m_paused) SetPause(); else ReleasePause();
}

void MainFrame::OnStop(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnRepeat(wxCommandEvent& WXUNUSED(event))
{
    if(!m_busy){
        m_repeat = m_repeat ? false : true;
        m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);
        m_toolBar->ToggleTool(ID_Repeat,m_repeat);
    }
}

void MainFrame::OnRepair(wxCommandEvent& WXUNUSED(event))
{
    if(m_busy) return;

    wxString args;
    long i = m_vList->GetFirstSelected();
    while(i != -1){
        char *label = _strdup(m_vList->GetItemText(i).char_str());
        args << wxString::Format(wxT(" %c:"),label[0]); free(label);
        i = m_vList->GetNextSelected(i);
    }

    if(args.IsEmpty()) return;

    /*
    create command line to check disk for corruption:
    CHKDSK {drive} /F ................. check the drive and correct problems
    PING -n {seconds + 1} localhost ... pause for the specified seconds
    */
    wxFileName path(wxT("%windir%\\system32\\cmd.exe"));
    path.Normalize(); wxString cmd(path.GetFullPath());
    cmd << wxT(" /C ( ");
    cmd << wxT("for %D in ( ") << args << wxT(" ) do ");
    cmd << wxT("@echo. ");
    cmd << wxT("& echo chkdsk %D ");
    cmd << wxT("& echo. ");
    cmd << wxT("& chkdsk %D /F ");
    cmd << wxT("& echo. ");
    cmd << wxT("& echo ------------------------------------------------- ");
    cmd << wxT("& ping -n 11 localhost >nul ");
    cmd << wxT(") ");
    cmd << wxT("& echo. ");
    cmd << wxT("& pause");

    itrace("Command Line: %ls", cmd.wc_str());
    if(!wxExecute(cmd)) Utils::ShowError(wxT("Cannot execute cmd.exe program!"));
}

#undef UD_EnableTool
#undef UD_DisableTool

/** @} */
