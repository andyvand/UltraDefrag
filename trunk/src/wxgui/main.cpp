//////////////////////////////////////////////////////////////////////////
//
//  UltraDefrag - a powerful defragmentation tool for Windows NT.
//  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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
 * @file main.c
 * @brief Main window.
 * @addtogroup MainWindow
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
// declarations
// =======================================================================

#include "main.h"

class App: public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();
};

class MainFrame: public wxFrame {
private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
END_EVENT_TABLE()

IMPLEMENT_APP(App)

// =======================================================================
// implementation
// =======================================================================

/**
 * @brief Initializes the application.
 */
bool App::OnInit()
{
    return true;
}

/**
 * @brief Deinitializes the application.
 */
int App::OnExit()
{
    return 0;
}

/** @} */
