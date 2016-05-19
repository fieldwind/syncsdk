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

#include "base/fscapi.h"
#include "base/util/utils.h"
#include "base/util/StringBuffer.h"
#include "base/globalsdef.h"
#include "base/Log.h"
#include "base/oauth2/OAuth2JsonParser.h"
#include "base/oauth2/OAuth2Credentials.h"

#include <string>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <ostream>
#include <algorithm>
#include <iterator>

namespace Funambol {

/*
 * Deletes the given char[] buffer if it is not NULL
 * and sets the pointer to NULL
 *
 */
void safeDelete(char* p[]) {
    if (*p) {
        delete [] *p; *p = NULL;
    }
}

void safeDel(char** p) {
    if (*p) {
        delete [] *p; *p = NULL;
    }
}

char* stringdup(const char* s, size_t len)
{
    if ( !s )
        return NULL;

    int l = (len==STRINGDUP_NOLEN)?strlen(s):len;

    char* news = new char[l+1];

    strncpy(news, s, l);
    news[l]=0;

    return news;
}

WCHAR* wstrdup(const WCHAR* s, size_t len)
{
    if ( !s )
        return NULL;

    int l = (len==STRINGDUP_NOLEN)?wcslen(s):len;

    WCHAR* news = new WCHAR[l+1];

    wcsncpy(news, s, l);
    news[l]=0;

    return news;
}

char* strtolower(const char *s)
{
    char* l = NULL;
    char* p = NULL;

    for(l = p = stringdup(s); *p; ++p) {
        *p=tolower(*p);
    }
    return l;
}

char* strtoupper(const char *s)
{
    char* u = NULL;
    char* p = NULL;

    for(u = p = stringdup(s); *p; ++p) {
        *p=toupper(*p);
    }
    return u;
}


WCHAR* wcstolower(const WCHAR *s)
{
    WCHAR* l = NULL;
    WCHAR* p = NULL;

    for(l = p = wstrdup(s); *p; ++p) {
        *p=towlower(*p);
    }

    return l;
}

WCHAR* wcstoupper(const WCHAR *s)
{
    WCHAR* u = NULL;
    WCHAR* p = NULL;

    for(u = p = wstrdup(s); *p; ++p) {
        *p=towupper(*p);
    }

    return u;
}

/**
 * find a substring from the end, with optional string lenght
 */
const char *brfind(const char *s1, const char *s2, size_t len)
{
    const char *sc1, *sc2, *ps1;

    if (!s1)
        return NULL;

    if (*s2 == '\0')
        return s1;

    if(len < strlen(s1)){
        ps1 = s1 + len;
    }
    else {
        ps1 = s1 + strlen(s1);
    }

    while(ps1 > s1) {
        --ps1;
        for (sc1 = ps1, sc2 = s2; *sc1 != *sc2; sc1++, sc2++) {
            if (*sc2 == '\0')
                return (ps1);
        }
    }
    return NULL;
}


void timestampToAnchor(unsigned long timestamp, char anchor[21]) {
    sprintf(anchor, "%lu", timestamp);
}

unsigned long anchorToTimestamp(const char* anchor) {
    unsigned long timestamp;

    return sscanf(anchor, "%lu", &timestamp) == 1 ? timestamp : 0;
}

bool wcscmpIgnoreCase(const char* p, const char* q) {

    bool ret = false;
    if (p == NULL || q == NULL)
        return ret;

    unsigned int lenp = 0, lenq = 0;
    lenp = strlen(p);
    lenq = strlen(q);

    if (lenp != lenq) {
        return ret;
    }

    for (unsigned int i = 0; i < lenp; i++) {
        if ( towlower(p[i]) != towlower(q[i]))
            return ret;
    }
    ret = true;
    return ret;
}


char* itow(int i) {
    char* ret = new char[10];
    memset(ret, 0, 10*sizeof(char) );
    sprintf(ret, "%i", i);
    return ret;
}

char* ltow(long i) {
    char* ret = new char[20];
    memset(ret, 0, 20*sizeof(char));
    sprintf(ret, "%ld", i);
    return ret;
}

long long atoll(const char *instr)
{
    long long retval;
    
    retval = 0;
    for (; *instr; instr++) {
        retval = 10*retval + (*instr - '0');
    }
    return retval;
}


/*
* It implements algo for authentication with MD5 method.
* It computes digest token according with follow:
* Let H   : MD5 Function represents by calculateMD5 method
* Let B64 : Base64 encoding Function represents by encodeBase64 method
* Data: H (B64(H(username:password)):nonce)
*/

char* MD5CredentialData(const char* userName, const char* password, const char* nonce) {

    int len = 0, lenNonce = 0, totLen = 0;

    char cnonce      [64];
    char digest      [16];
    char base64      [64];
    char base64Nonce [64];
    char token      [512];
    char* md5Digest = NULL;
    char ch          [3];

    memset(digest,      0, 16);
    memset(base64,      0, 64);
    memset(base64Nonce, 0, 64);
    memset(cnonce,      0, 64);
    memset(token,       0, 512);
    sprintf(ch, ":");

    sprintf(token, "%s:%s", userName, password);
    len = strlen(token);

    // H(username:password)
    calculateMD5((void*)token, len, digest);

    // B64(H(username:password))
    len = b64_encode((char*)base64, digest, 16);


    // decode nonce from stored base64 to bin
    strcpy(cnonce, nonce);
    lenNonce = b64_decode(cnonce, cnonce);

    memcpy(base64Nonce, base64, len);
    memcpy(&base64Nonce[len], ch, 1);
    memcpy(&base64Nonce[len+1], cnonce, lenNonce);

    totLen = len + 1 + lenNonce;

    memset(digest, 0, 16);
    calculateMD5(base64Nonce, totLen, digest);
    b64_encode(base64, digest, 16);

    // return new value
    md5Digest = stringdup(base64);
    return md5Digest;
}



char* calculateMD5(const void* token, int len, char* wdigest) {

    //algo for md5 digest
    char dig [18];
    funambol_md5_state_t state;
    funambol_md5_byte_t digest[16];
    int di;
    char* ret = NULL;

    funambol_md5_init  (&state);
    funambol_md5_append(&state, (const funambol_md5_byte_t *)token, len);
    funambol_md5_finish(&state, digest);
    for (di = 0; di < 16; ++di) {
        sprintf(dig + di, "%c", digest[di]);
    }
    if (wdigest == NULL) {
        ret = new char[16];
        memcpy(ret, dig, 16);
        return ret;
    } else {
        memcpy(wdigest, dig, 16);
        return NULL;
    }
}

int64_t fgetsize(FILE *f)
{
    if (!f) return 0;
    
    int64_t size;
    // int64_t pos = _ftelli64(f);
    int64_t pos = fileTell(f);

    fileSeek(f, 0, SEEK_END);
    size = fileTell(f);
    fileSeek(f, pos, SEEK_SET);
    
    /*_fseeki64(f, 0, SEEK_END);
    size = _ftelli64(f);
    _fseeki64(f, pos, SEEK_SET);
    */
    return size;
}

int64_t fgetsize(const char* fileName) {
    
    if (!fileName) return 0;
    
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    if (statFile(fileName, &st) < 0) {
        return 0;
    }
    return st.st_size;

// --old implementation--
//    if (fileName) {
//        FILE* f = fileOpen(fileName, "rb");
//        if (f) {
//            size_t size = fgetsize(f);
//            fclose(f);
//            return size;
//        }
//    }
//    return 0;
}

//Returns the file name, given its full (absolute path) name.
StringBuffer getFileNameFromPath(const StringBuffer& fullName) {
    
    StringBuffer fileName("");
    
    unsigned long pos = fullName.rfind("/");
    if (pos == StringBuffer::npos) {
        pos = fullName.rfind("\\");
        if (pos == StringBuffer::npos) {
            // fullName is already the file name
            return fullName;
        }
    }
    // Move to the first char of the filename
    pos += 1;
    
    fileName = fullName.substr(pos, fullName.length() - pos);
    return fileName;
}

StringBuffer getCompleteName(const char* dir, const StringBuffer& name) {
    
    if (name.find(dir) == 0) {
        // Filename contains the path from the first char -> it's already the complete name
        return name;
    }
    else {
        StringBuffer pathName(dir);
        pathName += "/"; 
        pathName += name;
        return pathName;
    }
}

StringBuffer getCompleteName(const char *dir, const WCHAR *name) {
    StringBuffer fileName;
    fileName.convert(name);
    return getCompleteName(dir, fileName);
}

bool checkFileExtension(const StringBuffer& fileName, const StringBuffer& extension, bool caseInsensitive)
{
    unsigned long pos = fileName.rfind(".");
    
    if (pos == StringBuffer::npos) {
        return false;
    }
    if (pos < fileName.length()) {
        pos += 1;
        StringBuffer ext = fileName.substr(pos, fileName.length() - pos);
        if (caseInsensitive) {
            if (ext.icmp(extension.c_str())) {
                return true;
            }
        } else {
            if (ext == extension) {
                return true;
            }
        }
    }
    return false;
}

bool checkFileExtension(const WCHAR* wfileName, const WCHAR* wextension, bool caseInsensitive) {

    if (!wfileName || !wextension) {
        return false;
    }
    StringBuffer fileName, extension;
    fileName.convert(wfileName);
    extension.convert(wextension);

    return checkFileExtension(fileName, extension, caseInsensitive);
}

std::string encodeForSaving(const char* value, const char* user_token) 
{
    std::string res("");

    if ((user_token != NULL) && (strlen(user_token) > 0)) {
        if (value == NULL || strlen(value) == 0) {
            return res; 
        }
        
        DataTransformer* enc = DataTransformerFactory::getEncoder(DT_DES);
        TransformationInfo info;
        info.size = (long)strlen(value)*sizeof(char);
        info.password = user_token;
        char* v = strdup(value);
        char* desValue = enc->transform(v, info);
        
        DataTransformer* b64Enc = DataTransformerFactory::getEncoder(DT_B64);
        char* b64Value = b64Enc->transform(desValue, info); 
        
        free(v);
        
        if (desValue) {
            free(desValue);
        }
        
        res = b64Value;
        delete [] b64Value;
    } else {
        res = value;
    }

    return res;
}

std::string decodeAfterReading(const char *value, const char* user_token) 
{
    std::string res(value);
    if ((user_token != NULL) && (strlen(user_token) > 0)) {
        if (value == NULL || strlen(value) == 0) {
            return "";
        }
        
        // Password is stored as b64(des(password))
        DataTransformer* b64Dec = DataTransformerFactory::getDecoder(DT_B64);
        TransformationInfo info;
        
        char* v = strdup(value);
        info.size = (long)strlen(v)*sizeof(char);
        char* b64DecValue = b64Dec->transform(v, info);
        
        DataTransformer* dec = DataTransformerFactory::getDecoder(DT_DES);
        info.password = user_token;
        
        char* desValue = dec->transform(b64DecValue, info);
        if (desValue != NULL && info.size > 0) {
            desValue[info.size] = 0;
        }
        
        free(v);
        
        if (desValue != NULL) {
            res = desValue;
        }
        delete [] b64DecValue;

        if (info.newReturnedData == true) {
            delete [] desValue;
        }               
    }         
    return res;          
}

bool applyFilterFromPatternsList(const std::string& value, const std::vector<std::string>& filterValuesList)
{
    bool match = false;
    
    if (value.empty() == false) {
        std::vector<std::string>::const_iterator it  = filterValuesList.begin(),
                                           end = filterValuesList.end();
                                           
        for (; it != end; it++) {
            std::string excluded_name_pattern = *it;
            size_t len = excluded_name_pattern.size();
            
            if (len > 0) {
                if (excluded_name_pattern[len - 1] == '*') {
                    // this is a very limited 'globbing' support: get the pattern from
                    // the start of the string till the '*' character and see if file 
                    // name starts with the same pattern
                    size_t glob_index = excluded_name_pattern.find('*');
                    std::string start_pattern = excluded_name_pattern.substr(0, glob_index);
                    size_t pattern_size = start_pattern.size();
                    
                    if (pattern_size > 0) {
                        if (strncmp(value.c_str(), start_pattern.c_str(), pattern_size) == 0) {
                            match = true;
                            break;
                        }
                    }
                } else {
                    if (value == excluded_name_pattern) {
                        match = true;
                        break;
                    }
                }
            }
        }
    }
    
    return match;
}

int splitString(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    int splitted = 0;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
        splitted++;
    }

    return splitted;
}

std::string spliceVectorToStringByDelim(const std::vector<std::string> &elts, const std::string& delim)
{
    std::ostringstream oss("");
    
    if (elts.empty() == false) {
        std::vector<std::string>::const_iterator it = elts.begin(), end = elts.end();
        std::copy(it, end - 1, std::ostream_iterator<std::string>(oss, delim.c_str()));

        oss << *(end - 1);
    }

    return oss.str();
}

} // end Funambol namespace
