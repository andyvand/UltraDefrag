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

#define UD_MakeToolItem(id, icon) \
    string.Printf(wxT("%hs%u"),#icon,size); \
    pic = utils.LoadPngResource(string.wc_str()); \
    if(pic) m_toolBar->AddTool(id,*pic); \
    delete pic;

#define UD_MakeToolCheckItem(id, icon) \
    string.Printf(wxT("%hs%u"),#icon,size); \
    pic = utils.LoadPngResource(string.wc_str()); \
    if(pic) m_toolBar->AddCheckTool(id,wxEmptyString, *pic); \
    delete pic;

/**
 * @brief Initializes tool bar.
 */
void MainFrame::InitToolbar()
{
    int size = wxSystemSettings::GetMetric(wxSYS_SMALLICON_X);
    if(size < 20) size = 16;
    else if(size < 24) size = 20;
    else if(size < 32) size = 24;
    else size = 32;

    //size = 32;

    m_toolBar = CreateToolBar();
    m_toolBar->SetToolBitmapSize(wxSize(size,size));

    wxBitmap *pic; Utils utils; wxString string;
    UD_MakeToolItem(ID_Analyze         , glass   )
    UD_MakeToolCheckItem(ID_Repeat     , repeat  )
    UD_MakeToolItem(ID_Defrag          , defrag  )
    UD_MakeToolItem(ID_QuickOpt        , quick   )
    UD_MakeToolItem(ID_FullOpt         , full    )
    UD_MakeToolItem(ID_MftOpt          , mft     )
    UD_MakeToolItem(ID_Pause           , pause   )
    UD_MakeToolItem(ID_Stop            , stop    )
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_ShowReport      , report  )
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_GuiOptions      , wrench  )
    m_toolBar->AddSeparator();
    UD_MakeToolCheckItem(ID_BootEnable , terminal)
    UD_MakeToolItem(ID_BootScript      , script  )
    m_toolBar->AddSeparator();
    UD_MakeToolItem(ID_HelpContents    , help    )

    m_toolBar->Realize();

    // initial settings (must be after realize)
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_toolBar->ToggleTool(ID_Repeat,m_repeat);
}

#undef UD_MakeToolItem
#undef UD_MakeToolCheckItem

/** @} */
