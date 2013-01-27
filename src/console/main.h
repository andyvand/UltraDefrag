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

#ifndef _UDEFRAG_CONSOLE_MAIN_H_
#define _UDEFRAG_CONSOLE_MAIN_H_

// =======================================================================
//                               Headers
// =======================================================================

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/cmdline.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/thread.h>

#include <conio.h>

#if wxUSE_UNICODE
#define wxCharStringFmtSpec "%ls"
#else
#define wxCharStringFmtSpec "%hs"
#endif

#include "../include/version.h"
#include "../dll/zenwinx/zenwinx.h"
#include "../dll/udefrag/udefrag.h"

// =======================================================================
//                          Macro definitions
// =======================================================================

/* sets text color only if -b option is not set */
#define color(c) { if(!g_use_default_colors) (void)SetConsoleTextAttribute(g_out,c); }

/* sets text color regardless of -b option */
#define force_color(c) { (void)SetConsoleTextAttribute(g_out,c); }

// =======================================================================
//                            Declarations
// =======================================================================

class Log: public wxLog {
public:
    Log();
    ~Log();

    virtual void DoLog(wxLogLevel level,
        const wxChar *msg,time_t timestamp);
};

bool check_admin_rights(void);
bool parse_cmdline(int argc, char **argv);
void show_help(void);
void init_map(char letter);
void redraw_map(udefrag_progress_info *pi);
void destroy_map(void);
void clear_line(void);
void print_unicode(wchar_t *string);
void ga_request(const wxString& path);

// =======================================================================
//                           Global variables
// =======================================================================

extern bool g_analyze;
extern bool g_optimize;
extern bool g_quick_optimization;
extern bool g_optimize_mft;
extern bool g_all;
extern bool g_all_fixed;
extern bool g_list_volumes;
extern bool g_list_all;
extern bool g_repeat;
extern bool g_no_progress;
extern bool g_show_vol_info;
extern bool g_show_map;
extern bool g_use_default_colors;
extern bool g_use_entire_window;
extern bool g_help;
extern bool g_wait;
extern bool g_shellex;
extern bool g_folder;
extern bool g_folder_itself;

extern short g_map_border_color;
extern char g_map_symbol;
extern int g_map_rows;
extern int g_map_symbols_per_line;
extern int g_extra_lines;

extern wxArrayString *g_volumes;
extern wxArrayString *g_paths;

extern HANDLE g_out;
extern short g_default_color;

extern bool g_stop;

#endif /* _UDEFRAG_CONSOLE_MAIN_H_ */
