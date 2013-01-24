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

/**
 * @file utils.c
 * @brief Miscellaneous utilities.
 * @addtogroup Utilities
 * @{
 */

/**
 * @brief Defines whether the user
 * has administrative rights or not.
 * @details Based on the UserInfo
 * NSIS plug-in.
 */
BOOL CheckAdminRights(void)
{
    HANDLE hToken;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = {SECURITY_NT_AUTHORITY};
    PSID psid = NULL;
    BOOL IsMember = FALSE;
    BOOL result = FALSE;
    
    if(!OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&hToken)){
        letrace("cannot open access token of the thread");
        if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken)){
            letrace("cannot open access token of the process");
            return FALSE;
        }
    }
    
    if(!AllocateAndInitializeSid(&SystemSidAuthority,2,
      SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,
      0,0,0,0,0,0,&psid)){
        letrace("cannot create the security identifier");
        CloseHandle(hToken);
        return FALSE;
    }
      
    if(!CheckTokenMembership(NULL,psid,&IsMember)){
        letrace("cannot check token membership");
    } else {
        if(!IsMember){
            itrace("the user is not a member of administrators group");
        } else {
            result = TRUE;
        }
    }
    
    if(psid) FreeSid(psid);
    CloseHandle(hToken);
    return result;
}

/** @} */
