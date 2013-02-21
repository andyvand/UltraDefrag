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

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnRepair(wxCommandEvent& WXUNUSED(event))
{
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

/** @} */
