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
 * @file menu.cpp
 * @brief Menu.
 * @addtogroup Menu
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                        Menu for main window
// =======================================================================

/**
 * @brief Initializes main window.
 */
void MainFrame::InitMenu()
{
    // create menu
    wxMenu *menuAction = new wxMenu;
    menuAction->Append(ID_Analyze,wxT("&Analyze\tF5"),wxT("Analyze selected drives"));
    menuAction->AppendSeparator();
    menuAction->Append(ID_Repeat,wxT("Re&peat action\tShift+R"),wxT("Repeat until finished"));
    menuAction->Append(wxID_EXIT);

    wxMenu *menuReport = new wxMenu;
    menuReport->Append(ID_ShowReport,wxT("&Show report\tF8"),wxT("Show fragmented files report"));

    wxMenu *menuSettings = new wxMenu;
    menuSettings->Append(ID_ReportOptions,wxT("&Reports\tCtrl+R"),wxT("Select report options"));

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuAction,   wxT("&Action")  );
    menuBar->Append(menuReport,   wxT("&Report")  );
    menuBar->Append(menuSettings, wxT("&Settings"));
    menuBar->Append(menuHelp,     wxT("&Help")    );

    SetMenuBar(menuBar);
}

/** @} */
