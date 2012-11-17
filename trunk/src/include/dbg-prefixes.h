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
 * @brief Prefixes for debugging messages.
 * @details These prefixes are intended for use
 * with messages passed to winx_dbg_print,
 * winx_dbg_print_ex, winx_dbg_print_header,
 * udefrag_dbg_print, WgxDbgPrint routines.
 * To keep logs clean and suitable for easy
 * analysis always use one of the prefixes
 * listed here.
 */

#ifndef _DBG_PREFIXES_H_
#define _DBG_PREFIXES_H_

#define I "INFO:  "  /* for general purpose progress information */
#define E "ERROR: "  /* for errors */
#define D "DEBUG: "  /* for debugging purposes */
/* after addition of new prefixes don't forget to adjust winx_dbg_print_header code */

#endif /* _DBG_PREFIXES_H_ */
