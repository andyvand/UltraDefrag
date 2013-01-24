/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2013 Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
* BootExecute Control program.
*/

/*
* To include CheckTokenMembership definition 
* (used by CheckAdminRights) on mingw the 
* _WIN32_WINNT constant must be set at least
* to 0x500.
*/
#if defined(__GNUC__)
#define _WIN32_WINNT 0x500
#endif
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <commctrl.h>

#include "../dll/zenwinx/zenwinx.h"

/* include CheckAdminRights */
#include "../share/utils.c"

#define error(msg) { \
    if(!silent) MessageBox(NULL,msg, \
        "BootExecute Control", \
        MB_OK | MB_ICONHAND); \
}

char cmd[32768] = {0}; /* should be enough as MSDN states */
int silent = 0;

int __cdecl main(int argc,char **argv)
{
    int h_flag = 0, r_flag = 0, u_flag = 0;
    int i, result;
    wchar_t *ucmd;

    /*
    * Strongly required to be 
    * compatible with manifest.
    */
    InitCommonControls();

    /* parse command line */
    for(i = 1; i < argc; i++){
        if(!_stricmp(argv[i],"/h")){
            h_flag = 1;
        } else if(!_stricmp(argv[i],"/r")){
            r_flag = 1;
        } else if(!_stricmp(argv[i],"/s")){
            silent = 1;
        } else if(!_stricmp(argv[i],"/u")){
            u_flag = 1;
        } else if(!_stricmp(argv[i],"/?")){
            h_flag = 1;
        } else {
            strncpy(cmd,argv[i],sizeof(cmd));
            cmd[sizeof(cmd) - 1] = 0;
        }
    }

    /* handle help requests */
    if(h_flag || !cmd[0] || (!r_flag && !u_flag)){
        if(!silent) MessageBox(NULL,
            "Usage:\n"
            "bootexctrl /r [/s] command - register command\n"
            "bootexctrl /u [/s] command - unregister command\n"
            "Specify /s option to run the program in silent mode.",
            "BootExecute Control",
            MB_OK
        );
        return EXIT_SUCCESS;
    }
    
    if(winx_init_library() < 0){
        error("Initialization failed!");
        return EXIT_FAILURE;
    }

    /*
    * Check for admin rights - 
    * they're strongly required.
    */
    if(!CheckAdminRights()){
        error("Administrative rights are "
            "needed to run the program!");
        winx_unload_library();
        return EXIT_FAILURE;
    }

    /* convert command to Unicode */
    ucmd = winx_swprintf(L"%hs",cmd);
    if(!ucmd){
        error("Not enough memory!");
        winx_unload_library();
        return EXIT_FAILURE;
    }

    if(r_flag){
        result = winx_bootex_register(ucmd);
        if(result < 0){
            error("Cannot register the command!\n"
                "Use DbgView program to get more information.");
        }
    } else {
        result = winx_bootex_unregister(ucmd);
        if(result < 0){
            error("Cannot unregister the command!\n"
                "Use DbgView program to get more information.");
        }
    }
    winx_free(ucmd);
    winx_unload_library();
    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
