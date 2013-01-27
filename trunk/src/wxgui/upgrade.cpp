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
 * @file upgrade.cpp
 * @brief Upgrade.
 * @addtogroup Upgrade
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#define VERSION_URL "http://ultradefrag.sourceforge.net/version.ini"

// =======================================================================
//                          Upgrade handling
// =======================================================================

void *UpgradeThread::Entry()
{
    while(!m_stop){
        if(m_check && m_level > 0){
            wxString path = Utils::DownloadFile(wxT(VERSION_URL));
            if(!path.IsEmpty()){
                wxTextFile file; file.Open(path);
                wxString lv = file.GetFirstLine();
                lv.Trim(true); lv.Trim(false);
                wxLogMessage(wxT("last version: %ls"),lv.wc_str());
                int lmj, lmn, li; // latest version numbers
                swscanf(lv.wc_str(),L"%u.%u.%u",&lmj,&lmn,&li);
                // lmj = 8; /* for testing */

                wxString cv(wxT(VERSIONINTITLE));
                wxLogMessage(wxT("current version: %ls"),cv.wc_str());
                int cmj, cmn, ci; // current version numbers
                swscanf(cv.wc_str(),L"UltraDefrag %u.%u.%u",&cmj,&cmn,&ci);
                bool unstable = false;
                if(cv.Lower().Find(wxT("alpha")) != wxNOT_FOUND) unstable = true;
                if(cv.Lower().Find(wxT("beta")) != wxNOT_FOUND) unstable = true;
                if(cv.Lower().Find(wxT("rc")) != wxNOT_FOUND) unstable = true;

                /* 5.0.0 > 4.99.99 */
                int current = cmj * 10000 + cmn * 100 + ci;
                int last = lmj * 10000 + lmn * 100 + li;
                bool upgrade = (last > current);
                if(last == current && unstable) upgrade = true;

                if(upgrade){
                    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_ShowUpgradeDialog);
                    event.SetString(lv);
                    wxPostEvent(g_MainFrame,event);
                }

                // remove file from cache
                file.Close(); wxRemoveFile(path);
            }
            m_check = false;
        }
        Sleep(300);
    }

    return NULL;
}

// =======================================================================
//                           Event handlers
// =======================================================================

void MainFrame::OnHelpUpdate(wxCommandEvent& event)
{
    switch(event.GetId()){
        case ID_HelpUpdateNone:
        case ID_HelpUpdateStable:
        case ID_HelpUpdateAll:
            m_upgradeThread->m_level = event.GetId() - ID_HelpUpdateNone;
            break;
        case ID_HelpUpdateCheck:
            m_upgradeThread->m_check = true;
    }
}

void MainFrame::OnShowUpgradeDialog(wxCommandEvent& event)
{
    wxString message(event.GetString());
    message << wxT(" ") << _("release is available for download!");

    if(Utils::MessageDialog(this,_("You can upgrade me ^-^"),
      wxART_INFORMATION,_("&Upgrade"),_("&Cancel"),message) == wxID_OK)
    {
        wxString url(wxT("http://ultradefrag.sourceforge.net"));
        if(!wxLaunchDefaultBrowser(url))
            Utils::ShowError(wxT("Cannot open %ls!"),url.wc_str());
    }
}

/** @} */
