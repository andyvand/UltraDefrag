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

#define UD_MakeMenuItem(id, label, accel, menu) \
    ItemLabel->Clear();             \
    ItemLabel->Append(label);       \
    ItemLabel->Append(wxT("\t"));   \
    ItemLabel->Append(wxT(accel));  \
    menu->Append(id,ItemLabel->Trim());

#define UD_MakeMenuCheckItem(id, label, accel, menu) \
    ItemLabel->Clear();             \
    ItemLabel->Append(label);       \
    ItemLabel->Append(wxT("\t"));   \
    ItemLabel->Append(wxT(accel));  \
    menu->AppendCheckItem(id,ItemLabel->Trim());

/**
 * @brief Initializes main window.
 */
void MainFrame::InitMenu()
{
    // create action menu
    wxMenu *menuAction = new wxMenu;
    wxString *ItemLabel = new wxString;

    UD_MakeMenuItem(ID_Analyze ,_("&Analyze")           ,"F5"      ,menuAction)
    UD_MakeMenuItem(ID_Defrag  ,_("&Defragment")        ,"F6"      ,menuAction)
    UD_MakeMenuItem(ID_QuickOpt,_("&Quick optimization"),"F7"      ,menuAction)
    UD_MakeMenuItem(ID_FullOpt ,_("&Full optimization") ,"Ctrl+F7" ,menuAction)
    UD_MakeMenuItem(ID_MftOpt  ,_("&Optimize MFT")      ,"Shift+F7",menuAction)
    UD_MakeMenuItem(ID_Pause   ,_("&Pause")             ,"Space"   ,menuAction)
    UD_MakeMenuItem(ID_Stop    ,_("&Stop")              ,"Ctrl+C"  ,menuAction)
    menuAction->AppendSeparator();
    UD_MakeMenuCheckItem(ID_Repeat,_("Re&peat action")  ,"Shift+R" ,menuAction)
    UD_MakeMenuItem(ID_Exit    ,_("E&xit")              ,"Alt+F4"  ,menuAction)

    // create report menu
    wxMenu *menuReport = new wxMenu;
    UD_MakeMenuItem(ID_ShowReport,_("&Show report"),"F8",menuReport)

    // create settings menu
    wxMenu *menuSettings = new wxMenu;
    UD_MakeMenuItem(ID_GuiOptions   ,_("&Edit GUI options")     ,"F10"   ,menuSettings)
    UD_MakeMenuItem(ID_BootEnable   ,_("Enable &boot time scan"),"F11"   ,menuSettings)
    UD_MakeMenuItem(ID_BootScript   ,_("Edit boot time &script"),"F12"   ,menuSettings)
    UD_MakeMenuItem(ID_ReportOptions,_("&Reports")              ,"Ctrl+R",menuSettings)

    // create help menu
    wxMenu *menuHelp = new wxMenu;
    UD_MakeMenuItem(ID_HelpAbout,_("&About"),"F4",menuHelp)

    // create main menu
    m_menuBar = new wxMenuBar;
    m_menuBar->Append(menuAction,   _("&Action")  );
    m_menuBar->Append(menuReport,   _("&Report")  );
    m_menuBar->Append(menuSettings, _("&Settings"));
    m_menuBar->Append(menuHelp,     _("&Help")    );

    SetMenuBar(m_menuBar);
    
    // initial settings
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);
}

#undef UD_MakeMenuItem
#undef UD_MakeMenuCheckItem

/** @} */
