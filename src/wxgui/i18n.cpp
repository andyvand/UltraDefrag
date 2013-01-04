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

typedef struct _Lang {
    wxString Name;
    int Id;
} Lang;

Lang Languages[] = {
    {wxT("German"), wxLANGUAGE_GERMAN},
    {wxEmptyString, 0}
};

// =======================================================================
//                         Internationalization
// =======================================================================

void MainFrame::SetLocale()
{
    m_Locale = new wxLocale();
    wxFileConfig *cfg = new wxFileConfig(wxT(""),wxT(""),
        wxT("lang.ini"),wxT(""),wxCONFIG_USE_RELATIVE_PATH);
    cfg->SetRecordDefaults(true);
    wxString lang = cfg->Read(wxT("/Language/Selected"),wxT("English (US)"));
    delete cfg;
    int id = wxLANGUAGE_ENGLISH_US;
    for(int i = 0; !Languages[i].Name.IsEmpty(); i++){
        if(lang.CmpNoCase(Languages[i].Name) == 0){
            id = Languages[i].Id;
            break;
        }
    }
    m_Locale->Init(id,wxLOCALE_CONV_ENCODING);
    m_Locale->AddCatalogLookupPathPrefix(wxT("i18n"));
    m_Locale->AddCatalog(wxT("UltraDefrag"));
}

/** @} */
