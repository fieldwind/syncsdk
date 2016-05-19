/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2007 Funambol, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License version 3 as published by
 * the Free Software Foundation with the addition of the following permission 
 * added to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED
 * WORK IN WHICH THE COPYRIGHT IS OWNED BY FUNAMBOL, FUNAMBOL DISCLAIMS THE 
 * WARRANTY OF NON INFRINGEMENT  OF THIRD PARTY RIGHTS.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License 
 * along with this program; if not, see http://www.gnu.org/licenses or write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 * 
 * You can contact Funambol, Inc. headquarters at 1065 East Hillsdale Blvd., 
 * Ste.400, Foster City, CA 94404 USA, or at email address info@funambol.com.
 * 
 * The interactive user interfaces in modified source and object code versions
 * of this program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU Affero General Public License version 3.
 * 
 * In accordance with Section 7(b) of the GNU Affero General Public License
 * version 3, these Appropriate Legal Notices must retain the display of the
 * "Powered by Funambol" logo. If the display of the logo is not reasonably 
 * feasible for technical reasons, the Appropriate Legal Notices must display
 * the words "Powered by Funambol".
 */


#ifndef INCL_POSIX_BUILD_ADAPTER
#define INCL_POSIX_BUILD_ADAPTER
/** @cond DEV */

#include "base/globalsdef.h"

// Enable the fix for encoding when building for posix
#define VOCL_ENCODING_FIX

BEGIN_NAMESPACE



template <typename T>
inline T min(T x, T y) {
    return ( (x) < (y) ? (x) : (y) );
}

template <typename T>
inline T max(T x, T y) {
    return ( (x) > (y) ? (x) : (y) );
}
    
END_NAMESPACE

//#define min(x,y) ( (x) < (y) ? (x) : (y) )
//#define max(x,y) ( (x) > (y) ? (x) : (y) )

#ifdef USE_WCHAR

BEGIN_NAMESPACE

// FIXME: remove this and adapt VOCL.
WCHAR *wcstok(WCHAR *s, const WCHAR *delim);

inline int _wtoi(const WCHAR *s) { return (int)wcstol(s, NULL, 10); }

END_NAMESPACE

#define _wcsicmp wcscasecmp
#define wcsicmp wcscasecmp


#if defined(MAC) || defined(FUN_IPHONE)

#   undef  _wcsicmp
#   undef  wcsicmp



inline int wcsicmp(const WCHAR * s1, const WCHAR * s2)
{
    const WCHAR * i1 = s1;
    const WCHAR * i2 = s2;
    
    while (i1 != NULL && i2 != NULL)
    {
        if (*i1 > *i2)
            return 1;
        if (*i1 < *i2)
            return -1;
            
        i1++;
        i2++;
    }
    if (i1 == NULL && i2 == NULL)
        return 0;
    if (i1 == NULL)
        return -1;
    if (i2 == NULL)
        return 1;
    // Should never happen
    return 0;
}

#   define _wcsicmp wcsicmp
#endif

#else

/* map WCHAR and its functions back to standard functions */

#       define SYNC4J_LINEBREAK "\n"

#include <stdio.h>
#include <stdlib.h>

inline int wsprintf(char* buf, const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    return vsprintf(buf, fmt, ap);
}

inline int wprintf(const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    return vprintf(fmt, ap);
}

inline FILE* _wfopen(const char* filename, const char* mode) {
    return fopen(filename, mode);
}

inline int fwprintf(FILE* file, const char* fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    return vfprintf(file, fmt, ap);
}

inline int swprintf(char *buf, size_t size, const char * fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    return vsnprintf(buf, size, fmt, ap);
}


inline int snwprintf(char *buf, size_t size, const char * fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    return vsnprintf(buf, size, fmt, ap);
}

inline char* wcscpy(char *dst, const char *src) {
    return strcpy(dst, src);
}

inline char* wcsncpy(char* dst, const char* src, size_t sz) {
    return strncpy(dst, src, sz);
}

inline int wcsncmp(const char* s1, const char* s2, size_t sz) {
    return strncmp(s1, s2, sz);
}

inline size_t wcslen(const char* s) {
    return strlen(s); 
}

inline int wcscmp(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

inline char* wcsstr(const char* s1, const char* s2) {
    return strstr(s1, s2);
}


//inline long wcstol(const char* s, const char** res, int i) {
//    return strtol(s, res, i);
//}


//inline unsigned long wctoul(const char* s, const char** res, int i) {
//    return strtoul(s, res, i);
//}

inline char* wcstok(char* s1, const char* s2) {
    return strtok(s1, s2);
}

inline char towlower(char x) { return tolower(x); }
inline char towupper(char x) { return toupper(x); }


inline char* wschr(const char* s, int p) {
    return strchr(s, p);
}

inline char* wcsrchr(const char* s, int p) {
    return strrchr(s, p);
}

inline char* wcscat(char* dst, const char* src) {
    return strcat(dst, src);
}

inline char* wcsncat(char* dst, const char* src, size_t s) {
    return strncat(dst, src, s);
}

inline int _wtoi(const char* s) {
    return atoi(s);
}

inline double wcstod(const char* s1, char** s2) {
    return strtod(s1, s2);
}

inline char* wcschr(const char* s, int ch) {
    return strchr(s, ch);
    
}

inline int _wcsicmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

inline int wcsicmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

inline int _stricmp(const char* s1, const char* s2) {
    return strcasecmp(s1, s2);
}

/*
#       define wmemmove memmove
#       define wmemcpy memcpy
#       define wmemcmp memcmp
#       define wmemset memset
#       define wctoul strtoul

#       define snwprintf snprintf
#define wcstol strtol

 
 */

#endif

#endif

