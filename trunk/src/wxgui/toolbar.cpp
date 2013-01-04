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
 * @file toolbar.cpp
 * @brief Toolbar.
 * @addtogroup Toolbar
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                        Toolbar for main window
// =======================================================================

#define TOOLBAR_IMAGE_COUNT 14

/**
 * @brief Initializes toolbar.
 */
void MainFrame::InitToolbar()
{
    m_ToolBar = CreateToolBar(wxNO_BORDER | wxTB_HORIZONTAL | wxTB_FLAT);

    wxBitmap toolbarImage = wxBITMAP(toolbar16);
    wxMask *imageMask = new wxMask(toolbarImage,wxColor(255,0,255,wxALPHA_OPAQUE));
    toolbarImage.SetMask(imageMask);

    int bmHeight = toolbarImage.GetHeight();

    wxBitmap toolBarBitmaps[TOOLBAR_IMAGE_COUNT];

    int i;
    for(i=0; i < TOOLBAR_IMAGE_COUNT; i++)
        toolBarBitmaps[i] = toolbarImage.GetSubBitmap(wxRect(bmHeight*i,0,bmHeight,bmHeight));

    m_ToolBar->SetToolBitmapSize(wxSize(bmHeight,bmHeight));

    m_ToolBar->AddTool(ID_Analyze,   wxT("Analyze"),   toolBarBitmaps[0], _("Analyze"),                  wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_Repeat,    wxT("Repeat"),    toolBarBitmaps[1], _("Repeat"),                   wxITEM_CHECK);
    m_ToolBar->AddTool(ID_Defrag,    wxT("Defrag"),    toolBarBitmaps[3], _("Defragment"),               wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_QuickOpt,  wxT("QuickOpt"),  toolBarBitmaps[4], _("Quick Optimize"),           wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_FullOpt,   wxT("FullOpt"),   toolBarBitmaps[5], _("Full Optimize"),            wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_MftOpt,    wxT("MftOpt"),    toolBarBitmaps[6], _("Optimize MFT"),             wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_Pause,     wxT("Pause"),     toolBarBitmaps[7], _("Pause"),                    wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_Stop,      wxT("Stop"),      toolBarBitmaps[8], _("Stop"),                     wxITEM_NORMAL);
    m_ToolBar->AddSeparator();
    m_ToolBar->AddTool(ID_ShowReport,wxT("ShowReport"),toolBarBitmaps[9], _("Show Fragmentation Report"),wxITEM_NORMAL);
    m_ToolBar->AddSeparator();
    m_ToolBar->AddTool(ID_GuiOptions,wxT("GuiOptions"),toolBarBitmaps[10],_("Edit GUI Settings"),        wxITEM_NORMAL);
    m_ToolBar->AddSeparator();
    m_ToolBar->AddTool(ID_BootEnable,wxT("BootEnable"),toolBarBitmaps[11],_("Enable Boot"),              wxITEM_NORMAL);
    m_ToolBar->AddTool(ID_BootScript,wxT("BootScript"),toolBarBitmaps[12],_("Edit Boot Script"),         wxITEM_NORMAL);
    m_ToolBar->AddSeparator();
    m_ToolBar->AddTool(ID_HelpAbout, wxT("HelpAbout"), toolBarBitmaps[13],_("About"),                    wxITEM_NORMAL);

    m_ToolBar->Realize();
}

/** @} */
