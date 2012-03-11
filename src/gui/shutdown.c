/*
 *  UltraDefrag - a powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
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

/**
 * @file shutdown.c
 * @brief Shutdown.
 * @addtogroup Shutdown
 * @{
 */

/*
* Ideas by Stefan Pendl <stefanpe@users.sourceforge.net>
* and Dmitri Arkhangelski <dmitriar@gmail.com>.
*/

#include "main.h"
#include <powrprof.h>

/**
 * @brief Adjusts position of controls inside
 * the shutdown confirmation dialog to make it
 * nice looking independent from text lengths
 * and font size.
 */
static void ResizeShutdownConfirmDialog(HWND hwnd,wchar_t *counter_msg)
{
    int client_width, client_height;
    int icon_width = 32;
    int icon_height = 32;
    int spacing = 7;
    int large_spacing = 11;
    int margin = 11;
    int extra_spacing = 20;
    int text1_width, text1_height;
    int text2_width, text2_height;
    int button_width = 75;
    int button_height = 23;
    HDC hdc;
    wchar_t *text1 = NULL, *text2 = NULL;
    wchar_t buffer[256];
    SIZE size;
    int text_block_width;
    int text_block_height;
    int top_width, top_height;
    int bottom_width, bottom_height;
    int icon_y, text1_y;
    int button1_x;
    RECT rc;
    HFONT hFont, hOldFont;
    
    /* get dimensions of texts */
    hdc = GetDC(hwnd);
    if(hdc == NULL){
        WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot get device context of the window");
        goto done;
    }
    
    if(use_custom_font_in_dialogs){
        hFont = wgxFont.hFont;
    } else {
        hFont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
        if(hFont == NULL){
            WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot get default font");
            ReleaseDC(hwnd,hdc);
            goto done;
        }
    }
    hOldFont = SelectObject(hdc,hFont);
    
    switch(when_done_action){
    case IDM_WHEN_DONE_HIBERNATE:
        text1 = WgxGetResourceString(i18n_table,L"REALLY_HIBERNATE_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_LOGOFF:
        text1 = WgxGetResourceString(i18n_table,L"REALLY_LOGOFF_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_REBOOT:
        text1 = WgxGetResourceString(i18n_table,L"REALLY_REBOOT_WHEN_DONE");
        break;
    case IDM_WHEN_DONE_SHUTDOWN:
        text1 = WgxGetResourceString(i18n_table,L"REALLY_SHUTDOWN_WHEN_DONE");
        break;
    default:
        text1 = _wcsdup(L"");
        break;
    }
    
    if(text1 == NULL){
        WgxDbgPrint("ResizeShutdownConfirmDialog: cannot allocate memory for text 1");
        SelectObject(hdc,hOldFont);
        ReleaseDC(hwnd,hdc);
        goto done;
    }
    
    if(!GetTextExtentPoint32W(hdc,text1,wcslen(text1),&size)){
        WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot get text1 dimensions");
        SelectObject(hdc,hOldFont);
        ReleaseDC(hwnd,hdc);
        goto done;
    }
    text1_width = size.cx;
    text1_height = size.cy;
    
    /* TODO: do it more accurately, without the need of extra_spacing  */
    _snwprintf(buffer,sizeof(buffer)/sizeof(wchar_t),L"%u %ws",seconds_for_shutdown_rejection,counter_msg);
    buffer[sizeof(buffer)/sizeof(wchar_t) - 1] = 0;
    text2 = buffer;
    if(!GetTextExtentPoint32W(hdc,text2,wcslen(text2),&size)){
        WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot get text2 dimensions");
        SelectObject(hdc,hOldFont);
        ReleaseDC(hwnd,hdc);
        goto done;
    }
    text2_width = size.cx + extra_spacing;
    text2_height = size.cy;
    
    SelectObject(hdc,hOldFont);
    ReleaseDC(hwnd,hdc);
    
    /* calculate dimensions of the entire text block */
    text_block_width = max(text1_width,text2_width);
    text_block_height = text1_height + spacing + text2_height;
    
    top_width = icon_width + large_spacing + text_block_width;
    top_height = max(icon_height,text_block_height);
    bottom_width = button_width * 2 + spacing;
    bottom_height = button_height;
    if(top_width < bottom_width){
        text_block_width += bottom_width - top_width;
        top_width = bottom_width;
    }
    
    /* calculate dimensions of the entire client area */
    client_width = top_width + margin * 2;
    client_height = top_height + large_spacing + bottom_height + margin * 2;
    
    rc.left = 0;
    rc.right = client_width;
    rc.top = 0;
    rc.bottom = client_height;
    if(!AdjustWindowRect(&rc,WS_CAPTION | WS_DLGFRAME,FALSE)){
        WgxDbgPrintLastError("ResizeShutdownConfirmDialog: cannot calculate window dimensions");
        goto done;
    }
            
    /* resize main window */
    MoveWindow(hwnd,0,0,rc.right - rc.left,rc.bottom - rc.top,FALSE);
            
    /* reposition controls */
    if(icon_height <= text_block_height){
        icon_y = (text_block_height - icon_height) / 2 + margin;
        text1_y = margin;
    } else {
        icon_y = margin;
        text1_y = (icon_height - text_block_height) / 2 + margin;
    }
    SetWindowPos(GetDlgItem(hwnd,IDC_SHUTDOWN_ICON),NULL,
        margin,
        icon_y,
        icon_width,
        icon_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_MESSAGE),NULL,
        margin + icon_width + large_spacing,
        text1_y,
        text_block_width,
        text1_height + spacing,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_DELAY_MSG),NULL,
        margin + icon_width + large_spacing,
        text1_y + text1_height + spacing,
        text_block_width,
        text2_height + spacing,
        SWP_NOZORDER);
    button1_x = (client_width - button_width * 2 - spacing) / 2;
    SetWindowPos(GetDlgItem(hwnd,IDC_YES_BUTTON),NULL,
        button1_x,
        margin + top_height + large_spacing,
        button_width,
        button_height,
        SWP_NOZORDER);
    SetWindowPos(GetDlgItem(hwnd,IDC_NO_BUTTON),NULL,
        button1_x + button_width + spacing,
        margin + top_height + large_spacing,
        button_width,
        button_height,
        SWP_NOZORDER);

    /* set localized texts */
    SetWindowTextW(GetDlgItem(hwnd,IDC_MESSAGE),text1);
    SetWindowTextW(GetDlgItem(hwnd,IDC_DELAY_MSG),text2);
    
    /* center window on the screen */
    WgxCenterWindow(hwnd);

done:
    if(text1) free(text1);
}

/**
 * @brief Asks user whether he really wants
 * to shutdown/hibernate the computer.
 */
BOOL CALLBACK ShutdownConfirmDlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    static UINT_PTR timer;
    static UINT counter;
    #define TIMER_ID 0x16748382
    #define MAX_TEXT_LENGTH 127
    static wchar_t buffer[MAX_TEXT_LENGTH + 1];
    static wchar_t counter_msg[MAX_TEXT_LENGTH + 1];
    wchar_t *text = NULL;

    switch(msg){
    case WM_INITDIALOG:
        if(use_custom_font_in_dialogs)
            WgxSetFont(hWnd,&wgxFont);
        WgxSetText(hWnd,i18n_table,L"PLEASE_CONFIRM");
        WgxSetText(GetDlgItem(hWnd,IDC_YES_BUTTON),i18n_table,L"YES");
        WgxSetText(GetDlgItem(hWnd,IDC_NO_BUTTON),i18n_table,L"NO");
        switch(when_done_action){
        case IDM_WHEN_DONE_HIBERNATE:
            text = WgxGetResourceString(i18n_table,L"SECONDS_TILL_HIBERNATION");
            break;
        case IDM_WHEN_DONE_LOGOFF:
            text = WgxGetResourceString(i18n_table,L"SECONDS_TILL_LOGOFF");
            break;
        case IDM_WHEN_DONE_REBOOT:
            text = WgxGetResourceString(i18n_table,L"SECONDS_TILL_REBOOT");
            break;
        case IDM_WHEN_DONE_SHUTDOWN:
            text = WgxGetResourceString(i18n_table,L"SECONDS_TILL_SHUTDOWN");
            break;
        }
        if(text){
            wcsncpy(counter_msg,text,MAX_TEXT_LENGTH);
            counter_msg[MAX_TEXT_LENGTH] = 0;
            free(text);
        } else {
            counter_msg[0] = 0;
        }

        /* set timer */
        counter = seconds_for_shutdown_rejection;
        if(counter == 0)
            (void)EndDialog(hWnd,1);
        timer = (UINT_PTR)SetTimer(hWnd,TIMER_ID,1000,NULL);
        if(timer == 0){
            /* message box will prevent shutdown which is dangerous */
            WgxDbgPrintLastError("ShutdownConfirmDlgProc: SetTimer failed");
            /* force shutdown to avoid situation when pc works a long time without any control */
            (void)EndDialog(hWnd,1);
        }
        
        /* adjust positions of controls to make the window nice looking */
        ResizeShutdownConfirmDialog(hWnd,counter_msg);

        /* shutdown will be rejected by pressing the space key */
        (void)SetFocus(GetDlgItem(hWnd,IDC_NO_BUTTON));
        return FALSE;
    case WM_TIMER:
        counter --;
        _snwprintf(buffer,MAX_TEXT_LENGTH + 1,L"%u %ls",counter,counter_msg);
        buffer[MAX_TEXT_LENGTH] = 0;
        SetWindowTextW(GetDlgItem(hWnd,IDC_DELAY_MSG),buffer);
        if(counter == 0){
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,1);
        }
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)){
        case IDC_YES_BUTTON:
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,1);
            break;
        case IDC_NO_BUTTON:
            (void)KillTimer(hWnd,TIMER_ID);
            (void)EndDialog(hWnd,0);
            break;
        }
        return TRUE;
    case WM_CLOSE:
        (void)KillTimer(hWnd,TIMER_ID);
        (void)EndDialog(hWnd,0);
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Shuts computer down or hibernates
 * them respect to user preferences.
 * @return Error code. Zero indicates success.
 */
int ShutdownOrHibernate(void)
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp;
    BOOL result;

    switch(when_done_action){
    case IDM_WHEN_DONE_HIBERNATE:
    case IDM_WHEN_DONE_LOGOFF:
    case IDM_WHEN_DONE_REBOOT:
    case IDM_WHEN_DONE_SHUTDOWN:
        if(seconds_for_shutdown_rejection){
            if(DialogBoxW(hInstance,MAKEINTRESOURCEW(IDD_SHUTDOWN),NULL,(DLGPROC)ShutdownConfirmDlgProc) == 0)
                return 0;
            /* in case of errors we'll shutdown anyway */
            /* to avoid situation when pc works a long time without any control */
        }
        break;
    case IDM_WHEN_DONE_EXIT:
        return 0; /* nothing to do */
    }
    
    /* set SE_SHUTDOWN privilege */
    if(!OpenProcessToken(GetCurrentProcess(), 
    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken)){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"ShutdownOrHibernate: cannot open process token!");
        return 4;
    }
    
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
    AdjustTokenPrivileges(hToken,FALSE,&tkp,0,(PTOKEN_PRIVILEGES)NULL,0);         
    if(GetLastError() != ERROR_SUCCESS){
        WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"ShutdownOrHibernate: cannot set shutdown privilege!");
        return 5;
    }
    
    /*
    * There is an opinion that SetSuspendState call
    * is more reliable than SetSystemPowerState:
    * http://msdn.microsoft.com/en-us/library/aa373206%28VS.85%29.aspx
    */
    switch(when_done_action){
    case IDM_WHEN_DONE_STANDBY:
        /* suspend, request permission from apps and drivers */
        result = SetSuspendState(FALSE,FALSE,FALSE);
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot suspend the system!");
            return 6;
        }
        break;
    case IDM_WHEN_DONE_HIBERNATE:
        /* hibernate, request permission from apps and drivers */
        result = SetSuspendState(TRUE,FALSE,FALSE);
        if(!result){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot hibernate the computer!");
            return 7;
        }
        break;
    case IDM_WHEN_DONE_LOGOFF:
        if(!ExitWindowsEx(EWX_LOGOFF | EWX_FORCEIFHUNG,
          SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot log the user off!");
            return 8;
        }
        break;
    case IDM_WHEN_DONE_REBOOT:
        if(!ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG,
          SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot reboot the computer!");
            return 9;
        }
        break;
    case IDM_WHEN_DONE_SHUTDOWN:
        /*
        * InitiateSystemShutdown() works fine
        * but doesn't power off the pc.
        */
        if(!ExitWindowsEx(EWX_POWEROFF | EWX_FORCEIFHUNG,
          SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED)){
            WgxDisplayLastError(NULL,MB_OK | MB_ICONHAND,"UltraDefrag: cannot shut down the computer!");
            return 10;
        }
        break;
    }
    
    return 0;
}

/** @} */
