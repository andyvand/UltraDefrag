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
 * @file statbar.cpp
 * @brief Status bar.
 * @addtogroup StatusBar
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define SB_PARTS 5

#define UD_SetStatusIcon(index,name) { \
    wxIcon *icon = new wxIcon(wxT(#name), wxBITMAP_TYPE_ICO_RESOURCE, g_iconSize, g_iconSize); \
    ::SendMessage((HWND)GetStatusBar()->GetHandle(),SB_SETICON,index,(LPARAM)icon->GetHICON()); \
}

#define UD_SetStatusText(index,text,counter) { \
    wxString t = text; \
    SetStatusText(wxString::Format(wxT("%lu %ls"),counter,t.wc_str()), index); \
}

// =======================================================================
//                              Status bar
// =======================================================================

void MainFrame::InitStatusBar()
{
    CreateStatusBar(SB_PARTS);
    SetStatusBarPane(-1);

    int w[SB_PARTS] = {-1,-1,-1,-1,-1};
    SetStatusWidths(SB_PARTS,w);

    UD_SetStatusIcon(0, directory);
    UD_SetStatusIcon(1, unfragmented);
    UD_SetStatusIcon(2, fragmented);
    UD_SetStatusIcon(3, compressed);
    UD_SetStatusIcon(4, mft);

    ProcessCommandEvent(ID_UpdateStatusBar);
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::UpdateStatusBar(wxCommandEvent& event)
{
    udefrag_progress_info *pi = \
        (udefrag_progress_info *) \
        event.GetClientData();

    if(!pi){
        pi = new udefrag_progress_info;
        memset(pi,0,sizeof(*pi));
    }

    UD_SetStatusText(0, _("folders"),    pi->directories);
    UD_SetStatusText(1, _("files"),      pi->files);
    UD_SetStatusText(2, _("fragmented"), pi->fragmented);
    UD_SetStatusText(3, _("compressed"), pi->compressed);

    char s[32]; winx_bytes_to_hr(pi->mft_size,2,s,sizeof(s));
    SetStatusText(wxString::Format(wxT("%hs MFT"),s), 4);

    delete pi;
}

#undef UD_SetStatusIcon
#undef UD_SetStatusText

/** @} */
