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
 * @file config.cpp
 * @brief Configuration.
 * @addtogroup Configuration
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                      Application configuration
// =======================================================================

/**
 * @brief Reads application configuration.
 * @note Intended to be called once somewhere
 * in the main frame constructor.
 */
void MainFrame::ReadAppConfiguration()
{
    wxConfigBase *cfg = wxConfigBase::Get();

    // get window size and position
    m_saved = cfg->HasGroup(wxT("MainFrame"));
    m_x = (int)cfg->Read(wxT("/MainFrame/x"),0l);
    m_y = (int)cfg->Read(wxT("/MainFrame/y"),0l);
    m_width = (int)cfg->Read(wxT("/MainFrame/width"),
        DPI(MAIN_WINDOW_DEFAULT_WIDTH));
    m_height = (int)cfg->Read(wxT("/MainFrame/height"),
        DPI(MAIN_WINDOW_DEFAULT_HEIGHT));

    // validate width and height
    wxDisplay display;
    if(m_width < DPI(MAIN_WINDOW_MIN_WIDTH)) m_width = DPI(MAIN_WINDOW_MIN_WIDTH);
    if(m_width > display.GetClientArea().width) m_width = DPI(MAIN_WINDOW_DEFAULT_WIDTH);
    if(m_height < DPI(MAIN_WINDOW_MIN_HEIGHT)) m_height = DPI(MAIN_WINDOW_MIN_HEIGHT);
    if(m_height > display.GetClientArea().height) m_height = DPI(MAIN_WINDOW_DEFAULT_HEIGHT);

    // validate x and y
    if(m_x < 0) m_x = 0; if(m_y < 0) m_y = 0;
    if(m_x > display.GetClientArea().width - 130)
        m_x = display.GetClientArea().width - 130;
    if(m_y > display.GetClientArea().height - 50)
        m_y = display.GetClientArea().height - 50;

    // now the window is surely inside of the screen

    cfg->Read(wxT("/MainFrame/maximized"),&m_maximized,false);

    m_separatorPosition = (int)cfg->Read(
        wxT("/MainFrame/SeparatorPosition"),
        (long)DPI(DEFAULT_LIST_HEIGHT)
    );

    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    cfg->Read(wxT("/Algorithm/SkipRemovableMedia"),&m_skipRem,true);
}

/**
 * @brief Saves application configuration.
 * @note Intended to be called in the main
 * frame destructor before termination
 * of threads.
 */
void MainFrame::SaveAppConfiguration()
{
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Write(wxT("/MainFrame/x"),(long)m_x);
    cfg->Write(wxT("/MainFrame/y"),(long)m_y);
    cfg->Write(wxT("/MainFrame/width"),(long)m_width);
    cfg->Write(wxT("/MainFrame/height"),(long)m_height);
    cfg->Write(wxT("/MainFrame/maximized"),(long)IsMaximized());
    cfg->Write(wxT("/MainFrame/SeparatorPosition"),
        (long)m_splitter->GetSashPosition());

    cfg->Write(wxT("/DrivesList/width1"),m_w1);
    cfg->Write(wxT("/DrivesList/width2"),m_w2);
    cfg->Write(wxT("/DrivesList/width3"),m_w3);
    cfg->Write(wxT("/DrivesList/width4"),m_w4);
    cfg->Write(wxT("/DrivesList/width5"),m_w5);
    cfg->Write(wxT("/DrivesList/width6"),m_w6);

    cfg->Write(wxT("/Language/Selected"),
        (long)g_locale->GetLanguage());

    cfg->Write(wxT("/Algorithm/RepeatAction"),m_repeat);
    cfg->Write(wxT("/Algorithm/SkipRemovableMedia"),m_skipRem);

    cfg->Write(wxT("/Upgrade/Level"),
        (long)m_upgradeThread->m_level);
}

// =======================================================================
//                          User preferences
// =======================================================================

// =======================================================================
//                      User preferences tracking
// =======================================================================

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnGuiFont(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnGuiOptions(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnBootScript(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnReportOptions(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnSortCriteriaChange(wxCommandEvent& WXUNUSED(event))
{
}

void MainFrame::OnSortOrderChange(wxCommandEvent& WXUNUSED(event))
{
}

/** @} */
