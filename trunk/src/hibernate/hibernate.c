/*
* Hibernate for Windows - a command line tool for Windows hibernation.
*
* Programmer:    Dmitri Arkhangelski (dmitriar@gmail.com)
* Creation date: October 2009
* License:       Public Domain
*/

/* Revised by Dmitri Arkhangelski, 2011, 2012 */

#include <windows.h>
#include <powrprof.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void show_help(void)
{
    printf(
        "Usage: \n"
        "  hibernate now - hibernates PC\n"
        "  hibernate /?  - displays this help\n"
        );
}

/**
 * @brief Prints Unicode string.
 * @note Only characters compatible
 * with the current locale (OEM by default)
 * will be printed correctly. Otherwise
 * the '?' characters will be printed instead.
 */
static void print_unicode_string(wchar_t *string)
{
    HANDLE hStdOut;
    DWORD written;
    
    if(string){
        hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleW(hStdOut,string,wcslen(string),&written,NULL);
    }
}

static void HandleError(char *msg)
{
    DWORD error;
    wchar_t *error_message;
    int length;

    error = GetLastError();
    /* format message and display it on the screen */
    if(FormatMessageW( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPWSTR)(void *)&error_message,
        0,
        NULL)){
            printf("%s: ",msg);
            /* get rid of trailing dot and new line */
            length = wcslen(error_message);
            if(length > 0){
                if(error_message[length - 1] == '\n')
                    error_message[length - 1] = 0;
            }
            if(length > 1){
                if(error_message[length - 2] == '\r')
                    error_message[length - 2] = 0;
            }
            if(length > 2){
                if(error_message[length - 3] == '.')
                    error_message[length - 3] = 0;
            }
            print_unicode_string(error_message);
            printf("!\n");
            LocalFree(error_message);
    } else {
        if(error == ERROR_COMMITMENT_LIMIT)
            printf("%s: not enough memory!\n",msg);
        else
            printf("%s: unknown reason!\n",msg);
    }
}

int __cdecl main(int argc, char **argv)
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    int i, now = 0;

    printf("Hibernate for Windows - command line tool for Windows hibernation.\n\n");

    for(i = 0; i < argc; i++){
        if(!strcmp(argv[i],"now")) now = 1;
        if(!strcmp(argv[i],"NOW")) now = 1;
    }

    if(now == 0){
        show_help();
    } else {
        /* enable shutdown privilege */
        if(!OpenProcessToken(GetCurrentProcess(), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
            HandleError("Cannot open process token");
            return 1;
        }
        
        LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
        tkp.PrivilegeCount = 1;  // one privilege to set    
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
        AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
        if(GetLastError() != ERROR_SUCCESS){
            HandleError("Cannot set shutdown privilege");
            return 1;
        }
        
        /* hibernate, request permission from apps and drivers */
        if(!SetSuspendState(TRUE,FALSE,FALSE)){
            HandleError("Cannot hibernate the computer");
            return 1;
        }
    }
    return 0;
}
