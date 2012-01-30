/*
* This script redirects English (US) page to the translated one when it is reasonable.
* Copyright (C) 2011-2012 Dmitri Arkhangelski (dmitriar@gmail.com).
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/**
 * List of existing translations.
 */
var translations = ["en", "de", "fa", "ru"];

/**
 * Target language.
 */
var target_language = null;
var root = location.href.substring(0, location.href.lastIndexOf("/"));

/**
 * Retrieves the value of the cookie with the specified name.
 */
function get_cookie(name)
{
  var cookie = document.cookie.split("; ");
  for (i = 0; i < cookie.length; i++){
    var c = cookie[i].split("=");
    if (name == c[0]) 
      return unescape(c[1]);
  }
  return null;
}

/**
 * Redirects English (US) page to the translated one when it is reasonable.
 */
function redirect(page){
    /* Check for the last used language. */ 
    target_language = get_cookie("language");

    /* Check for user preferred language. */
    if (!target_language){
        var lang_code = navigator.language || navigator.systemLanguage;
        var lang = lang_code.toLowerCase(); lang = lang.substr(0,2);
        for (i = 0; i < translations.length; i++){
            if (lang == translations[i]){
                target_language = lang;
                break;
            }
        }
    }
    
    if(!target_language){
        target_language = "en";
    }

    /* Save preferred language and redirect page. */
    document.cookie = "language=" + target_language + "; path=/";
    var target = root + "/" + target_language + "/" + page;
    window.location.replace ? window.location.replace(target) : window.location = target;
}
