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

#include <algorithm>
#include "base/Log.h"
#include "base/util/utils.h"
#include "base/stringUtils.h"
#include "spds/DataTransformerFactory.h"
#include "spds/B64Encoder.h"
#include "spds/B64Decoder.h"
#include "spds/DESEncoder.h"
#include "spds/DESDecoder.h"
#include "base/globalsdef.h"

USE_NAMESPACE

using namespace std;



/**
 * Converts a string to lower-case.
 * String passed is modified.
 * @param s   [IN-OUT] the string to be converted
 */
void toLowerCase(string& s) {
    transform(s.begin(), s.end(), s.begin(), tolower);
}

/**
 * Converts a string to lower-case (wstring version).
 * String passed is modified.
 * @param s   [IN-OUT] the string to be converted
 */
void toLowerCase(wstring& s) {
    transform(s.begin(), s.end(), s.begin(), tolower);
}


/**
 * Replace all strings 'source' with 'dest' inside string 'dataString'.
 * @param source       the token to search
 * @param dest         the token to replace
 * @param dataString   [IN-OUT] the string to work on
 * @param startPos     [OPTIONAL] tha start position (default = 0)
 */
void replaceAll(const wstring& source, const wstring& dest, wstring& dataString, const int startPos) {

    if (startPos >= (int)dataString.length()) {
        return;
    }

    wstring::size_type pos = dataString.find(source, startPos);

    while (pos != wstring::npos) {
        dataString.replace(pos, source.length(), dest);
        pos += dest.length();
        pos = dataString.find(source, pos);
    }
}





/**
 * Gets the content of <tag>...</tag> inside the string 'xml'.
 * Returns 0 if tag found, 1 if not found.
 *
 * @param xml     : the input string to search inside
 * @param tag     : the tag we are looking for
 * @param content : [OUT] the content of tag (string passed by ref)
 * @param pos     : (optional) the starting position to search inside 'xml' (default = 0)
 * @return          0 if tag found (even if empty content), 1 if errors
 */
int getElementContent(const wstring& xml, const wstring& tag, wstring& content, unsigned int pos) {

    wstring openTag, closeTag, emptyTag;
    wstring::size_type start, end;
    content = L"";

    //
    // First try to find the empty element <tag/>
    //
    emptyTag  = L"<";
    emptyTag += tag;
    emptyTag += L"/>";

    start = xml.find(emptyTag, pos);
    if (start != wstring::npos) {
        return 0;  // found!
    }

    //
    // Now search for the <tag [attributes]>Content</tag>
    //
    openTag   = L"<";
    openTag  += tag;

    closeTag  = L"</";
    closeTag += tag;
    closeTag += L">";

    start = xml.find(openTag, pos);
    if (start != wstring::npos) {
        start += openTag.length();

        start = xml.find(L">", start);      // Ignore the possible attributes inside the openTag
        if (start != wstring::npos) {
            start += 1;

            end = xml.find(closeTag, start);
            if (end == wstring::npos) {
                goto not_found;
            }

            content = xml.substr(start, end-start);
        }
        else {
            goto not_found;
        }
    }

    else {
        goto not_found;
    }

    return 0;

not_found:

    return 1;
}



/**
 * Gets the content of <tag>...</tag> inside the string 'xml'.
 * Returns 0 if tag found, 1 if not found.
 *
 * @param xml     : the input string to search inside
 * @param tag     : the tag we are looking for
 * @param content : [OUT] the content of tag (string passed by ref)
 * @param pos     : the starting position to search inside 'xml'
 * @param start   : [OUT] the starting position of content found ('wstring::npos' if not found)
 * @param end     : [OUT] the ending position of content found ('wstring::npos' if not found)
 * @return          0 if tag found (even if empty content), 1 if errors
 * @overload        getElementContent(const wstring& xml, const wstring& tag, wstring& content, unsigned int pos)
 */
int getElementContent(const wstring& xml, const wstring& tag, wstring& content, const wstring::size_type pos, wstring::size_type& start, wstring::size_type& end) {

    wstring openTag, closeTag, emptyTag;
    start   = wstring::npos;
    end     = wstring::npos;
    content = L"";

    //
    // First try to find the empty element <tag/>
    //
    emptyTag  = L"<";
    emptyTag += tag;
    emptyTag += L"/>";

    start = xml.find(emptyTag, pos);
    if (start != wstring::npos) {
        start += emptyTag.length();
        end = start;
        return 0;  // found!
    }

    //
    // Now search for the <tag [attributes]>Content</tag>
    //
    openTag   = L"<";
    openTag  += tag;

    closeTag  = L"</";
    closeTag += tag;
    closeTag += L">";

    start = xml.find(openTag, pos);
    if (start != wstring::npos) {
        start += openTag.length();

        start = xml.find(L">", start);      // Ignore the possible attributes inside the openTag
        if (start != wstring::npos) {
            start += 1;

            end = xml.find(closeTag, start);
            if (end == wstring::npos) {
                goto not_found;
            }

            content = xml.substr(start, end-start);
        }
        else {
            goto not_found;
        }
    }

    else {
        goto not_found;
    }

    return 0;

not_found:

    return 1;
}





