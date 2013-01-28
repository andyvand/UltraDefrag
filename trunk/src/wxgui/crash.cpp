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
 * @file crash.cpp
 * @brief Crash information handling.
 * @addtogroup CrashInfo
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define EVENT_BUFFER_SIZE (64 * 1024) /* 64 KB */

// =======================================================================
//                     Crash information handling
// =======================================================================

/**
 * @brief Collects information on application
 * crashes and displays it on the screen.
 */
void *CrashInfoThread::Entry()
{
    char *buffer = new char[EVENT_BUFFER_SIZE];
    DWORD bytes_to_read = EVENT_BUFFER_SIZE;
    DWORD bytes_read, bytes_needed;
    wxArrayString info;

    // get time stamp of the last processed event
    wxFileConfig *cfg = new wxFileConfig(wxT(""),wxT(""),
        wxT("crash-info.ini"),wxT(""),wxCONFIG_USE_RELATIVE_PATH);
    DWORD last_time_stamp = (DWORD)cfg->Read(wxT("/LastProcessedEvent/TimeStamp"),0l);
    DWORD new_time_stamp = last_time_stamp;

    // collect information on application crashes
    HANDLE hLog = ::OpenEventLog(NULL,wxT("Application"));
    if(!hLog){
        wxLogSysError(wxT("cannot open the Application event log"));
        goto done;
    }

    while(!m_stop){
        BOOL result = ::ReadEventLog(hLog,
            EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
            0,buffer,bytes_to_read,&bytes_read,&bytes_needed);
        if(!result){
            // handle errors
            switch(::GetLastError()){
            case ERROR_INSUFFICIENT_BUFFER:
                wxLogMessage(wxT("%u bytes of memory is needed ")
                    wxT("for the event log reading"),bytes_needed);
                bytes_to_read = bytes_needed;
                delete buffer; buffer = new char[bytes_to_read];
                break;
            case ERROR_HANDLE_EOF:
                goto save_info;
            default:
                wxLogSysError(wxT("cannot read the Application event log"));
                goto save_info;
            }
        } else {
            // handle events
            PEVENTLOGRECORD rec = (PEVENTLOGRECORD)buffer;
            while((char *)rec < buffer + bytes_read){
                if(rec->TimeGenerated <= last_time_stamp) break;
                // handle Application Error events only
                if(rec->EventType == EVENTLOG_ERROR_TYPE \
                  && (rec->EventID & 0xFFFF) == 1000 \
                  && rec->DataLength > 0)
                {
                    char *data = new char[rec->DataLength + 1];
                    memcpy(data,(char *)rec + rec->DataOffset,rec->DataLength);
                    data[rec->DataLength] = 0;
                    // handle UltraDefrag GUI and command line tool crashes only
                    if(strstr(data,"ultradefrag.exe") || strstr(data,"udefrag.exe")){
                        wxLogMessage(wxT("crashed in the past: %hs"),data);
                        wxString s; s.Printf(wxT("%hs"),data); s.Trim(); info.Add(s);
                        if(rec->TimeGenerated > new_time_stamp)
                            new_time_stamp = rec->TimeGenerated;
                    }
                    delete data;
                }
                rec = (PEVENTLOGRECORD)((char *)rec + rec->Length);
            }
        }
    }

    // save information to a file
save_info:
    if(!m_stop && !info.IsEmpty()){
        wxTextFile log;
        log.Create(wxT("crash-info.log"));
        log.AddLine(wxT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
        log.AddLine(wxT("                                                                                "));
        log.AddLine(wxT("         If this file contains the UltraDefrag crash information                "));
        log.AddLine(wxT("           you can help to fix the bug which caused the crash                   "));
        log.AddLine(wxT("           by submitting this information to the bug tracker:                   "));
        log.AddLine(wxT("        http://sourceforge.net/tracker/?group_id=199532&atid=969870             "));
        log.AddLine(wxT("                                                                                "));
        log.AddLine(wxT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
        log.AddLine(wxT("                                                                                "));
        for(int i = 0; i < (int)info.Count(); i++) log.AddLine(info[i]);
        log.Write();
        log.Close();
    }

    // open the file in default browser
    if(!m_stop && !info.IsEmpty()){
        wxFileName file(wxT("./crash-info.log"));
        file.Normalize();
        wxString logpath = file.GetFullPath();
        if(!wxLaunchDefaultBrowser(logpath))
            Utils::ShowError(wxT("Cannot open %ls!"),logpath.wc_str());
    }

    // save time stamp of the last processed event
    if(!m_stop) cfg->Write(wxT("/LastProcessedEvent/TimeStamp"),(long)new_time_stamp);

done:
    if(hLog) ::CloseEventLog(hLog);
    delete buffer; delete cfg;
    return NULL;
}

/** @} */
