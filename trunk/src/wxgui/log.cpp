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
 * @file log.cpp
 * @brief Logging.
 * @addtogroup Logging
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                                Logging
// =======================================================================

void Log::DoLog(wxLogLevel level,const wxChar *msg,time_t timestamp)
{
    #define INFO_FMT  (I wxCharStringFmtSpec)
    #define DEBUG_FMT (D wxCharStringFmtSpec)
    #define ERROR_FMT (E wxCharStringFmtSpec)

    switch(level){
    case wxLOG_FatalError:
        // XXX: fatal errors pass by actually
        ::winx_dbg_print(0,ERROR_FMT,msg);
        ::winx_flush_dbg_log(0);
        break;
    case wxLOG_Error:
        ::winx_dbg_print(0,ERROR_FMT,msg);
        break;
    case wxLOG_Warning:
    case wxLOG_Info:
        ::winx_dbg_print(0,DEBUG_FMT,msg);
        break;
    default:
        ::winx_dbg_print(0,INFO_FMT,msg);
        break;
    }
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnDebugLog(wxCommandEvent& WXUNUSED(event))
{
    wxString logpath;
    if(!wxGetEnv(wxT("UD_LOG_FILE_PATH"),&logpath)){
        wxMessageBox(wxT("Logging to a file is turned off."),
            wxT("Cannot open log file!"),wxOK | wxICON_HAND,this);
    } else {
        wxFileName file(logpath);
        file.Normalize();
        ::winx_flush_dbg_log(0);
        logpath = file.GetFullPath();
        if(!wxLaunchDefaultBrowser(logpath))
            Utils::ShowError(wxT("Cannot open %ls!"),logpath.wc_str());
    }
}

void MainFrame::OnDebugSend(wxCommandEvent& WXUNUSED(event))
{
    wxString url(wxT("http://sourceforge.net/p/ultradefrag/bugs/"));
    if(!wxLaunchDefaultBrowser(url))
        Utils::ShowError(wxT("Cannot open %ls!"),url.wc_str());
}

/** @} */
