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
 * @file main.cpp
 * @brief Entry point.
 * @addtogroup Entry
 * @{
 */

// Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
// and Dmitri Arkhangelski <dmitriar@gmail.com>.

// =======================================================================
//                            Declarations
// =======================================================================

#include "main.h"

#if !defined(__GNUC__)
#include <new.h> // for _set_new_handler
#endif

#define MAX_ENV_VAR_LENGTH 32767 // as MSDN states

void cleanup(void);

// =======================================================================
//                          Global variables
// =======================================================================

Log *g_Log = NULL;

bool g_analyze = false;
bool g_optimize = false;
bool g_quick_optimization = false;
bool g_optimize_mft = false;
bool g_all = false;
bool g_all_fixed = false;
bool g_list_volumes = false;
bool g_list_all = false;
bool g_repeat = false;
bool g_no_progress = false;
bool g_show_vol_info = false;
bool g_show_map = false;
bool g_use_default_colors = false;
bool g_use_entire_window = false;
bool g_help = false;
bool g_wait = false;
bool g_shellex = false;
bool g_folder = false;
bool g_folder_itself = false;

wxArrayString *g_volumes = NULL;
wxArrayString *g_paths = NULL;

HANDLE g_out = NULL;
short  g_default_color = 0x7; // default text color

bool g_first_progress_update = true;
bool g_stop = false;

// =======================================================================
//                                Logging
// =======================================================================

/**
 * @brief Sets up custom logging.
 */
Log::Log()
{
    delete SetActiveTarget(this);
}

/**
 * @brief Restores default logging.
 */
Log::~Log()
{
    SetActiveTarget(NULL);
}

/**
 * @brief Performs logging.
 */
void Log::DoLog(wxLogLevel level,const wxChar *msg,time_t timestamp)
{
    #define INFO_FMT  (I wxCharStringFmtSpec)
    #define DEBUG_FMT (D wxCharStringFmtSpec)
    #define ERROR_FMT (E wxCharStringFmtSpec)

    switch(level){
    case wxLOG_FatalError:
        // XXX: fatal errors pass by actually
        ::winx_dbg_print(0,ERROR_FMT,msg);
        ::winx_flush_dbg_log();
        break;
    case wxLOG_Error:
        ::winx_dbg_print(0,ERROR_FMT,msg);
        break;
    case wxLOG_Warning:
    case wxLOG_Info:
        ::winx_dbg_print(0,DEBUG_FMT,msg);
        break;
    default:
        ::winx_dbg_print(0,INFO_FMT,msg);
        break;
    }
}

// =======================================================================
//                           Errors handling
// =======================================================================

/**
 * @brief Prints the string in red, than restores green color.
 */
void display_error(const char *msg)
{
    color(FOREGROUND_RED | FOREGROUND_INTENSITY);
    fprintf(stderr,"%s",msg);
    color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message specific for a failed disk processing.
 */
static void display_defrag_error(udefrag_job_type job_type, int error)
{
    color(FOREGROUND_RED | FOREGROUND_INTENSITY);

    const char *operation = "optimization";
    if(job_type == ANALYSIS_JOB)
        operation = "analysis";
    else if(job_type == DEFRAGMENTATION_JOB)
        operation = "defragmentation";

    fprintf(stderr,"\nDisk %s failed!\n\n",operation);

    color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    fprintf(stderr,"%s\n\n",udefrag_get_error_description(error));
    if(error == UDEFRAG_UNKNOWN_ERROR)
        fprintf(stderr,"Enable logs or use DbgView program to get more information.\n\n");

    color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

/**
 * @brief Displays error message
 * specific for invalid disk volumes.
 */
static void display_invalid_volume_error(int error)
{
    color(FOREGROUND_RED | FOREGROUND_INTENSITY);
    fprintf(stderr,"The disk cannot be processed.\n\n");
    color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    if(error == UDEFRAG_UNKNOWN_ERROR){
        fprintf(stderr,"Disk is missing or some unknown error has been encountered.\n");
        fprintf(stderr,"Enable logs or use DbgView program to get more information.\n\n");
    } else {
        fprintf(stderr,"%s\n\n",udefrag_get_error_description(error));
    }

    color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

// =======================================================================
//                      Synchronization procedures
// =======================================================================

HANDLE g_synch_event;

/**
* @brief Synchronizes with other scheduled
* jobs when running with --wait option.
*/
static void begin_synchronization(void)
{
    // create event
    do {
        g_synch_event = CreateEvent(NULL,FALSE,TRUE,wxT("udefrag-exe-synch-event"));
        if(!g_synch_event){
            letrace("cannot create udefrag-exe-synch-event event");
            display_error("Synchronization failed!\n");
            break;
        }
        if(GetLastError() == ERROR_ALREADY_EXISTS && g_wait){
            CloseHandle(g_synch_event);
            Sleep(1000);
            continue;
        }
        break;
    } while(1);
}

static void end_synchronization(void)
{
    // release event
    if(g_synch_event) CloseHandle(g_synch_event);
}

// =======================================================================
//                     Volumes processing procedures
// =======================================================================

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
    g_stop = true;
    return TRUE;
}

void update_progress(udefrag_progress_info *pi, void *p)
{
    if(g_first_progress_update){
        /*
        * Ultra fast ntfs analysis contains one piece of code
        * that heavily loads CPU for one-two seconds, so let's
        * increase the priority of the progress drawing thread
        * for a bit more smooth progress indication.
        */
        (void)SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
        g_first_progress_update = false;
    }

    if(!g_no_progress){
        if(g_show_map){
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if(GetConsoleScreenBufferInfo(g_out,&csbi)){
                COORD pos; pos.X = 0;
                pos.Y = csbi.dwCursorPosition.Y - g_map_rows - 3 - 2;
                (void)SetConsoleCursorPosition(g_out,pos);
            }
        }

        const char *op_name = "optimize: ";
        if(pi->current_operation == VOLUME_ANALYSIS) op_name = "analyze:  ";
        else if(pi->current_operation == VOLUME_DEFRAGMENTATION) op_name = "defrag:   ";

        char letter = (char)(DWORD_PTR)p;

        clear_line();
        if(pi->current_operation == VOLUME_OPTIMIZATION && !g_stop && pi->completion_status == 0){
            if(pi->pass_number > 1)
                printf("\r%c: %s%6.2lf%% complete, pass %lu, moves total = %I64u",
                    letter,op_name,pi->percentage,pi->pass_number,pi->total_moves);
            else
                printf("\r%c: %s%6.2lf%% complete, moves total = %I64u",
                    letter,op_name,pi->percentage,pi->total_moves);
        } else {
            if(pi->pass_number > 1)
                printf("\r%c: %s%6.2lf%% complete, pass %lu, fragmented/total = %lu/%lu",
                    letter,op_name,pi->percentage,pi->pass_number,pi->fragmented,pi->files);
            else
                printf("\r%c: %s%6.2lf%% complete, fragmented/total = %lu/%lu",
                    letter,op_name,pi->percentage,pi->fragmented,pi->files);
        }
        if(pi->completion_status != 0 && !g_stop){
            /* set progress indicator to 100% state */
            clear_line();
            if(pi->pass_number > 1)
                printf("\r%c: %s100.00%% complete, %lu passes needed, fragmented/total = %lu/%lu",
                    letter,op_name,pi->pass_number,pi->fragmented,pi->files);
            else
                printf("\r%c: %s100.00%% complete, fragmented/total = %lu/%lu",
                    letter,op_name,pi->fragmented,pi->files);
            if(!g_show_map) printf("\n");
        }

        if(g_show_map) redraw_map(pi);
    }

    if(pi->completion_status != 0 && g_show_vol_info){
        /* print results of the completed job */
        char *results = udefrag_get_results(pi);
        if(results){
            printf("\n%s",results);
            udefrag_release_results(results);
        }
    }
}

int terminator(void *p)
{
    /* do it as quickly as possible :-) */
    return g_stop;
}

static bool process_single_volume(char letter)
{
    int result = udefrag_validate_volume(letter,false);
    if(result < 0){
        display_invalid_volume_error(result);
        return false;
    }

    init_map(letter);

    long map_size = g_map_rows * g_map_symbols_per_line;

    udefrag_job_type job_type = DEFRAGMENTATION_JOB;
    if(g_analyze) job_type = ANALYSIS_JOB;
    else if(g_optimize) job_type = FULL_OPTIMIZATION_JOB;
    else if(g_quick_optimization) job_type = QUICK_OPTIMIZATION_JOB;
    else if(g_optimize_mft) job_type = MFT_OPTIMIZATION_JOB;

    int flags = g_repeat ? UD_JOB_REPEAT : 0;
    if(g_shellex) flags |= UD_JOB_CONTEXT_MENU_HANDLER;

    g_stop = false; g_first_progress_update = true;

    result = udefrag_start_job(letter,job_type,flags,map_size,
        update_progress,terminator,(void *)(DWORD_PTR)letter);
    if(result < 0) display_defrag_error(job_type,result);

    destroy_map();
    return (result == 0);
}

static int process_volumes(void)
{
    bool overall_result = false;

    if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE)){
        letrace("cannot set Ctrl + C handler");
        display_error("Cannot set Ctrl + C handler!\n");
    }

    begin_synchronization();

    /* uncomment for the --wait option testing */
    //printf("the job gets running\n");
    //_getch();

    /* process paths */
    wxString orig_cut_filter; wxString cut_filter;
    bool save_filter = wxGetEnv(wxT("UD_CUT_FILTER"),&orig_cut_filter);
    char letter = 0;

    bool first_group = true; g_paths->Sort();
    for(int i = 0; i < (int)g_paths->GetCount(); i++){
        wxString path = (*g_paths)[i];
        if(g_shellex && g_folder){
            if(path.Last() == '\\'){
                // c:\ => c:\*
                path << wxT("*");
            } else {
                // c:\test => c:\test;c:\test\*
                wxString s = path;
                path << wxT(";") << s << wxT("\\*");
            }
        }

        if(path.Len() > MAX_ENV_VAR_LENGTH){
            // path is too long to be put into environment variable
            wxLogError(wxT("%ls path is too long"),path.wc_str());
            display_error(wxString::Format(
                wxT("%ls path is too long"),
                path.wc_str()).char_str());
            continue;
        }

        if(g_stop) break;

        // if cannot append path to cut filter
        // (too long or have another letter)
        // process everything from the cut filter
        // and reset it then before putting the current
        // path into it
        if((path.Len() + cut_filter.Len() > MAX_ENV_VAR_LENGTH) \
            || (letter != 0 && path[0] != letter))
        {
            wxSetEnv(wxT("UD_CUT_FILTER"),cut_filter);
            if(process_single_volume(letter))
                overall_result = true;
            cut_filter.Clear();
            first_group = false;
        }

        cut_filter << path;
        letter = path[0];

        color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        if(!first_group) printf("\n");
        print_unicode(path.wchar_str());
        printf("\n");
        color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }

    if(!cut_filter.IsEmpty()){
        wxSetEnv(wxT("UD_CUT_FILTER"),cut_filter);
        if(!g_stop){
            if(process_single_volume(letter))
                overall_result = true;
        }
    }

    if(save_filter)
        wxSetEnv(wxT("UD_CUT_FILTER"),orig_cut_filter);
    else
        wxUnsetEnv(wxT("UD_CUT_FILTER"));

    /* process volumes */
    for(int i = 0; i < (int)g_volumes->GetCount(); i++){
        if(g_stop) break;
        if(process_single_volume((*g_volumes)[i][0]))
            overall_result = true;
    }

    /* handle --all and --all-fixed options */
    if(g_all || g_all_fixed){
        volume_info *v = udefrag_get_vollist(g_all_fixed);
        if(v){
            for(int i = 0; v[i].letter; i++){
                if(g_stop) break;
                if(process_single_volume(v[i].letter))
                    overall_result = true;
            }
            udefrag_release_vollist(v);
        }
    }

    end_synchronization();

    (void)SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
    return (overall_result == true) ? 0 : 1;
}

// =======================================================================
//                       Volumes listing procedure
// =======================================================================

/**
 * @brief Displays list of disk volumes
 * available for defragmentation.
 */
static int list_volumes(void)
{
    color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("Disks available for defragmentation:\n\n");
    printf("Drive     FS     Capacity       Free   Label\n");
    printf("--------------------------------------------\n");

    volume_info *v = udefrag_get_vollist(g_list_all ? false : true);
    if(!v) return 1;

    for(int i = 0; v[i].letter; i++){
        char s[32];
        winx_bytes_to_hr((ULONGLONG)(v[i].total_space.QuadPart),2,s,sizeof(s));
        double total = (double)v[i].total_space.QuadPart;
        double free = (double)v[i].free_space.QuadPart;
        double d = (total > 0) ? free / total : 0;
        int percent = (int)(100 * d);
        color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("%c:  %8s %12s %8u %%   %ls\n",
            v[i].letter,v[i].fsname,s,percent,v[i].label);
    }
    udefrag_release_vollist(v);
    return 0;
}

// =======================================================================
//                             Web statistics
// =======================================================================

class StatThread: public wxThread {
public:
    StatThread() : wxThread(wxTHREAD_JOINABLE) { Create(); Run(); }
    ~StatThread() { Wait(); }

    virtual void *Entry();
};

void *StatThread::Entry()
{
    bool enabled = true; wxString s;
    if(wxGetEnv(wxT("UD_DISABLE_USAGE_TRACKING"),&s))
        if(s.Cmp(wxT("1")) == 0) enabled = false;

    if(enabled){
#ifndef _WIN64
        ga_request(wxT("/appstat/console-x86.html"));
#else
    #if defined(_IA64_)
        ga_request(wxT("/appstat/console-ia64.html"));
    #else
        ga_request(wxT("/appstat/console-x64.html"));
    #endif
#endif
    }

    return NULL;
}

StatThread *g_statThread = NULL;

// =======================================================================
//                    Application startup and shutdown
// =======================================================================

/**
 * @brief Initializes the application.
 */
bool init(int argc, char **argv)
{
    // initialize wxWidgets
    if(!wxInitialize(argc,argv)){
        fprintf(stderr,"wxWidgets initialization failed!\n");
        return false;
    }

    // initialize udefrag.dll library
    if(::udefrag_init_library() < 0){
        wxLogError(wxT("Initialization failed!"));
        cleanup();
        return false;
    }

    // initialize debug log
    wxString logpath;
    if(wxGetEnv(wxT("UD_LOG_FILE_PATH"),&logpath)){
        wxFileName file(logpath); file.Normalize();
        wxSetEnv(wxT("UD_LOG_FILE_PATH"),file.GetFullPath());
    }
    udefrag_set_log_file_path();

    // initialize logging
    g_Log = new Log();

    // start web statistics
    g_statThread = new StatThread();

    // check for administrative rights
    if(!check_admin_rights()){
        fprintf(stderr,"Administrative rights "
            "are needed to run the program!\n");
        cleanup();
        return false;
    }

    // parse command line
    if(!parse_cmdline(argc,argv)){
        cleanup();
        return false;
    }

    // set text color
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if(GetConsoleScreenBufferInfo(g_out,&csbi))
        g_default_color = csbi.wAttributes;
    color(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    return true;
}

/**
 * @brief Deinitializes the application.
 */
void cleanup(void)
{
    // stop web statistics
    delete g_statThread;

    // deinitialize logging
    delete g_Log;

    // deinitialize wxWidgets
    wxUninitialize();

    // free udefrag.dll library
    ::udefrag_unload_library();

    // restore text color
    color(g_default_color);

    // release resources
    delete g_volumes;
    delete g_paths;
}

// =======================================================================
//                             Entry point
// =======================================================================

#if !defined(__GNUC__)
static int out_of_memory_handler(size_t n)
{
    if(g_out) color(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("\nOut of memory!\n");
    if(g_out) color(g_default_color);
    exit(3); return 0;
}
#endif

int __cdecl main(int argc, char **argv)
{
    // set out of memory handler
#if !defined(__GNUC__)
    winx_set_killer(out_of_memory_handler);
    _set_new_handler(out_of_memory_handler);
    _set_new_mode(1);
#endif

    if(!init(argc,argv))
        return 1;

    if(g_help){
        show_help();
        cleanup();
        return 0;
    }

    printf(VERSIONINTITLE ", Copyright (c) UltraDefrag Development Team, 2007-2013.\n"
        "UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
        "and you are welcome to redistribute it under certain conditions.\n\n"
    );

    // uncomment to test out of memory condition
    /*for(int i = 0; i < 1000000000; i++)
        char *p = new char[1024];*/

    int result;

    if(g_list_volumes){
        result = list_volumes();
        cleanup();
        return result;
    }

    result = process_volumes();

    /* display prompt to hit any key in case of context menu handler */
    if(g_shellex){
        color(g_default_color);
        printf("\n");
        int pause_result = system("pause");
        if(pause_result > 0){
            /* command not found */
            printf("\n");
        }
        if(pause_result != 0 && pause_result != STATUS_CONTROL_C_EXIT){
            /* command or a command interpreter itself not found */
            printf("Hit any key to continue...");
            _getch();
        }
    }

    cleanup();
    return result;
}

/** @} */
