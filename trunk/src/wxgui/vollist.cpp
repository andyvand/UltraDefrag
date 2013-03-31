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
 * @file vollist.cpp
 * @brief List of volumes.
 * @addtogroup VolList
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

int g_fixedIcon;
int g_fixedDirtyIcon;
int g_removableIcon;
int g_removableDirtyIcon;

// =======================================================================
//                           List of volumes
// =======================================================================

void MainFrame::InitVolList()
{
    // save default font used for the list
    m_vListFont = new wxFont(m_vList->GetFont());

    // set monospace font for the list unless Burmese translation is selected
    if(g_locale->GetCanonicalName().Left(2) != wxT("my")){
        wxFont font = m_vList->GetFont();
        if(font.SetFaceName(wxT("Courier New"))){
            font.SetPointSize(DPI(9));
            m_vList->SetFont(font);
        }
    }

    // adjust widths so all the columns will fit to the window
    int width = m_vList->GetClientSize().GetWidth();
    double scale = (double)width / (m_w1 + m_w2 + m_w3 + m_w4 + m_w5 + m_w6);
    m_w1 *= scale; m_w2 *= scale; m_w3 *= scale; m_w4 *= scale; m_w5 *= scale;
    m_w6 *= scale;

    int w1 = (int)floor(m_w1); int w2 = (int)floor(m_w2);
    int w3 = (int)floor(m_w3); int w4 = (int)floor(m_w4);
    int w5 = (int)floor(m_w5); int w6 = width - w1 - w2 - w3 - w4 - w5;

    m_vList->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT,  w1);
    m_vList->InsertColumn(1, wxEmptyString, wxLIST_FORMAT_LEFT,  w2);
    m_vList->InsertColumn(2, wxEmptyString, wxLIST_FORMAT_RIGHT, w3);
    m_vList->InsertColumn(3, wxEmptyString, wxLIST_FORMAT_RIGHT, w4);
    m_vList->InsertColumn(4, wxEmptyString, wxLIST_FORMAT_RIGHT, w5);
    m_vList->InsertColumn(5, wxEmptyString, wxLIST_FORMAT_RIGHT, w6);

    // attach drive icons
    int size = g_iconSize;
    wxImageList *list = new wxImageList(size,size);
    g_fixedIcon          = list->Add(wxIcon(wxT("fixed")           , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    g_fixedDirtyIcon     = list->Add(wxIcon(wxT("fixed_dirty")     , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    g_removableIcon      = list->Add(wxIcon(wxT("removable")       , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    g_removableDirtyIcon = list->Add(wxIcon(wxT("removable_dirty") , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    m_vList->SetImageList(list,wxIMAGE_LIST_SMALL);

    // ensure that the list will cover integral number of items
    m_vListHeight = 0xFFFFFFFF; // prevent expansion of the list
    m_vList->InsertItem(0,wxT("hi"),0);
    ProcessCommandEvent(ID_AdjustListHeight);

    Connect(wxEVT_SIZE,wxSizeEventHandler(MainFrame::OnListSize),NULL,this);
    m_splitter->Connect(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
        wxSplitterEventHandler(MainFrame::OnSplitChanged),NULL,this);
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::AdjustListColumns(wxCommandEvent& event)
{
    int width = event.GetInt();
    if(width < 0) width = m_vList->GetClientSize().GetWidth();

    double scale = (double)width / (m_w1 + m_w2 + m_w3 + m_w4 + m_w5 + m_w6);
    m_w1 *= scale; m_w2 *= scale; m_w3 *= scale; m_w4 *= scale; m_w5 *= scale;
    m_w6 *= scale;

    int w1 = (int)floor(m_w1); int w2 = (int)floor(m_w2);
    int w3 = (int)floor(m_w3); int w4 = (int)floor(m_w4);
    int w5 = (int)floor(m_w5); int w6 = width - w1 - w2 - w3 - w4 - w5;

    m_vList->SetColumnWidth(0,w1);
    m_vList->SetColumnWidth(1,w2);
    m_vList->SetColumnWidth(2,w3);
    m_vList->SetColumnWidth(3,w4);
    m_vList->SetColumnWidth(4,w5);
    m_vList->SetColumnWidth(5,w6);
}

void MainFrame::AdjustListHeight(wxCommandEvent& WXUNUSED(event))
{
    // get client height of the list
    int height = m_splitter->GetSashPosition();
    height -= 2 * wxSystemSettings::GetMetric(wxSYS_BORDER_Y);

    // avoid recursion
    if(height == m_vListHeight) return;
    bool expand = (height > m_vListHeight) ? true : false;
    m_vListHeight = height;

    if(!m_vList->GetColumnCount()) return;

    // get height of the list header
    HWND header = ListView_GetHeader((HWND)m_vList->GetHandle());
    if(!header){ letrace("cannot get list header"); return; }

    RECT rc;
    if(!Header_GetItemRect(header,0,&rc)){
        letrace("cannot get list header size"); return;
    }
    int header_height = rc.bottom - rc.top;

    // get height of a single row
    wxRect rect; if(!m_vList->GetItemRect(0,rect)) return;
    int item_height = rect.GetHeight();

    // force list to cover integral number of items
    int items = (height - header_height) / item_height;
    int new_height = header_height + items * item_height;
    if(expand && new_height < height){
        items ++; new_height += item_height;
    }

    m_vListHeight = new_height;

    // adjust client height of the list
    new_height += 2 * wxSystemSettings::GetMetric(wxSYS_BORDER_Y);
    m_splitter->SetSashPosition(new_height);
}

void MainFrame::OnSplitChanged(wxSplitterEvent& event)
{
    // shrink the last list column so the vertical
    // scrollbar will not pull horizontal one out
    if(m_vList->GetCountPerPage() >= m_vList->GetItemCount())
        m_vList->SetColumnWidth(m_vList->GetColumnCount() - 1,0);

    // ensure that the list control will cover integral number of items
    PostCommandEvent(this,ID_AdjustListHeight);

    // adjust list columns once again to reflect the actual layout
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(-1); wxPostEvent(this,evt);

    event.Skip();
}

void MainFrame::OnListSize(wxSizeEvent& event)
{
    int old_width = m_vList->GetClientSize().GetWidth();
    int new_width = this->GetClientSize().GetWidth();
    new_width -= 2 * wxSystemSettings::GetMetric(wxSYS_EDGE_X);
    if(m_vList->GetCountPerPage() < m_vList->GetItemCount())
        new_width -= wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);

    // scale list columns; avoid horizontal scrollbar appearance
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(new_width);
    if(new_width <= old_width){
        ProcessEvent(evt);
    } else {
        wxPostEvent(this,evt);
    }

    event.Skip();
}

// =======================================================================
//                            Drives scanner
// =======================================================================

void *ListThread::Entry()
{
    while(!m_stop){
        if(m_rescan){
            volume_info *v = ::udefrag_get_vollist(g_mainFrame->m_skipRem);
            if(v){
                wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_PopulateList);
                event.SetClientData((void *)v);
                wxPostEvent(g_mainFrame,event);
            }
            m_rescan = false;
        }
        Sleep(300);
    }

    return NULL;
}

void MainFrame::UpdateVolumeInformation(wxCommandEvent& event)
{
    int index = event.GetInt();
    volume_info *v = (volume_info *)event.GetClientData();

    if(v->is_dirty){
        if(v->is_removable) m_vList->SetItemImage(index,g_removableDirtyIcon);
        else m_vList->SetItemImage(index,g_fixedDirtyIcon);
        m_vList->SetItem(index,1,_("Disk needs to be repaired"));
    } else {
        if(v->is_removable) m_vList->SetItemImage(index,g_removableIcon);
        else m_vList->SetItemImage(index,g_fixedIcon);
    }

    char s[32]; wxString string;
    ::winx_bytes_to_hr((ULONGLONG)(v->total_space.QuadPart),2,s,sizeof(s));
    string.Printf(wxT("%hs"),s); m_vList->SetItem(index,3,string);

    ::winx_bytes_to_hr((ULONGLONG)(v->free_space.QuadPart),2,s,sizeof(s));
    string.Printf(wxT("%hs"),s); m_vList->SetItem(index,4,string);

    double total = (double)v->total_space.QuadPart;
    double free = (double)v->free_space.QuadPart;
    double d = (total > 0) ? free / total : 0;
    int p = (int)(100 * d);
    string.Printf(wxT("%u %%"),p); m_vList->SetItem(index,5,string);

    delete v;
}

void MainFrame::PopulateList(wxCommandEvent& event)
{
    volume_info *v = (volume_info *)event.GetClientData();

    m_vList->DeleteAllItems();

    // shrink the last list column so the vertical
    // scrollbar will not pull horizontal one out
    m_vList->SetColumnWidth(m_vList->GetColumnCount() - 1,0);

    for(int i = 0; v[i].letter; i++){
        wxString label;
        label.Printf(wxT("%-10ls %ls"),
            wxString::Format(wxT("%c: [%hs]"),
            v[i].letter,v[i].fsname).wc_str(),
            v[i].label);
        m_vList->InsertItem(i,label);
        wxCommandEvent e(wxEVT_COMMAND_MENU_SELECTED,ID_UpdateVolumeInformation);
        volume_info *v_copy = new volume_info;
        memcpy(v_copy,&v[i],sizeof(volume_info));
        e.SetInt(i); e.SetClientData((void *)v_copy); ProcessEvent(e);
    }

    // adjust list columns once again to reflect the actual layout
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(-1); ProcessEvent(evt);

    m_vList->Select(0);

    ::udefrag_release_vollist(v);
}

void MainFrame::OnSkipRem(wxCommandEvent& WXUNUSED(event))
{
    if(!m_busy){
        m_skipRem = m_menuBar->FindItem(ID_SkipRem)->IsChecked();
        m_listThread->m_rescan = true;
    }
}

void MainFrame::OnRescan(wxCommandEvent& WXUNUSED(event))
{
    if(!m_busy) m_listThread->m_rescan = true;
}

/** @} */
