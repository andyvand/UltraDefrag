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
 * @file config.cpp
 * @brief Configuration.
 * @addtogroup Configuration
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

extern "C" {
#define lua_c
#include "../lua5.1/lua.h"
#include "../lua5.1/lauxlib.h"
#include "../lua5.1/lualib.h"
}

// =======================================================================
//                      Application configuration
// =======================================================================

/**
 * @brief Reads application configuration.
 * @note Intended to be called once somewhere
 * in the main frame constructor.
 */
void MainFrame::ReadAppConfiguration()
{
    wxConfigBase *cfg = wxConfigBase::Get();

    // get window size and position
    m_saved = cfg->HasGroup(wxT("MainFrame"));
    m_x = (int)cfg->Read(wxT("/MainFrame/x"),0l);
    m_y = (int)cfg->Read(wxT("/MainFrame/y"),0l);
    m_width = (int)cfg->Read(wxT("/MainFrame/width"),
        DPI(MAIN_WINDOW_DEFAULT_WIDTH));
    m_height = (int)cfg->Read(wxT("/MainFrame/height"),
        DPI(MAIN_WINDOW_DEFAULT_HEIGHT));

    // validate width and height
    wxDisplay display;
    if(m_width < DPI(MAIN_WINDOW_MIN_WIDTH)) m_width = DPI(MAIN_WINDOW_MIN_WIDTH);
    if(m_width > display.GetClientArea().width) m_width = DPI(MAIN_WINDOW_DEFAULT_WIDTH);
    if(m_height < DPI(MAIN_WINDOW_MIN_HEIGHT)) m_height = DPI(MAIN_WINDOW_MIN_HEIGHT);
    if(m_height > display.GetClientArea().height) m_height = DPI(MAIN_WINDOW_DEFAULT_HEIGHT);

    // validate x and y
    if(m_x < 0) m_x = 0; if(m_y < 0) m_y = 0;
    if(m_x > display.GetClientArea().width - 130)
        m_x = display.GetClientArea().width - 130;
    if(m_y > display.GetClientArea().height - 50)
        m_y = display.GetClientArea().height - 50;

    // now the window is surely inside of the screen

    cfg->Read(wxT("/MainFrame/maximized"),&m_maximized,false);

    m_separatorPosition = (int)cfg->Read(
        wxT("/MainFrame/SeparatorPosition"),
        (long)DPI(DEFAULT_LIST_HEIGHT)
    );

    cfg->Read(wxT("/DrivesList/width1"), &m_w1, 0.0);
    cfg->Read(wxT("/DrivesList/width2"), &m_w2, 0.0);
    cfg->Read(wxT("/DrivesList/width3"), &m_w3, 0.0);
    cfg->Read(wxT("/DrivesList/width4"), &m_w4, 0.0);
    cfg->Read(wxT("/DrivesList/width5"), &m_w5, 0.0);
    cfg->Read(wxT("/DrivesList/width6"), &m_w6, 0.0);

    if(!m_w1) m_w1 = 110; if(!m_w2) m_w2 = 110; if(!m_w3) m_w3 = 110;
    if(!m_w4) m_w4 = 110; if(!m_w5) m_w5 = 110; if(!m_w6) m_w6 = 65;

    cfg->Read(wxT("/Algorithm/RepeatAction"),&m_repeat,false);
    cfg->Read(wxT("/Algorithm/SkipRemovableMedia"),&m_skipRem,true);
}

/**
 * @brief Saves application configuration.
 * @note Intended to be called in the main
 * frame destructor before termination
 * of threads.
 */
void MainFrame::SaveAppConfiguration()
{
    wxConfigBase *cfg = wxConfigBase::Get();
    cfg->Write(wxT("/MainFrame/x"),(long)m_x);
    cfg->Write(wxT("/MainFrame/y"),(long)m_y);
    cfg->Write(wxT("/MainFrame/width"),(long)m_width);
    cfg->Write(wxT("/MainFrame/height"),(long)m_height);
    cfg->Write(wxT("/MainFrame/maximized"),(long)IsMaximized());
    cfg->Write(wxT("/MainFrame/SeparatorPosition"),
        (long)m_splitter->GetSashPosition());

    cfg->Write(wxT("/DrivesList/width1"),m_vList->GetColumnWidth(0));
    cfg->Write(wxT("/DrivesList/width2"),m_vList->GetColumnWidth(1));
    cfg->Write(wxT("/DrivesList/width3"),m_vList->GetColumnWidth(2));
    cfg->Write(wxT("/DrivesList/width4"),m_vList->GetColumnWidth(3));
    cfg->Write(wxT("/DrivesList/width5"),m_vList->GetColumnWidth(4));
    cfg->Write(wxT("/DrivesList/width6"),m_vList->GetColumnWidth(5));

    cfg->Write(wxT("/Language/Selected"),
        (long)g_locale->GetLanguage());

    cfg->Write(wxT("/Algorithm/RepeatAction"),m_repeat);
    cfg->Write(wxT("/Algorithm/SkipRemovableMedia"),m_skipRem);

    // save sorting parameters
    if(m_menuBar->FindItem(ID_SortByPath)->IsChecked()){
        cfg->Write(wxT("/Algorithm/Sorting"),wxT("path"));
    } else if(m_menuBar->FindItem(ID_SortBySize)->IsChecked()){
        cfg->Write(wxT("/Algorithm/Sorting"),wxT("size"));
    } else if(m_menuBar->FindItem(ID_SortByCreationDate)->IsChecked()){
        cfg->Write(wxT("/Algorithm/Sorting"),wxT("c_time"));
    } else if(m_menuBar->FindItem(ID_SortByModificationDate)->IsChecked()){
        cfg->Write(wxT("/Algorithm/Sorting"),wxT("m_time"));
    } else if(m_menuBar->FindItem(ID_SortByLastAccessDate)->IsChecked()){
        cfg->Write(wxT("/Algorithm/Sorting"),wxT("a_time"));
    }
    if(m_menuBar->FindItem(ID_SortAscending)->IsChecked()){
        cfg->Write(wxT("/Algorithm/SortingOrder"),wxT("asc"));
    } else {
        cfg->Write(wxT("/Algorithm/SortingOrder"),wxT("desc"));
    }

    cfg->Write(wxT("/Upgrade/Level"),
        (long)m_upgradeThread->m_level);
}

// =======================================================================
//                          User preferences
// =======================================================================

#define UD_AdjustOption(name) { \
    if(!wxGetEnv(wxT("UD_") wxT(#name),NULL)) \
        wxSetEnv(wxT("UD_") wxT(#name), \
            wxString::Format(wxT("%u"),DEFAULT_##name)); \
}

void MainFrame::ReadUserPreferences(wxCommandEvent& WXUNUSED(event))
{
    /*
    * The program should be configurable
    * through guiopts.lua file only.
    */
    wxUnsetEnv(wxT("UD_DBGPRINT_LEVEL"));
    wxUnsetEnv(wxT("UD_DISABLE_REPORTS"));
    wxUnsetEnv(wxT("UD_DRY_RUN"));
    wxUnsetEnv(wxT("UD_EX_FILTER"));
    wxUnsetEnv(wxT("UD_FILE_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FRAGMENT_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FRAGMENTATION_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FRAGMENTS_THRESHOLD"));
    wxUnsetEnv(wxT("UD_FREE_COLOR_R"));
    wxUnsetEnv(wxT("UD_FREE_COLOR_G"));
    wxUnsetEnv(wxT("UD_FREE_COLOR_B"));
    wxUnsetEnv(wxT("UD_GRID_COLOR_R"));
    wxUnsetEnv(wxT("UD_GRID_COLOR_G"));
    wxUnsetEnv(wxT("UD_GRID_COLOR_B"));
    wxUnsetEnv(wxT("UD_GRID_LINE_WIDTH"));
    wxUnsetEnv(wxT("UD_IN_FILTER"));
    wxUnsetEnv(wxT("UD_LOG_FILE_PATH"));
    wxUnsetEnv(wxT("UD_MAP_BLOCK_SIZE"));
    wxUnsetEnv(wxT("UD_MINIMIZE_TO_SYSTEM_TRAY"));
    wxUnsetEnv(wxT("UD_OPTIMIZER_FILE_SIZE_THRESHOLD"));
    wxUnsetEnv(wxT("UD_REFRESH_INTERVAL"));
    wxUnsetEnv(wxT("UD_SECONDS_FOR_SHUTDOWN_REJECTION"));
    wxUnsetEnv(wxT("UD_SHOW_MENU_ICONS"));
    wxUnsetEnv(wxT("UD_SHOW_PROGRESS_IN_TASKBAR"));
    wxUnsetEnv(wxT("UD_SHOW_TASKBAR_ICON_OVERLAY"));
    wxUnsetEnv(wxT("UD_SORTING"));
    wxUnsetEnv(wxT("UD_SORTING_ORDER"));
    wxUnsetEnv(wxT("UD_TIME_LIMIT"));

    /* interprete guiopts.lua file */
    lua_State *L; int status;
    wxFileName path(wxT(".\\options\\guiopts.lua"));
    path.Normalize();
    if(!path.FileExists()){
        etrace("%ls file not found",
            path.GetFullPath().wc_str());
        goto done;
    }

    L = lua_open();
    if(!L){
        etrace("Lua initialization failed");
        goto done;
    }

    /* stop collector during initialization */
    lua_gc(L,LUA_GCSTOP,0);
    luaL_openlibs(L);
    lua_gc(L,LUA_GCRESTART,0);

    status = luaL_dofile(L,path.GetFullPath().char_str());
    if(status != 0){
        wxString error = wxT("cannot interprete ") + path.GetFullPath();
        etrace("%ls",error.wc_str());
        if(!lua_isnil(L,-1)){
            const char *msg = lua_tostring(L,-1);
            if(!msg) msg = "(error object is not a string)";
            etrace("%hs",msg);
            error += wxString::Format(wxT("\n%hs"),msg);
            lua_pop(L, 1);
        }
        wxMessageDialog dlg(NULL,error,
            wxT("UltraDefrag"),wxOK | wxICON_ERROR);
        dlg.ShowModal();
    }

    lua_close(L);

done:
    // ensure that GUI specific options are always set
    UD_AdjustOption(DRY_RUN);
    UD_AdjustOption(GRID_COLOR_R);
    UD_AdjustOption(GRID_COLOR_G);
    UD_AdjustOption(GRID_COLOR_B);
    UD_AdjustOption(GRID_LINE_WIDTH);
    UD_AdjustOption(FREE_COLOR_R);
    UD_AdjustOption(FREE_COLOR_G);
    UD_AdjustOption(FREE_COLOR_B);
    UD_AdjustOption(MAP_BLOCK_SIZE);
    UD_AdjustOption(MINIMIZE_TO_SYSTEM_TRAY);
    UD_AdjustOption(SECONDS_FOR_SHUTDOWN_REJECTION);
    UD_AdjustOption(SHOW_MENU_ICONS);
    UD_AdjustOption(SHOW_PROGRESS_IN_TASKBAR);
    UD_AdjustOption(SHOW_TASKBAR_ICON_OVERLAY);

    // reset log file path
    wxString v;
    if(wxGetEnv(wxT("UD_LOG_FILE_PATH"),&v)){
        wxFileName logpath(v); logpath.Normalize();
        wxSetEnv(wxT("UD_LOG_FILE_PATH"),logpath.GetFullPath());
    }
    ::udefrag_set_log_file_path();
}

int MainFrame::CheckOption(const wxString& name)
{
    wxString value;
    if(wxGetEnv(name,&value)){
        unsigned long v;
        if(value.ToULong(&v))
            return (int)v;
    }
    return 0;
}

#undef UD_AdjustOption

// =======================================================================
//                      User preferences tracking
// =======================================================================

void *ConfigThread::Entry()
{
    ULONGLONG counter = 0;

    itrace("configuration tracking started");

    HANDLE h = FindFirstChangeNotification(wxT(".\\options"),
        FALSE,FILE_NOTIFY_CHANGE_LAST_WRITE);
    if(h == INVALID_HANDLE_VALUE){
        letrace("FindFirstChangeNotification failed");
        goto done;
    }

    while(!g_mainFrame->CheckForTermination(1)){
        DWORD status = WaitForSingleObject(h,100);
        if(status == WAIT_OBJECT_0){
            if(!(counter % 2)){
                /*
                * Do nothing, 'cause
                * "If a change occurs after a call
                * to FindFirstChangeNotification but
                * before a call to FindNextChangeNotification,
                * the operating system records the change.
                * When FindNextChangeNotification is executed,
                * the recorded change immediately satisfies
                * a wait for the change notification." (MSDN)
                * And so on... it happens between
                * FindNextChangeNotification calls too.
                *
                * However, everything mentioned above is false
                * when the program modifies the file itself.
                */
            } else {
                itrace("configuration has been changed");
                PostCommandEvent(g_mainFrame,ID_ReadUserPreferences);
                PostCommandEvent(g_mainFrame,ID_SetWindowTitle);
                PostCommandEvent(g_mainFrame,ID_AdjustSystemTrayIcon);
                PostCommandEvent(g_mainFrame,ID_AdjustTaskbarIconOverlay);
                PostCommandEvent(g_mainFrame,ID_RedrawMap);
            }
            counter ++;
            /* wait for the next notification */
            if(!FindNextChangeNotification(h)){
                letrace("FindNextChangeNotification failed");
                break;
            }
        }
    }

    FindCloseChangeNotification(h);

done:
    itrace("configuration tracking stopped");
    return NULL;
}

// =======================================================================
//                            Event handlers
// =======================================================================

void MainFrame::OnGuiOptions(wxCommandEvent& WXUNUSED(event))
{
    if(m_title->Find(wxT("Portable")) != wxNOT_FOUND)
        Utils::ShellExec(wxT("notepad"),wxT("open"),wxT(".\\options\\guiopts.lua"));
    else
        Utils::ShellExec(wxT(".\\options\\guiopts.lua"),wxT("edit"));
}

void MainFrame::OnBootScript(wxCommandEvent& WXUNUSED(event))
{
    wxFileName script(wxT("%SystemRoot%\\system32\\ud-boot-time.cmd"));
    script.Normalize(); Utils::ShellExec(script.GetFullPath(),wxT("edit"));
}

void MainFrame::OnReportOptions(wxCommandEvent& WXUNUSED(event))
{
    if(m_title->Find(wxT("Portable")) != wxNOT_FOUND)
        Utils::ShellExec(wxT("notepad"),wxT("open"),wxT(".\\options\\udreportopts.lua"));
    else
        Utils::ShellExec(wxT(".\\options\\udreportopts.lua"),wxT("edit"));
}

/** @} */
