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

enum {
    UPGRADE_NONE = 0,
    UPGRADE_STABLE,
    UPGRADE_ALL
};

#define VERSION_URL "http://ultradefrag.sourceforge.net/version.ini"
#define STABLE_VERSION_URL "http://ultradefrag.sourceforge.net/stable-version.ini"

// =======================================================================
//                          Upgrade handling
// =======================================================================

void *UpgradeThread::Entry()
{
    while(!m_stop){
        if(m_check && m_level){
            wxString path = Utils::DownloadFile(
                m_level == UPGRADE_ALL ? wxT(VERSION_URL) :
                wxT(STABLE_VERSION_URL)
            );
            if(!path.IsEmpty()){
                wxTextFile file; file.Open(path);
                wxString lv = file.GetFirstLine();
                lv.Trim(true); lv.Trim(false);
                int last = ParseVersionString(lv.char_str());

                const char *cv = VERSIONINTITLE;
                int current = ParseVersionString(&cv[12]);

                wxLogMessage(wxT("last version   : UltraDefrag %ls"),lv.wc_str());
                wxLogMessage(wxT("current version: %hs"),cv);

                if(last && current && last > current){
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

/**
 * @brief Parses a version string and generates an integer for comparison.
 * @return An integer representing the version. Zero indicates that
 * the version string parsing failed.
 */
int UpgradeThread::ParseVersionString(const char *version)
{
    char *string = _strdup(version);
    if(!string) return 0;

    _strlwr(string);

    // version numbers (major, minor, revision, unstable version)
    int mj, mn, rev, uv;

    int res = sscanf(string,"%u.%u.%u alpha%u",&mj,&mn,&rev,&uv);
    if(res == 4){
        uv += 100;
    } else {
        res = sscanf(string,"%u.%u.%u beta%u",&mj,&mn,&rev,&uv);
        if(res == 4){
            uv += 200;
        } else {
            res = sscanf(string,"%u.%u.%u rc%u",&mj,&mn,&rev,&uv);
            if(res == 4){
                uv += 300;
            } else {
                res = sscanf(string,"%u.%u.%u",&mj,&mn,&rev);
                if(res == 3){
                    uv = 999;
                } else {
                    etrace("parsing of '%hs' failed",version);
                    return 0;
                }
            }
        }
    }

    free(string);

    /* 5.0.0 > 4.99.99 rc10*/
    return mj * 10000000 + mn * 100000 + rev * 1000 + uv;
}

// =======================================================================
//                           Event handlers
// =======================================================================

void MainFrame::OnHelpUpdate(wxCommandEvent& event)
{
    switch(event.GetId()){
        case ID_HelpUpdateNone:
            // disable checks
            m_upgradeThread->m_level = 0;
            break;
        case ID_HelpUpdateStable:
        case ID_HelpUpdateAll:
            // enable the check...
            m_upgradeThread->m_level = event.GetId() - ID_HelpUpdateNone;
            // ...and check it now
        case ID_HelpUpdateCheck:
            m_upgradeThread->m_check = true;
    }
}

void MainFrame::OnShowUpgradeDialog(wxCommandEvent& event)
{
    wxString message = wxString();
    message.Printf(_("Release %s is available for download!"),event.GetString());

    if(Utils::MessageDialog(this,_("You can upgrade me ^-^"),
      wxART_INFORMATION,_("&Upgrade"),_("&Cancel"),message) == wxID_OK)
    {
        wxString url(wxT("http://ultradefrag.sourceforge.net"));
        if(!wxLaunchDefaultBrowser(url))
            Utils::ShowError(wxT("Cannot open %ls!"),url.wc_str());
    }
}

/** @} */
