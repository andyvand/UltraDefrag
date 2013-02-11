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

#define UD_SetMenuIcon(id, icon) \
    string.Printf(wxT("%hs%u"),#icon,g_iconSize); \
    pic = Utils::LoadPngResource(string.wc_str()); \
    if(pic) m_menuBar->FindItem(id)->SetBitmap(*pic); \
    delete pic;

#define UD_SetMarginWidth(menu) \
    list = menu->GetMenuItems(); \
    count = list.GetCount(); \
    for(index = 0; index < count; index++) \
        list.Item(index)->GetData()->SetMarginWidth(g_iconSize);

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
    m_menuAction->Append(ID_Analyze);
    m_menuAction->Append(ID_Defrag);
    m_menuAction->Append(ID_QuickOpt);
    m_menuAction->Append(ID_FullOpt);
    m_menuAction->Append(ID_MftOpt);
    m_menuAction->Append(ID_Pause);
    m_menuAction->Append(ID_Stop);
    m_menuAction->AppendSeparator();
    m_menuAction->AppendCheckItem( \
        ID_Repeat, wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->AppendCheckItem( \
        ID_SkipRem, wxEmptyString);
    m_menuAction->Append(ID_Rescan);
    m_menuAction->AppendSeparator();
    m_menuAction->Append(ID_Repair);
    m_menuAction->AppendSeparator();
    m_subMenuWhenDone = \
        m_menuAction->AppendSubMenu( \
        menuWhenDone, wxEmptyString);
    m_menuAction->AppendSeparator();
    m_menuAction->Append(ID_Exit);

    // create report menu
    wxMenu *menuReport = new wxMenu;
    menuReport->Append(ID_ShowReport);

    // create language menu
    m_menuLanguage = new wxMenu;
    m_menuLanguage->Append(ID_LangShowLog);
    m_menuLanguage->Append(ID_LangShowReport);
    m_menuLanguage->Append(ID_LangOpenFolder);
    m_menuLanguage->Append(ID_LangSubmit);
    m_menuLanguage->AppendSeparator();

    wxString AppLocaleDir(wxGetCwd());
    AppLocaleDir.Append(wxT("/locale"));
    if(!wxDirExists(AppLocaleDir)){
        wxLogMessage(wxT("lang dir not found: %ls"), AppLocaleDir.wc_str());
        AppLocaleDir = wxGetCwd() + wxT("/../wxgui/locale");
    }
    if(!wxDirExists(AppLocaleDir)){
        wxLogMessage(wxT("lang dir not found: %ls"), AppLocaleDir.wc_str());
        AppLocaleDir = wxGetCwd() + wxT("/../../wxgui/locale");
    }

    wxDir dir(AppLocaleDir);
    const wxLanguageInfo *info;

    if(!dir.IsOpened()){
        wxLogMessage(wxT("can't open lang dir: %ls"), AppLocaleDir.wc_str());
        info = g_locale->FindLanguageInfo(wxT("en_US"));
        m_menuLanguage->AppendRadioItem(ID_LocaleChange \
            + info->Language, info->Description);
    } else {
        wxString folder;
        wxArrayString langArray;

        bool cont = dir.GetFirst(&folder, wxT("*"), wxDIR_DIRS);

        while(cont){
            info = g_locale->FindLanguageInfo(folder);
            if(info){
                if(info->Description == wxT("Chinese")){
                    langArray.Add(wxT("Chinese (Traditional)"));
                } else {
                    if(info->Description == wxT("English")){
                        langArray.Add(wxT("English (U.K.)"));
                    } else {
                        langArray.Add(info->Description);
                    }
                }
            } else {
                wxLogMessage(wxT("can't find locale info for %ls"), folder.wc_str());
            }
            cont = dir.GetNext(&folder);
        }

        langArray.Sort();

        // divide list of languages to three columns
        unsigned int breakDelta = (unsigned int)ceil((double) \
            (langArray.Count() + langArray.Count() % 2 + 4) / 3);
        unsigned int breakCnt = breakDelta - 4;
        wxLogMessage(wxT("languages: %d, break count: %d, delta: %d"),
            langArray.Count(), breakCnt, breakDelta);
        for(unsigned int i = 0;i < langArray.Count();i++){
            info = g_locale->FindLanguageInfo(langArray[i]);
            m_menuLanguage->AppendRadioItem(ID_LocaleChange \
                + info->Language, info->Description);
            if((i+1) % breakCnt == 0){
                m_menuLanguage->Break();
                breakCnt += breakDelta;
            }
        }
    }

    // create GUI configuration menu
    wxMenu *menuGUIconfig = new wxMenu;
    menuGUIconfig->Append(ID_GuiFont);
    menuGUIconfig->Append(ID_GuiOptions);

    // create boot configuration menu
    wxMenu *menuBootConfig = new wxMenu;
    menuBootConfig->AppendCheckItem( \
        ID_BootEnable, wxEmptyString);
    menuBootConfig->Append(ID_BootScript);

    // create sorting configuration menu
    wxMenu *menuSortingConfig = new wxMenu;
    menuSortingConfig->AppendRadioItem( \
        ID_SortByPath, wxEmptyString);
    menuSortingConfig->AppendRadioItem( \
        ID_SortBySize, wxEmptyString);
    menuSortingConfig->AppendRadioItem( \
        ID_SortByCreationDate, wxEmptyString);
    menuSortingConfig->AppendRadioItem( \
        ID_SortByModificationDate, wxEmptyString);
    menuSortingConfig->AppendRadioItem( \
        ID_SortByLastAccessDate, wxEmptyString);
    menuSortingConfig->AppendSeparator();
    menuSortingConfig->AppendRadioItem( \
        ID_SortAscending, wxEmptyString);
    menuSortingConfig->AppendRadioItem( \
        ID_SortDescending, wxEmptyString);

    // create settings menu
    wxMenu *menuSettings = new wxMenu;
    m_subMenuLanguage = menuSettings->AppendSubMenu(m_menuLanguage  , wxEmptyString);
    m_subMenuGUIconfig = menuSettings->AppendSubMenu(menuGUIconfig  , wxEmptyString);
    m_subMenuBootConfig = menuSettings->AppendSubMenu(menuBootConfig, wxEmptyString);
    m_subMenuSortingConfig = menuSettings->AppendSubMenu(menuSortingConfig, wxEmptyString);
    menuSettings->Append(ID_ReportOptions);

    // create debug menu
    wxMenu *menuDebug = new wxMenu;
    menuDebug->Append(ID_DebugLog);
    menuDebug->Append(ID_DebugSend);

    // create upgrade menu
    wxMenu *menuUpgrade = new wxMenu;
    menuUpgrade->AppendRadioItem( \
        ID_HelpUpgradeNone, wxEmptyString);
    menuUpgrade->AppendRadioItem( \
        ID_HelpUpgradeStable, wxEmptyString);
    menuUpgrade->AppendRadioItem( \
        ID_HelpUpgradeAll, wxEmptyString);
    menuUpgrade->AppendSeparator();
    menuUpgrade->Append(ID_HelpUpgradeCheck);

    // create help menu
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(ID_HelpContents);
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpBestPractice);
    menuHelp->Append(ID_HelpFaq);
    menuHelp->Append(ID_HelpLegend);
    menuHelp->AppendSeparator();
    m_subMenuDebug = \
        menuHelp->AppendSubMenu( \
        menuDebug, wxEmptyString);
    menuHelp->AppendSeparator();
    m_subMenuUpgrade = \
        menuHelp->AppendSubMenu( \
        menuUpgrade, wxEmptyString);
    menuHelp->AppendSeparator();
    menuHelp->Append(ID_HelpAbout);

    // create main menu
    m_menuBar = new wxMenuBar;
    m_menuBar->Append(m_menuAction, wxEmptyString);
    m_menuBar->Append(menuReport  , wxEmptyString);
    m_menuBar->Append(menuSettings, wxEmptyString);
    m_menuBar->Append(menuHelp    , wxEmptyString);

    SetMenuBar(m_menuBar);

    // set menu icons
    wxBitmap *pic; wxString string;
    UD_SetMenuIcon(ID_Analyze         , glass )
    UD_SetMenuIcon(ID_Defrag          , defrag)
    UD_SetMenuIcon(ID_QuickOpt        , quick )
    UD_SetMenuIcon(ID_FullOpt         , full  )
    UD_SetMenuIcon(ID_MftOpt          , mft   )
    UD_SetMenuIcon(ID_Stop            , stop  )
    UD_SetMenuIcon(ID_ShowReport      , report)
    UD_SetMenuIcon(ID_GuiOptions      , gear  )
    UD_SetMenuIcon(ID_BootScript      , script)
    UD_SetMenuIcon(ID_HelpContents    , help  )
    UD_SetMenuIcon(ID_HelpBestPractice, light )
    UD_SetMenuIcon(ID_HelpAbout       , star  )

    wxMenuItemList list; size_t count, index;
    UD_SetMarginWidth(m_menuBar->GetMenu(0))
    UD_SetMarginWidth(m_menuBar->GetMenu(1))
    UD_SetMarginWidth(m_menuBar->GetMenu(3))
    UD_SetMarginWidth(menuGUIconfig)
    UD_SetMarginWidth(menuBootConfig)

    // initial settings
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    m_menuBar->FindItem(ID_Repeat)->Check(m_repeat);

    cfg->Read(wxT("/Algorithm/SkipRemovableMedia"),&m_skipRem,true);
    m_menuBar->FindItem(ID_SkipRem)->Check(m_skipRem);

    int id = g_locale->GetLanguage();
    wxMenuItem *item = m_menuBar->FindItem(ID_LocaleChange + id);
    if(item) item->Check(true);
}

#undef UD_SetMarginWidth
#undef UD_SetMenuIcon

/** @} */
