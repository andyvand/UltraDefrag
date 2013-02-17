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

// =======================================================================
//                           List of volumes
// =======================================================================

void MainFrame::InitVolList()
{
    wxConfigBase *cfg = wxConfigBase::Get();
    int w1 = (int)cfg->Read(wxT("/DrivesList/width1"),0l);
    int w2 = (int)cfg->Read(wxT("/DrivesList/width2"),0l);
    int w3 = (int)cfg->Read(wxT("/DrivesList/width3"),0l);
    int w4 = (int)cfg->Read(wxT("/DrivesList/width4"),0l);
    int w5 = (int)cfg->Read(wxT("/DrivesList/width5"),0l);
    int w6 = (int)cfg->Read(wxT("/DrivesList/width6"),0l);

    if(!w1) w1 = 110; if(!w2) w2 = 110; if(!w3) w3 = 110;
    if(!w4) w4 = 110; if(!w5) w5 = 110; if(!w6) w6 = 65;

    // adjust widths so all the columns will fit to the window
    int w, h; m_vList->GetClientSize(&w,&h);
    double scale = (double)w / (w1 + w2 + w3 + w4 + w5 + w6);
    w1 *= scale; w2 *= scale; w3 *= scale; w4 *= scale; w5 *= scale;
    w6 = w - w1 - w2 - w3 - w4 - w5;

    Connect(wxEVT_SIZE,wxSizeEventHandler(MainFrame::OnListSize),NULL,this);
    m_splitter->Connect(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING,
        wxSplitterEventHandler(MainFrame::OnSplitChanging),NULL,this);
    m_splitter->Connect(wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
        wxSplitterEventHandler(MainFrame::OnSplitChanged),NULL,this);

    m_vList->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT,  w1);
    m_vList->InsertColumn(1, wxEmptyString, wxLIST_FORMAT_LEFT,  w2);
    m_vList->InsertColumn(2, wxEmptyString, wxLIST_FORMAT_RIGHT, w3);
    m_vList->InsertColumn(3, wxEmptyString, wxLIST_FORMAT_RIGHT, w4);
    m_vList->InsertColumn(4, wxEmptyString, wxLIST_FORMAT_RIGHT, w5);
    m_vList->InsertColumn(5, wxEmptyString, wxLIST_FORMAT_RIGHT, w6);

    // attach drive icons
    int size = g_iconSize;
    wxImageList *list = new wxImageList(size,size);
    list->Add(wxIcon(wxT("fixed")           , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    list->Add(wxIcon(wxT("fixed_dirty")     , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    list->Add(wxIcon(wxT("removable")       , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    list->Add(wxIcon(wxT("removable_dirty") , wxBITMAP_TYPE_ICO_RESOURCE, size, size));
    m_vList->SetImageList(list,wxIMAGE_LIST_SMALL);
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnSplitChanging(wxSplitterEvent& event)
{
    event.Skip();
}

void MainFrame::OnSplitChanged(wxSplitterEvent& event)
{
    // ensure that the vertical scroll-bar will not spoil the list appearance
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(-1); wxPostEvent(this,evt);

    event.Skip();
}

void MainFrame::OnListSize(wxSizeEvent& event)
{
    // scale columns; use some tricks to avoid horizontal scroll-bar appearance
    int new_width = GetClientSize().GetWidth();
    new_width -= 2 * wxSystemSettings::GetMetric(wxSYS_EDGE_X);
    new_width -= wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    int old_width = m_vList->GetClientSize().GetWidth();
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(new_width);
    if(new_width <= old_width) ProcessEvent(evt);
    else wxPostEvent(this,evt);

    // adjust widths once again later when the list will be scaled actually
    evt.SetInt(-1); wxPostEvent(this,evt);

    event.Skip();
}

void MainFrame::AdjustListColumns(wxCommandEvent& event)
{
    int w1 = m_vList->GetColumnWidth(0);
    int w2 = m_vList->GetColumnWidth(1);
    int w3 = m_vList->GetColumnWidth(2);
    int w4 = m_vList->GetColumnWidth(3);
    int w5 = m_vList->GetColumnWidth(4);
    int w6 = m_vList->GetColumnWidth(5);

    if(!w1) w1 = 110; if(!w2) w2 = 110; if(!w3) w3 = 110;
    if(!w4) w4 = 110; if(!w5) w5 = 110; if(!w6) w6 = 65;

    int width = event.GetInt();
    if(width < 0) width = m_vList->GetClientSize().GetWidth();
    double scale = (double)width / (w1 + w2 + w3 + w4 + w5 + w6);
    w1 *= scale; w2 *= scale; w3 *= scale; w4 *= scale; w5 *= scale;
    w6 = width - w1 - w2 - w3 - w4 - w5;

    m_vList->SetColumnWidth(0,w1);
    m_vList->SetColumnWidth(1,w2);
    m_vList->SetColumnWidth(2,w3);
    m_vList->SetColumnWidth(3,w4);
    m_vList->SetColumnWidth(4,w5);
    m_vList->SetColumnWidth(5,w6);
}

// =======================================================================
//                            Drives scanner
// =======================================================================

void *ListThread::Entry()
{
    while(!m_stop){
        if(m_rescan){
            volume_info *v = udefrag_get_vollist(g_mainFrame->m_skipRem);
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

void MainFrame::PopulateList(wxCommandEvent& event)
{
    volume_info *v = (volume_info *)event.GetClientData();

    m_vList->DeleteAllItems();

    for(int i = 0; v[i].letter; i++){
        wxString label;
        label.Printf(wxT("%-10ls %ls"),
            wxString::Format(wxT("%c: [%hs]"),
            v[i].letter,v[i].fsname).wc_str(),
            v[i].label);
        int imageIndex = v[i].is_removable ? 2 : 0;
        if(v[i].is_dirty) imageIndex ++;
        m_vList->InsertItem(i,label,imageIndex);
    }

    m_vList->Select(0);

    // ensure that the vertical scroll-bar will not spoil the list appearance
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_AdjustListColumns);
    evt.SetInt(m_vList->GetClientSize().GetWidth()); ProcessEvent(evt);

    udefrag_release_vollist(v);
}

void MainFrame::OnSkipRem(wxCommandEvent& WXUNUSED(event))
{
    m_skipRem = m_menuBar->FindItem(ID_SkipRem)->IsChecked();
    m_listThread->m_rescan = true;
}

void MainFrame::OnRescan(wxCommandEvent& WXUNUSED(event))
{
    m_listThread->m_rescan = true;
}

/** @} */
