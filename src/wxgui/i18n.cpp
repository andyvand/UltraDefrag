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
 * @file i18n.cpp
 * @brief Internationalization.
 * @addtogroup Internationalization
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                         Internationalization
// =======================================================================

// macro to add missing languages copied from wxWidgets sources
#define UD_LNG(wxlang, canonical, winlang, winsublang, layout, desc) \
    info.Language = wxlang;                               \
    info.CanonicalName = wxT(canonical);                  \
    info.LayoutDirection = layout;                        \
    info.Description = wxT(desc);                         \
    info.WinLang = winlang, info.WinSublang = winsublang; \
    m_locale->AddLanguage(info);

#define UD_UpdateMenuItemLabel(id,label,accel) \
    if(::strlen(accel)){ \
        ItemLabel = _(label); \
        ItemLabel << wxT("\t") << wxT(accel); \
        m_menuBar->FindItem(id)->SetItemLabel(ItemLabel); \
        if(m_toolBar->FindById(id)){ \
            ItemLabel = _(label); \
            ItemLabel << wxT(" (") << wxT(accel) << wxT(")"); \
            m_toolBar->SetToolShortHelp(id,ItemLabel); \
        } \
    } else { \
        m_menuBar->FindItem(id)->SetItemLabel(_(label)); \
        if(m_toolBar->FindById(id)) \
            m_toolBar->SetToolShortHelp(id,_(label)); \
    }

void MainFrame::InitLocale()
{
    m_locale = new wxLocale();

    // add translations missing from wxWidgets
    wxLanguageInfo info;
    UD_LNG(wxUD_LANGUAGE_BOSNIAN,           "bs"          , LANG_BOSNIAN   , SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN, wxLayout_LeftToRight, "Bosnian")
    UD_LNG(wxUD_LANGUAGE_ILOKO,             "ilo"         , 0              , 0              , wxLayout_LeftToRight, "Iloko")
    UD_LNG(wxUD_LANGUAGE_INDONESIAN_BAHASA, "id_ID@bahasa", LANG_INDONESIAN, SUBLANG_DEFAULT, wxLayout_LeftToRight, "Indonesian (Bahasa Indonesia)")
    UD_LNG(wxUD_LANGUAGE_KAPAMPANGAN,       "pa_PH"       , 0              , 0              , wxLayout_LeftToRight, "Kapampangan")
    UD_LNG(wxUD_LANGUAGE_NORWEGIAN,         "no"          , LANG_NORWEGIAN , SUBLANG_DEFAULT, wxLayout_LeftToRight, "Norwegian")
    UD_LNG(wxUD_LANGUAGE_WARAY_WARAY,       "wa_PH"       , 0              , 0              , wxLayout_LeftToRight, "Waray-Waray")

    // get initial language selection
    int id = wxLANGUAGE_ENGLISH_US;
    wxConfigBase *cfg = wxConfigBase::Get();
    if(cfg->HasGroup(wxT("Language"))){
        id = (int)cfg->Read(wxT("/Language/Selected"),id);
    } else {
        id = m_locale->GetSystemLanguage();
        if(id == wxLANGUAGE_UNKNOWN)
            id = wxLANGUAGE_ENGLISH_US;
    }

    SetLocale(id);
}

void MainFrame::SetLocale(int id)
{
    // apply language selection
    m_locale->Init(id,wxLOCALE_CONV_ENCODING);
    m_locale->AddCatalogLookupPathPrefix(wxT("locale"));

    // locations for development
    m_locale->AddCatalogLookupPathPrefix(wxT("../wxgui/locale"));
    m_locale->AddCatalogLookupPathPrefix(wxT("../../wxgui/locale"));

    m_locale->AddCatalog(wxT("UltraDefrag"));
}

void MainFrame::OnLocaleChange(wxCommandEvent& event)
{
    SetLocale(event.GetId() - ID_LocaleChange);

    // update menu labels and tool bar tool-tips
    wxString ItemLabel;
    // main menu
    m_menuBar->SetMenuLabel(0, _("&Action"));
    m_menuBar->SetMenuLabel(1, _("&Report"));
    m_menuBar->SetMenuLabel(2, _("&Settings"));
    m_menuBar->SetMenuLabel(3, _("&Help"));
    // action menu
    UD_UpdateMenuItemLabel(ID_Analyze          , "&Analyze"             , "F5");
    UD_UpdateMenuItemLabel(ID_Defrag           , "&Defragment"          , "F6");
    UD_UpdateMenuItemLabel(ID_QuickOpt         , "&Quick optimization"  , "F7");
    UD_UpdateMenuItemLabel(ID_FullOpt          , "&Full optimization"   , "Ctrl+F7");
    UD_UpdateMenuItemLabel(ID_MftOpt           , "&Optimize MFT"        , "Shift+F7");
    UD_UpdateMenuItemLabel(ID_Pause            , "Pa&use"               , "Space");
    UD_UpdateMenuItemLabel(ID_Stop             , "&Stop"                , "Ctrl+C");
    UD_UpdateMenuItemLabel(ID_Repeat           , "Re&peat action"       , "Shift+R");
    UD_UpdateMenuItemLabel(ID_SkipRem          , "Skip removable &media", "Ctrl+M");
    UD_UpdateMenuItemLabel(ID_Rescan           , "&Rescan drives"       , "Ctrl+D");
    UD_UpdateMenuItemLabel(ID_Repair           , "Repair dri&ves"       , "");
    m_subMenuWhenDone->SetItemLabel(_("&When done"));
    UD_UpdateMenuItemLabel(ID_Exit             , "E&xit"                , "Alt+F4");
    // when done sub-menu
    UD_UpdateMenuItemLabel(ID_WhenDoneNone     , "&None"                , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneExit     , "E&xit"                , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneStandby  , "Stan&dby"             , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneHibernate, "&Hibernate"           , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneLogoff   , "&Logoff"              , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneReboot   , "&Reboot"              , "");
    UD_UpdateMenuItemLabel(ID_WhenDoneShutdown , "&Shutdown"            , "");
    // report menu
    UD_UpdateMenuItemLabel(ID_ShowReport       , "&Show report"         , "F8");
    // settings menu
    m_subMenuLanguage->SetItemLabel(_("&Language"));
    m_subMenuGUIconfig->SetItemLabel(_("&Graphical interface"));
    m_subMenuBootConfig->SetItemLabel(_("&Boot time scan"));
    UD_UpdateMenuItemLabel(ID_ReportOptions    , "&Reports"             , "Ctrl+R");
    // language sub-menu
    UD_UpdateMenuItemLabel(ID_LangShowLog      , "&View change log"     , "");
    UD_UpdateMenuItemLabel(ID_LangShowReport   , "View translation &report", "");
    UD_UpdateMenuItemLabel(ID_LangOpenFolder   , "&Translations folder"    , "");
    UD_UpdateMenuItemLabel(ID_LangSubmit       , "&Submit current translation", "");
    // graphical interface sub-menu
    UD_UpdateMenuItemLabel(ID_GuiFont          , "&Font"                , "F9");
    UD_UpdateMenuItemLabel(ID_GuiOptions       , "&Options"             , "F10");
    // boot time scan sub-menu
    UD_UpdateMenuItemLabel(ID_BootEnable       , "&Enable"              , "F11");
    UD_UpdateMenuItemLabel(ID_BootScript       , "&Script"              , "F12");
    // help menu
    UD_UpdateMenuItemLabel(ID_HelpContents     , "&Contents"            , "F1");
    UD_UpdateMenuItemLabel(ID_HelpBestPractice , "Best &practice"       , "F2");
    UD_UpdateMenuItemLabel(ID_HelpFaq          , "&Frequently asked questions", "F3");
    UD_UpdateMenuItemLabel(ID_HelpLegend       , "Cluster map &legend"  , "");
    m_subMenuDebug->SetItemLabel(_("&Debug"));
    UD_UpdateMenuItemLabel(ID_HelpUpdate       , "Check for &update"    , "");
    UD_UpdateMenuItemLabel(ID_HelpAbout        , "&About"               , "F4");
    // debug sub-menu
    UD_UpdateMenuItemLabel(ID_DebugLog         , "Open &log"            , "Alt+L");
    UD_UpdateMenuItemLabel(ID_DebugSend        , "Send bug &report"     , "");

    // update tool-tips that differ from menu labels
    ItemLabel = _("&Boot time scan"); ItemLabel << wxT(" (F11)");
    m_toolBar->SetToolShortHelp(ID_BootEnable,ItemLabel);
    ItemLabel = _("Boot time script"); ItemLabel << wxT(" (F12)");
    m_toolBar->SetToolShortHelp(ID_BootScript,ItemLabel);
    ItemLabel = _("&Help"); ItemLabel << wxT(" (F1)");
    m_toolBar->SetToolShortHelp(ID_HelpContents,ItemLabel);
}

#undef UD_UpdateMenuItemLabel
#undef UD_LNG

/** @} */
