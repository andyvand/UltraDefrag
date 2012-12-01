/*
 *  WGX - Windows GUI Extended Library.
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
 * @file config.c
 * @brief Configuration.
 * @addtogroup Config
 * @{
 */

#include "wgx-internals.h"

typedef struct _escape_sequence {
    char c;
    char *sq;
} escape_sequence;

escape_sequence esq[] = {
    {'\\', "\\\\"},
    {'\a', "\\a" },
    {'\b', "\\b" },
    {'\f', "\\f" },
    {'\n', "\\n" },
    {'\r', "\\r" },
    {'\t', "\\t" },
    {'\v', "\\v" },
    {'\'', "\\\'" },
    {'\"', "\\\"" },
    {0,    NULL  }
};

/**
 * @brief Reads options from configuration file.
 * @details Configuration files must be written
 * in Lua language to be accepted.
 * @return TRUE indicates that configuration file
 * has been successfully processed.
 * @note 
 * - NULL can be passed as first parameter to
 * initialize options by their default values.
 * - Option name equal to NULL indicates the end of the table.
 * - WGX_CFG_EMPTY and WGX_CFG_COMMENT options are ignored.
 * - If option type is WGX_CFG_INT value buffer must point to
 * variable of type int. Default value is interpreted as
 * integer, not as pointer. value_length field is ignored.
 * - If otion type is WGX_CFG_STING, all fields of the structure
 * have an obvious meaning.
 */
BOOL WgxGetOptions(char *path,WGX_OPTION *table)
{
    lua_State *L;
    int status;
    const char *msg;
    int i;
    char *s;
    
    if(table == NULL)
        return FALSE;
    
    /* set defaults */
    for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_INT){
            /* copy default value to the buffer */
            *((int *)table[i].value) = (int)(DWORD_PTR)table[i].default_value;
        } else if(table[i].type == WGX_CFG_STRING){
            /* copy default string to the buffer */
            strncpy((char *)table[i].value,
                (char *)table[i].default_value,
                table[i].value_length);
            ((char *)table[i].value)[table[i].value_length - 1] = 0;
        }
    }
    
    if(path == NULL)
        return TRUE; /* nothing to update */
    
    L = lua_open();
    if(L == NULL){
        etrace("cannot initialize Lua library");
        return FALSE;
    }
    
    /* stop collector during initialization */
    lua_gc(L, LUA_GCSTOP, 0);
    luaL_openlibs(L);
    lua_gc(L, LUA_GCRESTART, 0);

    status = luaL_dofile(L,path);
    if(status != 0){
        etrace("cannot interprete %s",path);
        if(!lua_isnil(L, -1)){
            msg = lua_tostring(L, -1);
            if(msg == NULL) msg = "(error object is not a string)";
            etrace("%s",msg);
            lua_pop(L, 1);
        }
        lua_close(L);
        return FALSE;
    }
    
    /* search for variables */
    for(i = 0; table[i].name; i++){
        lua_getglobal(L, table[i].name);
        if(!lua_isnil(L, lua_gettop(L))){
            if(table[i].type == WGX_CFG_INT){
                *((int *)table[i].value) = (int)lua_tointeger(L, lua_gettop(L));
            } else if(table[i].type == WGX_CFG_STRING){
                s = (char *)lua_tostring(L, lua_gettop(L));
                if(s != NULL){
                    strncpy((char *)table[i].value,s,table[i].value_length);
                    ((char *)table[i].value)[table[i].value_length - 1] = 0;
                } else {
                    strcpy((char *)table[i].value,"");
                }
            }
        }
        /* important: remove received variable from stack */
        lua_pop(L, 1);
    }
    
    /*for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_EMPTY){
            trace(D"\n");
        } else if(table[i].type == WGX_CFG_COMMENT){
            trace(D"-- %s\n",table[i].name);
        } else if(table[i].type == WGX_CFG_INT){
            trace(D"%s = %i\n",table[i].name,*((int *)table[i].value));
        } else if(table[i].type == WGX_CFG_STRING){
            trace(D"%s = \"%s\"\n",table[i].name,(char *)table[i].value);
        }
    }*/
    
    /* cleanup */
    lua_close(L);
    return TRUE;
}

/**
 * @brief Saves options to configuration file.
 * Configuration files produced are actually 
 * Lua programs.
 * @note
 * - Option name equal to NULL inicates the end of the table.
 * - WGX_CFG_EMPTY option forces to insert an empty line.
 * - WGX_CFG_COMMENT inserts a comment with text passed in option's name.
 * - WGX_CFG_INT saves an integer number, on which its value points.
 * - WGX_CFG_STRING saves a string pointed by value field of the structure.
 */
BOOL WgxSaveOptions(char *path,WGX_OPTION *table,WGX_SAVE_OPTIONS_CALLBACK cb)
{
    FILE *f;
    char *msg;
    int i, result = 0;
    unsigned int j, k, n;
    char c;
    char *sq;
    
    if(path == NULL || table == NULL){
        if(cb != NULL)
            cb("WgxSaveOptions: invalid parameter");
        return FALSE;
    }
    
    f = fopen(path,"wt");
    if(f == NULL){
        msg = wgx_sprintf("Cannot open %s "
            "file: %s",path,_strerror(NULL));
        if(msg){
            etrace("%s",msg);
            if(cb) cb(msg);
        } else {
            etrace("Cannot open file: not enough memory!");
            if(cb) cb("Cannot open file: not enough memory!");
        }
        free(msg);
        return FALSE;
    }

    for(i = 0; table[i].name; i++){
        if(table[i].type == WGX_CFG_EMPTY){
            //trace(D"\n");
            result = fprintf(f,"\n");
        } else if(table[i].type == WGX_CFG_COMMENT){
            //trace(D"-- %s\n",table[i].name);
            result = fprintf(f,"-- %s\n",table[i].name);
        } else if(table[i].type == WGX_CFG_INT){
            //trace(D"%s = %i\n",table[i].name,*((int *)table[i].value));
            result = fprintf(f,"%s = %i\n",table[i].name,*((int *)table[i].value));
        } else if(table[i].type == WGX_CFG_STRING){
            //trace(D"%s = \"%s\"\n",table[i].name,(char *)table[i].value);
            /*result = fprintf(f,"%s = \"%s\"\n",table[i].name,(char *)table[i].value);*/
            result = fprintf(f,"%s = \"",table[i].name);
            if(result < 0)
                goto fail;
        
            n = strlen((char *)table[i].value);
            for(j = 0; j < n; j++){
                c = ((char *)table[i].value)[j];
                /* replace character by escape sequence when needed */
                sq = NULL;
                for(k = 0; esq[k].c; k++){
                    if(esq[k].c == c)
                        sq = esq[k].sq;
                }
                if(sq)
                    result = fprintf(f,"%s",sq);
                else
                    result = fprintf(f,"%c",((char *)table[i].value)[j]);
                if(result < 0)
                    goto fail;
            }
            
            result = fprintf(f,"\"\n");
        }
        if(result < 0){
fail:
            fclose(f);
            msg = wgx_sprintf("Cannot write to %s "
                "file: %s",path,_strerror(NULL));
            if(msg){
                etrace("%s",msg);
                if(cb) cb(msg);
            } else {
                etrace("Cannot write to file: not enough memory!");
                if(cb) cb("Cannot write to file: not enough memory!");
            }
            free(msg);
            return FALSE;
        }
    }

    /* cleanup */
    fclose(f);
    return TRUE;
}

/** @} */
