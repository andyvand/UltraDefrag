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
 * @file map.cpp
 * @brief Cluster map.
 * @details Even wxBufferedPaintDC
 * is too expensive and causes flicker
 * on map resize, so we're using low
 * level API here.
 * @addtogroup ClusterMap
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                            Cluster map
// =======================================================================

ClusterMap::ClusterMap(wxWindow* parent) : wxWindow(parent,wxID_ANY)
{
    HDC hdc = GetDC((HWND)GetHandle());
    m_cacheDC = ::CreateCompatibleDC(hdc);
    if(!m_cacheDC) letrace("cannot create cache dc");
    m_cacheBmp = ::CreateCompatibleBitmap(m_cacheDC,
        wxGetDisplaySize().GetWidth(),
        wxGetDisplaySize().GetHeight()
    );
    if(!m_cacheBmp) letrace("cannot create cache bitmap");
    ::SelectObject(m_cacheDC,m_cacheBmp);
    ::SetBkMode(m_cacheDC,TRANSPARENT);
    ::ReleaseDC((HWND)GetHandle(),hdc);

    m_width = m_height = 0;
}

ClusterMap::~ClusterMap()
{
    ::DeleteDC(m_cacheDC);
    ::DeleteObject(m_cacheBmp);
}

// =======================================================================
//                           Event handlers
// =======================================================================

BEGIN_EVENT_TABLE(ClusterMap, wxWindow)
    EVT_ERASE_BACKGROUND(ClusterMap::OnEraseBackground)
    EVT_PAINT(ClusterMap::OnPaint)
END_EVENT_TABLE()

void ClusterMap::OnEraseBackground(wxEraseEvent& event)
{
    int width, height; GetClientSize(&width,&height);
    if(width > m_width || height > m_height){
        // expand free space to reduce flicker
        HDC hdc = GetDC((HWND)GetHandle());

        char free_r = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_R"));
        char free_g = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_G"));
        char free_b = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_B"));
        HBRUSH brush = ::CreateSolidBrush(RGB(free_r,free_g,free_b));

        RECT rc; rc.left = m_width; rc.top = 0;
        rc.right = width; rc.bottom = height;
        ::FillRect(hdc,&rc,brush);

        rc.left = 0; rc.top = m_height;
        rc.right = width; rc.bottom = height;
        ::FillRect(hdc,&rc,brush);

        ::DeleteObject(brush);

        ::ReleaseDC((HWND)GetHandle(),hdc);
    }
    m_width = width; m_height = height;
}

void ClusterMap::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    int width, height; GetClientSize(&width,&height);

    int block_size = g_mainFrame->CheckOption(wxT("UD_MAP_BLOCK_SIZE"));
    int line_width = g_mainFrame->CheckOption(wxT("UD_GRID_LINE_WIDTH"));

    int cell_size = block_size + line_width;
    int blocks_per_line = (width - line_width) / cell_size;
    int lines = (height - line_width) / cell_size;

    // fill map by the free color
    char free_r = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_R"));
    char free_g = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_G"));
    char free_b = (char)g_mainFrame->CheckOption(wxT("UD_FREE_COLOR_B"));
    HBRUSH brush = ::CreateSolidBrush(RGB(free_r,free_g,free_b));
    RECT rc; rc.left = rc.top = 0; rc.right = width; rc.bottom = height;
    ::FillRect(m_cacheDC,&rc,brush); ::DeleteObject(brush);
    if(!blocks_per_line || !lines) return;

    // draw grid
    if(line_width){
        char grid_r = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_R"));
        char grid_g = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_G"));
        char grid_b = (char)g_mainFrame->CheckOption(wxT("UD_GRID_COLOR_B"));
        brush = ::CreateSolidBrush(RGB(grid_r,grid_g,grid_b));
        for(int i = 0; i < blocks_per_line + 1; i++){
            RECT rc; rc.left = cell_size * i; rc.top = 0;
            rc.right = rc.left + line_width;
            rc.bottom = cell_size * lines + line_width;
            ::FillRect(m_cacheDC,&rc,brush);
        }
        for(int i = 0; i < lines + 1; i++){
            RECT rc; rc.left = 0; rc.top = cell_size * i;
            rc.right = cell_size * blocks_per_line + line_width;
            rc.bottom = rc.top + line_width;
            ::FillRect(m_cacheDC,&rc,brush);
        }
        ::DeleteObject(brush);
    }

    // draw squares
    JobsCacheEntry *currentJob = g_mainFrame->m_currentJob;
    if(currentJob){
        if(currentJob->pi.cluster_map_size){
        }
    }

    // draw map on the screen
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint((HWND)GetHandle(),&ps);
    ::BitBlt(hdc,0,0,width,height,m_cacheDC,0,0,SRCCOPY);
    ::EndPaint((HWND)GetHandle(),&ps);
}

void MainFrame::RedrawMap(wxCommandEvent& WXUNUSED(event))
{
    m_cMap->Refresh();
}

/** @} */
