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
 * @file about.cpp
 * @brief About box.
 * @addtogroup AboutBox
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

// =======================================================================
//                         Custom about dialog
// =======================================================================

AboutDialog::AboutDialog()
{
    if(!wxDialog::Create(g_MainFrame,wxID_ANY,
        _("About UltraDefrag"))) return;

    wxStaticText *version = new wxStaticText(this,wxID_ANY,wxT(VERSIONINTITLE));
    wxFont fontBig(*wxNORMAL_FONT);
    fontBig.SetPointSize(fontBig.GetPointSize() + 2);
    fontBig.SetWeight(wxFONTWEIGHT_BOLD);
    version->SetFont(fontBig);

    wxStaticText *copyright = new wxStaticText(this,wxID_ANY,
        wxT("(C) 2007-2013 UltraDefrag development team"));
    wxStaticText *description = new wxStaticText(this,wxID_ANY,
        _("An open source defragmentation utility."));
    wxStaticText *credits = new wxStaticText(this,wxID_ANY,
        _("Credits and licenses are listed in the handbook."));

    wxHyperlinkCtrl *homepage = new wxHyperlinkCtrl(this,wxID_ANY,
        _("UltraDefrag website"),wxT("http://ultradefrag.sourceforge.net"));

    wxSizerFlags flags;
    flags.Center().Border(wxLEFT & wxRIGHT);
    int space = DPI(11);
    
    wxSizer *text = new wxBoxSizer(wxVERTICAL);
    text->AddSpacer(space);
    text->Add(version,flags);
    text->AddSpacer(space);
    text->Add(copyright,flags);
    text->AddSpacer(space);
    text->Add(description,flags);
    text->Add(credits,flags);
    text->AddSpacer(space);
    text->Add(homepage,flags);
    text->AddSpacer(space);
    
    wxDisplay display;
    wxStaticBitmap *bmp = new wxStaticBitmap(this,wxID_ANY,
        display.GetCurrentMode().GetDepth() > 8 ? \
        wxBITMAP(ship) : wxBITMAP(ship_8bit));

    wxSizer *contents = new wxBoxSizer(wxHORIZONTAL);
    contents->Add(bmp,wxSizerFlags().Center().Border());
    contents->Add(text,wxSizerFlags(1).Center().Border());
    SetSizerAndFit(contents);
    Center();
}

/** @} */
