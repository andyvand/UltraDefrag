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

// macros to add missing languages copied from wxWidgets sources
#if !defined(__WIN32__) || defined(__WXMICROWIN__)
#define SETWINLANG(info,lang,sublang)
#else
#define SETWINLANG(info,lang,sublang) \
    info.WinLang = lang, info.WinSublang = sublang;
#endif // __WIN32__

#define LNG(wxlang, canonical, winlang, winsublang, layout, desc) \
    info.Language = wxlang;                               \
    info.CanonicalName = wxT(canonical);                  \
    info.LayoutDirection = layout;                        \
    info.Description = wxT(desc);                         \
    SETWINLANG(info, winlang, winsublang)                 \
    m_Locale->AddLanguage(info);

// user defined language IDs
#define wxUD_LANGUAGE_BOSNIAN_LATIN     wxLANGUAGE_USER_DEFINED+1
#define wxUD_LANGUAGE_BURMESE_PADAUK    wxLANGUAGE_USER_DEFINED+2
#define wxUD_LANGUAGE_ILOKO             wxLANGUAGE_USER_DEFINED+3
#define wxUD_LANGUAGE_INDONESIAN_BAHASA wxLANGUAGE_USER_DEFINED+4
#define wxUD_LANGUAGE_KAPAMPANGAN       wxLANGUAGE_USER_DEFINED+5
#define wxUD_LANGUAGE_WARAY_WARAY       wxLANGUAGE_USER_DEFINED+6

void MainFrame::SetLocale()
{
    int id = wxLANGUAGE_ENGLISH_US;
    wxLanguageInfo info;

    m_Locale = new wxLocale();
    wxConfigBase *cfg = wxConfigBase::Get();

    // add languages missing from wxWidgets
   LNG(wxUD_LANGUAGE_BOSNIAN_LATIN,     "bs"   , LANG_BOSNIAN   , SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN, wxLayout_LeftToRight, "Bosnian")
   LNG(wxUD_LANGUAGE_BURMESE_PADAUK,    "my_PA", 0              , 0              , wxLayout_LeftToRight, "Burmese (Padauk)")
   LNG(wxUD_LANGUAGE_ILOKO,             "il_PH", 0              , 0              , wxLayout_LeftToRight, "Iloko")
   LNG(wxUD_LANGUAGE_INDONESIAN_BAHASA, "id_BI", LANG_INDONESIAN, SUBLANG_DEFAULT, wxLayout_LeftToRight, "Indonesian (Bahasa Indonesia)")
   LNG(wxUD_LANGUAGE_KAPAMPANGAN,       "pa_PH", 0              , 0              , wxLayout_LeftToRight, "Kapampangan")
   LNG(wxUD_LANGUAGE_WARAY_WARAY,       "wa_PH", 0              , 0              , wxLayout_LeftToRight, "Waray-Waray")

    // get initial language selection
    if(cfg->HasGroup(wxT("Language"))){
        id = (int)cfg->Read(wxT("/Language/Selected"),id);
    }else{
        id = m_Locale->GetSystemLanguage();

        if(id == wxLANGUAGE_UNKNOWN)
            id = wxLANGUAGE_ENGLISH_US;
    }
    if(!m_Locale->IsAvailable(id))
        id = wxLANGUAGE_ENGLISH_US;

    // apply language selection
    m_Locale->Init(id,wxLOCALE_CONV_ENCODING);
    m_Locale->AddCatalogLookupPathPrefix(wxT("i18n"));
    m_Locale->AddCatalog(wxT("UltraDefrag"));
}

#undef LNG

/** @} */
