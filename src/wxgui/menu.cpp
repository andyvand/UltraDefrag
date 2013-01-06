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

/* label must always be second argument to be able to extract it using
   "xgettext -kUD_MakeMenuItem:2 -kUD_MakeMenuCheckItem:2" for translation*/
#define UD_MakeMenuItem(id, label, accel, menu) \
    ItemLabel->Clear();             \
    ItemLabel->Append(_(label));    \
    ItemLabel->Append(wxT("\t"));   \
    ItemLabel->Append(wxT(accel));  \
    menu->Append(id,ItemLabel->Trim());

#define UD_MakeMenuCheckItem(id, label, accel, menu) \
    ItemLabel->Clear();             \
    ItemLabel->Append(_(label));    \
    ItemLabel->Append(wxT("\t"));   \
    ItemLabel->Append(wxT(accel));  \
    menu->AppendCheckItem(id,ItemLabel->Trim());

/**
 * @brief Initializes main window.
 */
void MainFrame::InitMenu()
{
    wxString *ItemLabel = new wxString;

    // create when done menu
    wxMenu *menuWhenDone = new wxMenu;
    menuWhenDone->AppendRadioItem(ID_WhenDoneNone     ,_("&None"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneExit     ,_("E&xit"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneStandby  ,_("Stan&dby"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneHibernate,_("&Hibernate"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneLogoff   ,_("&Logoff"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneReboot   ,_("&Reboot"));
    menuWhenDone->AppendRadioItem(ID_WhenDoneShutdown ,_("&Shutdown"));

    // create action menu
    wxMenu *menuAction = new wxMenu;
    UD_MakeMenuItem(ID_Analyze     ,"&Analyze"             ,"F5"      ,menuAction)
    UD_MakeMenuItem(ID_Defrag      ,"&Defragment"          ,"F6"      ,menuAction)
    UD_MakeMenuItem(ID_QuickOpt    ,"&Quick optimization"  ,"F7"      ,menuAction)
    UD_MakeMenuItem(ID_FullOpt     ,"&Full optimization"   ,"Ctrl+F7" ,menuAction)
    UD_MakeMenuItem(ID_MftOpt      ,"&Optimize MFT"        ,"Shift+F7",menuAction)
    UD_MakeMenuItem(ID_Pause       ,"&Pause"               ,"Space"   ,menuAction)
    UD_MakeMenuItem(ID_Stop        ,"&Stop"                ,"Ctrl+C"  ,menuAction)
    menuAction->AppendSeparator();
    UD_MakeMenuCheckItem(ID_Repeat ,"Re&peat action"       ,"Shift+R" ,menuAction)
    menuAction->AppendSeparator();
    UD_MakeMenuCheckItem(ID_SkipRem,"Skip removable &media","Ctrl+M"  ,menuAction)
    UD_MakeMenuItem(ID_Rescan      ,"&Rescan drives"       ,"Ctrl+D"  ,menuAction)
    menuAction->AppendSeparator();
    menuAction->Append(ID_Repair   ,_("Repair dri&ves"));
    menuAction->AppendSeparator();
    menuAction->AppendSubMenu(menuWhenDone,_("&When done"));
    menuAction->AppendSeparator();
    UD_MakeMenuItem(ID_Exit        ,"E&xit"                ,"Alt+F4"  ,menuAction)

    // create report menu
    wxMenu *menuReport = new wxMenu;
    UD_MakeMenuItem(ID_ShowReport,"&Show report","F8",menuReport)

    // create language menu
    m_menuLanguage = new wxMenu;
    m_menuLanguage->Append(ID_LangShowLog   ,_("&View change log"));
    m_menuLanguage->Append(ID_LangShowReport,_("View translation &report"));
    m_menuLanguage->Append(ID_LangOpenFolder,_("&Translations folder"));
    m_menuLanguage->Append(ID_LangSubmit    ,_("&Submit current translation"));

    // create GUI configuration menu
    wxMenu *menuGUIconfig = new wxMenu;
    UD_MakeMenuItem(ID_GuiFont   ,"&Font"   ,"F9" ,menuGUIconfig)
    UD_MakeMenuItem(ID_GuiOptions,"&Options","F10",menuGUIconfig)

    // create boot configuration menu
    wxMenu *menuBootConfig = new wxMenu;
    UD_MakeMenuItem(ID_BootEnable,"&Enable","F11",menuBootConfig);
    UD_MakeMenuItem(ID_BootScript,"&Script","F12",menuBootConfig);

    // create settings menu
    wxMenu *menuSettings = new wxMenu;
    menuSettings->AppendSubMenu(m_menuLanguage,_("&Language"));
    menuSettings->AppendSubMenu(menuGUIconfig ,_("&Graphical interface"));
    menuSettings->AppendSubMenu(menuBootConfig,_("&Boot time scan"));
    UD_MakeMenuItem(ID_ReportOptions,"&Reports","Ctrl+R",menuSettings)

    // create debug menu
    wxMenu *menuDebug = new wxMenu;
    UD_MakeMenuItem(ID_DebugLog,"Open &log","Alt+L",menuDebug)
    menuDebug->Append(ID_DebugSend,_("Send bug &report"));

    // create help menu
    wxMenu *menuHelp = new wxMenu;
    UD_MakeMenuItem(ID_HelpContents    ,"&Contents"     ,"F1",menuHelp)
    menuHelp->AppendSeparator();
    UD_MakeMenuItem(ID_HelpBestPractice,"Best &practice","F2",menuHelp)
    UD_MakeMenuItem(ID_HelpFaq         ,"&FAQ"          ,"F3",menuHelp)
    menuHelp->Append(ID_HelpLegend     ,_("Cluster map &legend"));
    menuHelp->AppendSeparator();
    menuHelp->AppendSubMenu(menuDebug  ,_("&Debug"));
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpUpdate     ,_("Check for &update"));
    menuHelp->AppendSeparator();
    UD_MakeMenuItem(ID_HelpAbout       ,"&About"        ,"F4",menuHelp)

    // create main menu
    m_menuBar = new wxMenuBar;
    m_menuBar->Append(menuAction,  _("&Action"));
    m_menuBar->Append(menuReport,  _("&Report"));
    m_menuBar->Append(menuSettings,_("&Settings"));
    m_menuBar->Append(menuHelp,    _("&Help"));

    SetMenuBar(m_menuBar);

    // initial settings
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);

    cfg->Read(wxT("/Algorithm/SkipRemovableMedia"),&m_skipRem,true);
    m_menuBar->FindItem(ID_SkipRem)->Check(m_skipRem);
}

#undef UD_MakeMenuItem
#undef UD_MakeMenuCheckItem

/** @} */
