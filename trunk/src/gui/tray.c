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
 * @file tray.c
 * @brief System tray icons.
 * @addtogroup SystemTray
 * @{
 */

#include "main.h"

#define SYSTRAY_ICON_ID 0x1

/*
* This structure is accepted by Windows
* NT 4.0 and more recent Windows editions.
*/
typedef struct _UD_NOTIFYICONDATAW {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    WCHAR  szTip[64];
} UD_NOTIFYICONDATAW, *PUD_NOTIFYICONDATAW;

/**
 * @brief Adds an icon to the taskbar
 * notification area (system tray) or
 * modifies them.
 * @param[in] dwMessage the first parameter
 * to be passed in to the Shell_NotifyIcon
 * routine.
 * @return Boolean value. TRUE indicates success.
 */
BOOL ShowSystemTrayIcon(DWORD dwMessage)
{
    UD_NOTIFYICONDATAW nid;
    int id;
    
    if(!minimize_to_system_tray && dwMessage != NIM_DELETE) return TRUE;
    
    memset(&nid,0,sizeof(UD_NOTIFYICONDATAW));
    nid.cbSize = sizeof(UD_NOTIFYICONDATAW);
    nid.hWnd = hWindow;
    nid.uID = SYSTRAY_ICON_ID;
    if(dwMessage != NIM_DELETE){
        nid.uFlags = NIF_ICON | NIF_TIP;
        if(dwMessage == NIM_ADD){
            nid.uFlags |= NIF_MESSAGE;
            nid.uCallbackMessage = WM_TRAYMESSAGE;
        }
        /* set icon */
        id = job_is_running ? IDI_TRAY_ICON_BUSY : IDI_TRAY_ICON;
        if(!WgxLoadIcon(hInstance,id,GetSystemMetrics(SM_CXSMICON),&nid.hIcon)) goto fail;
        /* set tooltip */
        wcscpy(nid.szTip,L"UltraDefrag");
    }
    if(Shell_NotifyIconW(dwMessage,(NOTIFYICONDATAW *)(void *)&nid)) return TRUE;
    WgxDbgPrintLastError("ShowSystemTrayIcon: Shell_NotifyIconW failed");
    
fail:
    if(dwMessage == NIM_ADD){
        /* show main window again */
        ShowWindow(hWindow,SW_SHOW);
        /* turn off minimize to tray option */
        minimize_to_system_tray = 0;
        WgxDbgPrint("ShowSystemTrayIcon: minimize_to_system_tray option turned off");
    }
    return FALSE;
}

/**
 * @brief Sets the tray icon tooltip.
 */
void SetSystemTrayIconTooltip(wchar_t *text)
{
    UD_NOTIFYICONDATAW nid;
    
    if(!minimize_to_system_tray || text == NULL) return;
    
    memset(&nid,0,sizeof(UD_NOTIFYICONDATAW));
    nid.cbSize = sizeof(UD_NOTIFYICONDATAW);
    nid.hWnd = hWindow;
    nid.uID = SYSTRAY_ICON_ID;
    nid.uFlags = NIF_TIP;
    wcsncpy(nid.szTip,text,64);
    nid.szTip[63] = 0;
    if(!Shell_NotifyIconW(NIM_MODIFY,(NOTIFYICONDATAW *)(void *)&nid))
        WgxDbgPrintLastError("SetSystemTrayIconTooltip: Shell_NotifyIconW failed");
}

/**
 * @brief Shows the tray icon context menu.
 */
void ShowSystemTrayIconContextMenu(void)
{
    MENUITEMINFOW mi;
    HMENU hMenu = NULL;
    POINT pt;
    wchar_t *text;
    
    /* create popup menu */
    hMenu = CreatePopupMenu();
    if(hMenu == NULL){
        WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot create menu");
        goto done;
    }
    memset(&mi,0,sizeof(MENUITEMINFOW));
    mi.cbSize = sizeof(MENUITEMINFOW);
    mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mi.fType = MFT_STRING;
    mi.wID = IDM_SHOWHIDE;
    mi.fState = MFS_DEFAULT;
    text = WgxGetResourceString(i18n_table,IsWindowVisible(hWindow) ? L"HIDE" : L"SHOW");
    if(text){
        mi.dwTypeData = text;
        if(!InsertMenuItemW(hMenu,IDM_SHOWHIDE,FALSE,&mi)){
            WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot insert menu item (case 1)");
            free(text);
            goto done;
        }
        free(text);
    } else {
        mi.dwTypeData = IsWindowVisible(hWindow) ? L"Hide" : L"Show";
        if(!InsertMenuItemW(hMenu,IDM_SHOWHIDE,FALSE,&mi)){
            WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot insert menu item (case 1)");
            goto done;
        }
    }
    mi.fType = MFT_SEPARATOR;
    mi.wID = 0, mi.fState = 0;
    mi.dwTypeData = NULL;
    if(!InsertMenuItemW(hMenu,0,FALSE,&mi)){
        WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot insert menu item (case 2)");
        goto done;
    }
    mi.fType = MFT_STRING;
    mi.wID = IDM_EXIT;
    mi.fState = 0;
    text = WgxGetResourceString(i18n_table,L"EXIT");
    if(text){
        mi.dwTypeData = text;
        if(!InsertMenuItemW(hMenu,IDM_EXIT,FALSE,&mi)){
            WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot insert menu item (case 3)");
            free(text);
            goto done;
        }
        free(text);
    } else {
        mi.dwTypeData = L"Exit";
        if(!InsertMenuItemW(hMenu,IDM_EXIT,FALSE,&mi)){
            WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot insert menu item (case 3)");
            goto done;
        }
    }
    
    /* show menu */
    if(!GetCursorPos(&pt)){
        WgxDbgPrintLastError("ShowSystemTrayIconContextMenu: cannot get cursor position");
        goto done;
    }
    (void)TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON,pt.x,pt.y,0,hWindow,NULL);

done:
    if(hMenu) DestroyMenu(hMenu);
}

/**
 * @brief Removes an icon from the taskbar
 * notification area (system tray).
 * @return Boolean value. TRUE indicates success.
 */
BOOL HideSystemTrayIcon(void)
{
    return ShowSystemTrayIcon(NIM_DELETE);
}

/** @} */
