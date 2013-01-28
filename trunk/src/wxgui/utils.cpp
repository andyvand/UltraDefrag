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

typedef HRESULT (__stdcall *URLMON_PROCEDURE)(
    /* LPUNKNOWN */ void *lpUnkcaller,
    LPCWSTR szURL,
    LPWSTR szFileName,
    DWORD cchFileName,
    DWORD dwReserved,
    /*IBindStatusCallback*/ void *pBSC
);

// =======================================================================
//                         Auxiliary utilities
// =======================================================================

/**
 * @brief Defines whether the user
 * has administrative rights or not.
 * @details Based on the UserInfo
 * NSIS plug-in.
 */
bool Utils::CheckAdminRights(void)
{
    HANDLE hToken;
    if(!OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&hToken)){
        letrace("cannot open access token of the thread");
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken)){
            letrace("cannot open access token of the process");
            return false;
        }
    }

    PSID psid = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = {SECURITY_NT_AUTHORITY};
    if(!AllocateAndInitializeSid(&SystemSidAuthority,2,
      SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
      0,0,0,0,0,0,&psid)){
        letrace("cannot create the security identifier");
        CloseHandle(hToken);
        return false;
    }

    BOOL is_member = false;
    if(!CheckTokenMembership(NULL,psid,&is_member)){
        letrace("cannot check token membership");
        if(psid) FreeSid(psid); CloseHandle(hToken);
        return false;
    }

    if(!is_member) itrace("the user is not a member of administrators group");
    if(psid) FreeSid(psid); CloseHandle(hToken);
    return (is_member == 0) ? false : true;
}

/**
 * @brief Downloads a file from the web.
 * @return Path to the downloaded file.
 * @note If the program terminates before
 * the file download completion it crashes.
 */
wxString Utils::DownloadFile(const wxString& url)
{
    wxLogMessage(wxT("downloading %ls"),url.wc_str());

    wxDynamicLibrary lib(wxT("urlmon"));
    wxDYNLIB_FUNCTION(URLMON_PROCEDURE,
        URLDownloadToCacheFileW, lib);

    if(!pfnURLDownloadToCacheFileW)
        return wxEmptyString;

    wchar_t buffer[MAX_PATH + 1];
    HRESULT result = pfnURLDownloadToCacheFileW(NULL,url.wc_str(),buffer,MAX_PATH,0,NULL);
    if(result != S_OK){
        wxLogError(wxT("URLDownloadToCacheFile failed with code 0x%x"),(UINT)result);
        return wxEmptyString;
    }

    buffer[MAX_PATH] = 0;
    wxString path(buffer);
    return path;
}

/**
 * @brief Sends a request to Google Analytics
 * service gathering statistics of the use
 * of the program.
 * @details Based on http://code.google.com/apis/analytics/docs/
 * and http://www.vdgraaf.info/google-analytics-without-javascript.html
 */
void Utils::GaRequest(const wxString& path)
{
    srand((unsigned int)time(NULL));
    int utmn = (rand() << 16) + rand();
    int utmhid = (rand() << 16) + rand();
    int cookie = (rand() << 16) + rand();
    int random = (rand() << 16) + rand();
    __int64 today = (__int64)time(NULL);

    wxString request;

    request << wxT("http://www.google-analytics.com/__utm.gif?utmwv=4.6.5");
    request << wxString::Format(wxT("&utmn=%u"),utmn);
    request << wxT("&utmhn=ultradefrag.sourceforge.net");
    request << wxString::Format(wxT("&utmhid=%u&utmr=-"),utmhid);
    request << wxT("&utmp=") << path;
    request << wxT("&utmac=");
    request << wxT("UA-15890458-1");
    request << wxString::Format(wxT("&utmcc=__utma%%3D%u.%u.%I64u.%I64u.%I64u.") \
        wxT("50%%3B%%2B__utmz%%3D%u.%I64u.27.2.utmcsr%%3Dgoogle.com%%7Cutmccn%%3D") \
        wxT("(referral)%%7Cutmcmd%%3Dreferral%%7Cutmcct%%3D%%2F%3B"),
        cookie,random,today,today,today,cookie,today);

    wxString url(request);
    wxString file = DownloadFile(url);
    if(!file.IsEmpty())
        wxRemoveFile(file);
}

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
                file.AddLine(wxT("Redirecting... if the page has not been redirected automatically click "));
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
 * @brief wxMessageDialog analog,
 * but with custom button labels
 * and with ability to center dialog
 * over the parent window.
 * @param[in] parent the parent window.
 * @param[in] caption the dialog caption.
 * @param[in] icon one of the wxART_xxx constants.
 * @param[in] text1 the OK button label.
 * @param[in] text2 the Cancel button label.
 * @param[in] format the format specification.
 * @param[in] ... the parameters.
 */
int Utils::MessageDialog(wxFrame *parent,
    const wxString& caption, const wxArtID& icon,
    const wxString& text1, const wxString& text2,
    const wxChar* format, ...)
{
    wxString message;
    va_list args;
    va_start(args,format);
    message.PrintfV(format,args);
    va_end(args);

    wxDialog dlg(parent,wxID_ANY,caption);

    wxStaticBitmap *pic = new wxStaticBitmap(&dlg,wxID_ANY,
        wxArtProvider::GetIcon(icon,wxART_MESSAGE_BOX));
    wxStaticText *msg = new wxStaticText(&dlg,wxID_ANY,message,
        wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE);

    wxGridBagSizer* contents = new wxGridBagSizer(0, 0);

    contents->Add(pic, wxGBPosition(0, 0), wxDefaultSpan,
        (wxBOTTOM)|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 10);
    contents->Add(msg, wxGBPosition(0, 1), wxDefaultSpan,
        (wxALL & ~wxTOP)|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 10);

    wxButton *ok = new wxButton(&dlg,wxID_OK,text1);
    wxButton *cancel = new wxButton(&dlg,wxID_CANCEL,text2);

    wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
    buttons->Add(ok,wxSizerFlags(1).Border(wxRIGHT));
    buttons->Add(cancel,wxSizerFlags(1).Border(wxLEFT));

    contents->Add(buttons, wxGBPosition(1, 0), wxGBSpan(1, 2),
        wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL);

    wxBoxSizer *space = new wxBoxSizer(wxHORIZONTAL);
    space->Add(contents,wxSizerFlags().DoubleBorder());

    dlg.SetSizerAndFit(space);
    space->SetSizeHints(&dlg);

    if(!parent->IsIconized()) dlg.Center();
    else dlg.CenterOnScreen();

    return dlg.ShowModal();
}

/**
 * @brief Shows an error and
 * invites to open log file.
 */
void Utils::ShowError(const wxChar* format, ...)
{
    wxString message;
    va_list args;
    va_start(args,format);
    message.PrintfV(format,args);
    va_end(args);

    wxString log = _("Open &log");
    log.Replace(wxT("&"),wxT(""));

    if(MessageDialog(g_MainFrame,_("Error!"),
      wxART_ERROR,log,_("&Cancel"),message) == wxID_OK)
    {
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED,ID_DebugLog);
        wxPostEvent(g_MainFrame,event);
    }
}

/** @} */
