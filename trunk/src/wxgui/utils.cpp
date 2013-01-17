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
 * @file utils.cpp
 * @brief Auxiliary utilities.
 * @addtogroup Utils
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                         Auxiliary utilities
// =======================================================================

/**
 * @brief Creates a bitmap
 * from a png resource.
 */
wxBitmap * Utils::LoadPngResource(const wchar_t *name)
{
    HRSRC resource = ::FindResource(NULL,name,RT_RCDATA);
    if(!resource){
        wxLogSysError(wxT("cannot find %ls resource"),name);
        return NULL;
    }

    HGLOBAL handle = ::LoadResource(NULL,resource);
    if(!handle){
        wxLogSysError(wxT("cannot load %ls resource"),name);
        return NULL;
    }

    char *data = (char *)::LockResource(handle);
    if(!data){
        wxLogSysError(wxT("cannot lock %ls resource"),name);
        return NULL;
    }

    DWORD size = ::SizeofResource(NULL,resource);
    if(!size){
        wxLogSysError(wxT("cannot get size of %ls resource"),name);
        return NULL;
    }

    wxMemoryInputStream is(data,size);
    return new wxBitmap(wxImage(is,wxBITMAP_TYPE_PNG,-1),-1);
}

/**
 * @brief Opens the UltraDefrag Handbook,
 * either its local copy or from the web.
 */
void Utils::OpenHandbook(const wxString& page, const wxString& anchor)
{
    wxString path; wxURI url;
    path = wxT("./handbook/") + page;

    if(wxFileExists(path)){
        path = wxT("file:////") + wxGetCwd();
        path.Replace(wxT("\\"),wxT("/"));
        if(!anchor.IsEmpty()){
            /*
            * wxLaunchDefaultBrowser
            * is unable to open a local
            * page with anchor appended.
            * So, we're making a redirector
            * and opening it instead.
            */
            wxString redirector(wxT("./handbook/"));
            redirector << page << wxT(".") << anchor << wxT(".html");
            if(!wxFileExists(redirector)){
                wxTextFile file;
                file.Create(redirector);
                file.AddLine(wxT("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">"));
                file.AddLine(wxT("<html><head><meta http-equiv=\"Refresh\" content=\"0; URL=") \
                    + page + wxT("#") + anchor + wxT("\">"));
                file.AddLine(wxT("</head><body>"));
                file.AddLine(wxT("If the page has not been redirected automatically click "));
                file.AddLine(wxT("<a href=\"") + page + wxT("#") + anchor + wxT("\">here</a>."));
                file.AddLine(wxT("</body></html>"));
                file.Write();
                file.Close();
            }
            path << wxT("/") << redirector;
        } else {
            path << wxT("/handbook/") << page;
        }
    } else {
        path = wxT("http://ultradefrag.sourceforge.net");
        path << wxT("/handbook/") << page;
        if(!anchor.IsEmpty())
            path << wxT("#") << anchor;
    }

    url.Create(path);
    path = url.BuildURI();

    wxLogMessage(wxT("%ls"),path.wc_str());
    if(!wxLaunchDefaultBrowser(path))
        ShowError(wxT("Cannot open %ls!"),path.wc_str());
}

/**
 * @brief Shows an error and
 * invites to open the log file.
 */
void Utils::ShowError(const wxChar* format, ...)
{
    va_list args;
    va_start(args,format);
    wxString message;
    message.PrintfV(format,args);
    va_end(args);

    wxDialog dlg(g_MainFrame,wxID_ANY,_("Error!"));

    wxStaticBitmap *icon = new wxStaticBitmap(&dlg,wxID_ANY,
        wxArtProvider::GetIcon(wxART_ERROR,wxART_MESSAGE_BOX));
    wxStaticText *msg = new wxStaticText(&dlg,wxID_ANY,message,
        wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE);

    wxGridBagSizer* contents = new wxGridBagSizer(0, 0);

    contents->Add(icon, wxGBPosition(0, 0), wxDefaultSpan,
        wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
    contents->Add(msg, wxGBPosition(0, 1), wxDefaultSpan,
        wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);

    wxString label = _("Open &log");
    label.Replace(wxT("&"),wxT(""));
    wxButton *log = new wxButton(&dlg,wxID_OK,label);
    wxButton *cancel = new wxButton(&dlg,wxID_CANCEL,_("Cancel"));

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(log,wxSizerFlags(1).Border());
    buttons->Add(cancel,wxSizerFlags(1).Border());

    contents->Add(buttons, wxGBPosition(1, 0), wxGBSpan(1, 2),
        wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
    dlg.SetSizerAndFit(contents);
    contents->SetSizeHints(&dlg);
    dlg.Center();

    if(dlg.ShowModal() == wxID_OK){
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_DebugLog);
        wxPostEvent(g_MainFrame,event);
    }
}

/** @} */
