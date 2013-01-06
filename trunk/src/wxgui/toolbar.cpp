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

#define UD_MakeToolItem(id, bmpidx) \
    ItemLabel->Clear();             \
    ItemLabel->Append(m_menuBar->FindItem(id)->GetItemLabelText()); \
    m_toolBar->AddTool(id,toolBarBitmaps[bmpidx],ItemLabel->Trim());

#define UD_MakeToolCheckItem(id, bmpidx) \
    ItemLabel->Clear();             \
    ItemLabel->Append(m_menuBar->FindItem(id)->GetItemLabelText()); \
    m_toolBar->AddCheckTool(id,wxEmptyString,toolBarBitmaps[bmpidx],wxNullBitmap,ItemLabel->Trim());

/**
 * @brief Initializes toolbar.
 */
void MainFrame::InitToolbar()
{
    m_toolBar = CreateToolBar();

    wxBitmap toolbarImage;
    int size = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
    if(size < 20){
        wxDisplay display;
        toolbarImage = display.GetCurrentMode().GetDepth() > 8 ? \
            wxBITMAP(toolbar16) : wxBITMAP(toolbar16_8bit);
    } else if(size < 24){
        toolbarImage = wxBITMAP(toolbar20);
    } else if(size < 32){
        toolbarImage = wxBITMAP(toolbar24);
    } else {
        toolbarImage = wxBITMAP(toolbar32);
    }
        
    wxMask *imageMask = new wxMask(toolbarImage,wxColor(255,0,255,wxALPHA_OPAQUE));
    toolbarImage.SetMask(imageMask);

    int bmHeight = toolbarImage.GetHeight();
    m_toolBar->SetToolBitmapSize(wxSize(bmHeight,bmHeight));

    wxBitmap toolBarBitmaps[TOOLBAR_IMAGE_COUNT];
    for(int i = 0; i < TOOLBAR_IMAGE_COUNT; i++)
        toolBarBitmaps[i] = toolbarImage.GetSubBitmap(wxRect(bmHeight*i,0,bmHeight,bmHeight));

    wxString *ItemLabel = new wxString;

    UD_MakeToolItem(ID_Analyze    ,0)
    UD_MakeToolCheckItem(ID_Repeat,1)
    UD_MakeToolItem(ID_Defrag     ,3)
    UD_MakeToolItem(ID_QuickOpt   ,4)
    UD_MakeToolItem(ID_FullOpt    ,5)
    UD_MakeToolItem(ID_MftOpt     ,6)
    UD_MakeToolItem(ID_Pause      ,7)
    UD_MakeToolItem(ID_Stop       ,8)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_ShowReport ,9)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_GuiOptions,10)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_BootEnable,11)
    UD_MakeToolItem(ID_BootScript,12)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_HelpAbout ,13)

    m_toolBar->Realize();

    // initial settings (must be after realize)
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_toolBar->ToggleTool(ID_Repeat,m_repeat);

    m_toolBar->EnableTool(ID_BootEnable,false);
    m_toolBar->EnableTool(ID_BootScript,false);

    // uncomment to test grayed out images
    /*m_toolBar->EnableTool(ID_Analyze,false);
    m_toolBar->EnableTool(ID_Repeat,false);
    m_toolBar->EnableTool(ID_Defrag,false);
    m_toolBar->EnableTool(ID_QuickOpt,false);
    m_toolBar->EnableTool(ID_FullOpt,false);
    m_toolBar->EnableTool(ID_MftOpt,false);
    m_toolBar->EnableTool(ID_BootEnable,false);
    m_toolBar->EnableTool(ID_BootScript,false);*/
}

#undef UD_MakeToolItem
#undef UD_MakeToolCheckItem

/** @} */
