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
 * @file btd.cpp
 * @brief Boot time defragmenter registration tracking.
 * @addtogroup BootTracking
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//              Boot time defragmenter registration tracking
// =======================================================================

BtdThread::BtdThread(wxThreadKind kind)
  : wxThread(kind)
{
}

void *BtdThread::Entry()
{
    wxLogMessage(wxT("boot registration tracking started"));

    HKEY hKey;
    if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,
      L"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
      0,KEY_NOTIFY,&hKey) != ERROR_SUCCESS){
        wxLogSysError(wxT("cannot open SMSS key"));
        return NULL;
    }

    HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if(hEvent == NULL){
        wxLogSysError(wxT("cannot create ")
            wxT("event for SMSS key tracking"));
        RegCloseKey(hKey);
        return NULL;
    }

    while(!m_stop){
        LONG error = RegNotifyChangeKeyValue(hKey,FALSE,
            REG_NOTIFY_CHANGE_LAST_SET,hEvent,TRUE);
        if(error != ERROR_SUCCESS){
            ::SetLastError(error);
            wxLogSysError(wxT("RegNotifyChangeKeyValue failed"));
            break;
        }
        while(!m_stop){
            if(WaitForSingleObject(hEvent,100) == WAIT_OBJECT_0){
                int result = ::udefrag_bootex_check(L"defrag_native");
                if(result >= 0){
                    wxLogMessage(wxT("boot time defragmenter %hs"),
                        result > 0 ? "enabled" : "disabled");
                    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_BootChange);
                    event.SetInt(result > 0 ? true : false);
                    wxPostEvent(g_MainFrame,event);
                }
                break;
            }
        }
    }

    wxLogMessage(wxT("boot registration tracking stopped"));
    CloseHandle(hEvent);
    RegCloseKey(hKey);
    return NULL;
}

/** @} */
