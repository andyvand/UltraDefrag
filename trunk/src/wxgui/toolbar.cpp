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
//                       Tool bar for main window
// =======================================================================

#define UD_MakeToolItem(id, bmpidx) \
    ItemLabel->Clear();             \
    ItemLabel->Append(m_menuBar->FindItem(id)->GetItemLabelText()); \
    m_toolBar->AddTool(id,toolBarBitmaps[bmpidx],ItemLabel->Trim());

#define UD_MakeToolCheckItem(id, bmpidx) \
    ItemLabel->Clear();             \
    ItemLabel->Append(m_menuBar->FindItem(id)->GetItemLabelText()); \
    m_toolBar->AddCheckTool(id,wxEmptyString,toolBarBitmaps[bmpidx],wxNullBitmap,ItemLabel->Trim());

/**
 * @brief Initializes tool bar.
 */
void MainFrame::InitToolbar()
{
    m_toolBar = CreateToolBar();
    wxBitmap toolbarImage;

    int size = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
    if(size < 20){
        wxDisplay display;
        if(display.GetCurrentMode().GetDepth() > 8){
            toolbarImage = wxBITMAP(toolbar16);
        } else {
            wxSystemOptions::SetOption(wxT("msw.remap"),0);
            toolbarImage = wxBITMAP(toolbar16_8bit);
        }
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

    UD_MakeToolItem(ID_Analyze     ,ID_BMP_Analyze)
    UD_MakeToolCheckItem(ID_Repeat ,ID_BMP_Repeat)
    UD_MakeToolItem(ID_Defrag      ,ID_BMP_Defrag)
    UD_MakeToolItem(ID_QuickOpt    ,ID_BMP_QuickOpt)
    UD_MakeToolItem(ID_FullOpt     ,ID_BMP_FullOpt)
    UD_MakeToolItem(ID_MftOpt      ,ID_BMP_MftOpt)
    UD_MakeToolItem(ID_Pause       ,ID_BMP_Pause)
    UD_MakeToolItem(ID_Stop        ,ID_BMP_Stop)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_ShowReport  ,ID_BMP_ShowReport)
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_GuiOptions  ,ID_BMP_GuiOptions)
    m_toolBar->AddSeparator();
    m_toolBar->AddCheckTool(ID_BootEnable,wxEmptyString,
        toolBarBitmaps[ID_BMP_BootEnable],wxNullBitmap,
        _("&Boot time scan"));
    m_toolBar->AddTool(ID_BootScript,
        toolBarBitmaps[ID_BMP_BootScript],
        _("Boot time script"));
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_HelpContents,ID_BMP_HelpContents)

    m_toolBar->Realize();

    // initial settings (must be after realize)
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_toolBar->ToggleTool(ID_Repeat,m_repeat);

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
