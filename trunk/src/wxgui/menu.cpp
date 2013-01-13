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
 * @brief Initializes main menu.
 */
void MainFrame::InitMenu()
{
    // create when done menu
    wxMenu *menuWhenDone = new wxMenu;
    menuWhenDone->AppendRadioItem(ID_WhenDoneNone     , wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneExit     , wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneStandby  , wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneHibernate, wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneLogoff   , wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneReboot   , wxEmptyString);
    menuWhenDone->AppendRadioItem(ID_WhenDoneShutdown , wxEmptyString);

    // create action menu
    wxMenu *m_menuAction = new wxMenu;
    m_menuAction->Append(ID_Analyze , wxEmptyString);
    m_menuAction->Append(ID_Defrag  , wxEmptyString);
    m_menuAction->Append(ID_QuickOpt, wxEmptyString);
    m_menuAction->Append(ID_FullOpt , wxEmptyString);
    m_menuAction->Append(ID_MftOpt  , wxEmptyString);
    m_menuAction->Append(ID_Pause   , wxEmptyString);
    m_menuAction->Append(ID_Stop    , wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->AppendCheckItem(ID_Repeat , wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->AppendCheckItem(ID_SkipRem, wxEmptyString);
    m_menuAction->Append(ID_Rescan  , wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->Append(ID_Repair  , wxEmptyString);
    m_menuAction->AppendSeparator();
    m_subMenuWhenDone = m_menuAction->AppendSubMenu(menuWhenDone, wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->Append(ID_Exit    , wxEmptyString);

    // create report menu
    wxMenu *menuReport = new wxMenu;
    menuReport->Append(ID_ShowReport, wxEmptyString);

    // create language menu
    m_menuLanguage = new wxMenu;
    m_menuLanguage->Append(ID_LangShowLog   , wxEmptyString);
    m_menuLanguage->Append(ID_LangShowReport, wxEmptyString);
    m_menuLanguage->Append(ID_LangOpenFolder, wxEmptyString);
    m_menuLanguage->Append(ID_LangSubmit    , wxEmptyString);
    m_menuLanguage->AppendSeparator();

    wxString AppLocaleDir(wxGetCwd());
    AppLocaleDir.Append(wxT("/locale"));
    if(!wxDirExists(AppLocaleDir)){
        wxLogMessage(wxT("lang dir not found: %ls"), AppLocaleDir.wc_str());
        AppLocaleDir.Clear();
        AppLocaleDir.Append(wxGetCwd());
        AppLocaleDir.Append(wxT("/../wxgui/locale"));
    }
    if(!wxDirExists(AppLocaleDir)){
        wxLogMessage(wxT("lang dir not found: %ls"), AppLocaleDir.wc_str());
        AppLocaleDir.Clear();
        AppLocaleDir.Append(wxGetCwd());
        AppLocaleDir.Append(wxT("/../../wxgui/locale"));
    }

    const wxLanguageInfo *info;
    if(!wxDirExists(AppLocaleDir)){
        wxLogMessage(wxT("lang dir not found: %ls"), AppLocaleDir.wc_str());

        info = m_locale->FindLanguageInfo(wxT("en_US"));
        m_menuLanguage->AppendRadioItem(ID_LangSelection + info->Language, info->Description);
    } else {
        wxDir dir(AppLocaleDir);

        if(!dir.IsOpened()){
            wxLogMessage(wxT("can't open lang dir: %ls"), AppLocaleDir.wc_str());

            info = m_locale->FindLanguageInfo(wxT("en_US"));
            m_menuLanguage->AppendRadioItem(ID_LangSelection + info->Language, info->Description);
        } else {
            wxString folder;
            wxArrayString langArray;

            bool cont = dir.GetFirst(&folder, wxT("*"), wxDIR_DIRS);

            while(cont){
                info = m_locale->FindLanguageInfo(folder);
                langArray.Add(info->Description);

                cont = dir.GetNext(&folder);
            }

            langArray.Sort();

            unsigned int breakDelta = (unsigned int)((langArray.Count() + 4) / 3);
            unsigned int breakCnt = breakDelta - 4;
            for(unsigned int i=0;i<langArray.Count();i++){
                info = m_locale->FindLanguageInfo(langArray[i]);

                m_menuLanguage->AppendRadioItem(ID_LangSelection + info->Language, info->Description);
                if((i+1) % breakCnt == 0){
                    m_menuLanguage->Break();

                    breakCnt += breakDelta;
                }
            }
        }
    }

    // create GUI configuration menu
    wxMenu *menuGUIconfig = new wxMenu;
    menuGUIconfig->Append(ID_GuiFont   , wxEmptyString);
    menuGUIconfig->Append(ID_GuiOptions, wxEmptyString);

    // create boot configuration menu
    wxMenu *menuBootConfig = new wxMenu;
    menuBootConfig->AppendCheckItem(ID_BootEnable, wxEmptyString);
    menuBootConfig->Append(ID_BootScript         , wxEmptyString);

    // create settings menu
    wxMenu *menuSettings = new wxMenu;
    m_subMenuLanguage = menuSettings->AppendSubMenu(m_menuLanguage  , wxEmptyString);
    m_subMenuGUIconfig = menuSettings->AppendSubMenu(menuGUIconfig  , wxEmptyString);
    m_subMenuBootConfig = menuSettings->AppendSubMenu(menuBootConfig, wxEmptyString);
    menuSettings->Append(ID_ReportOptions, wxEmptyString);

    // create debug menu
    wxMenu *menuDebug = new wxMenu;
    menuDebug->Append(ID_DebugLog , wxEmptyString);
    menuDebug->Append(ID_DebugSend, wxEmptyString);

    // create help menu
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(ID_HelpContents    , wxEmptyString);
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpBestPractice, wxEmptyString);
    menuHelp->Append(ID_HelpFaq         , wxEmptyString);
    menuHelp->Append(ID_HelpLegend      , wxEmptyString);
    menuHelp->AppendSeparator();
    m_subMenuDebug = menuHelp->AppendSubMenu(menuDebug, wxEmptyString);
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpUpdate      , wxEmptyString);
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpAbout       , wxEmptyString);

    // create main menu
    m_menuBar = new wxMenuBar;
    m_menuBar->Append(m_menuAction, wxEmptyString);
    m_menuBar->Append(menuReport  , wxEmptyString);
    m_menuBar->Append(menuSettings, wxEmptyString);
    m_menuBar->Append(menuHelp    , wxEmptyString);

    SetMenuBar(m_menuBar);

    // initial settings
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);

    cfg->Read(wxT("/Algorithm/SkipRemovableMedia"),&m_skipRem,true);
    m_menuBar->FindItem(ID_SkipRem)->Check(m_skipRem);

    int id = m_locale->GetLanguage();
    m_menuBar->FindItem(ID_LangSelection + id)->Check(true);
}

/** @} */
