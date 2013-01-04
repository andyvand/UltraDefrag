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

void MainFrame::SetLocale()
{
    int id = wxLANGUAGE_ENGLISH_US;
    m_Locale = new wxLocale();
    wxConfigBase *cfg = wxConfigBase::Get();

    if(cfg->HasGroup(wxT("Language"))){
        id = (int)cfg->Read(wxT("/Language/Selected"),id);
    }else{
        id = m_Locale->GetSystemLanguage();

        if(id == wxLANGUAGE_UNKNOWN)
            id = wxLANGUAGE_ENGLISH_US;
    }
    if(!m_Locale->IsAvailable(id))
        id = wxLANGUAGE_ENGLISH_US;

    m_Locale->Init(id,wxLOCALE_CONV_ENCODING);
    m_Locale->AddCatalogLookupPathPrefix(wxT("i18n"));
    m_Locale->AddCatalog(wxT("UltraDefrag"));
}

/** @} */
