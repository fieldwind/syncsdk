/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2011 Funambol, Inc.
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

#ifndef __OAUTH2_JSON_PARSER_H__
#define __OAUTH2_JSON_PARSER_H__

#include <vector>
#include "cJSON.h"

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"
#include "base/oauth2/OAuth2Credentials.h"

BEGIN_FUNAMBOL_NAMESPACE

typedef enum OAuth2ParserStatus
{
    EOAuth2NoError = 0,
    EOAuth2InvalidArgument,
    EOAuth2ParseError,
    EOAuth2InvalidMessage,
    EOAuth2KeyMissing,
    EOAuth2ValueMissing
} EOAuth2Status;


class OAuth2JsonParser
{
private:
    StringBuffer errorCode;
    StringBuffer errorMessage;

    bool parseOAuth2Credentials(const char* accessTokenJson, OAuth2Credentials& oauth2Credenials, OAuth2ParserStatus* errCode);
    bool formatOAuth2Credentials(const OAuth2Credentials& AccessCredentials, char **jsonOAuth2AccessCredentials, bool prettyPrint);

public:
    OAuth2JsonParser();
    virtual ~OAuth2JsonParser();

    StringBuffer formatOAuth2CredentialData(const char* accessCode, const char* refreshCode, const char* clientType, 
                                            const int accessCodeExpiresInSecs, const time_t accessTokenSetTime);

    OAuth2Credentials parseOAuth2CredentialData(const char* data);


    bool checkErrorMessages(const char* response, time_t* lastUpdate = NULL);

    StringBuffer& getErrorCode()     { return errorCode;    }
    StringBuffer& getErrorMessage()  { return errorMessage; }
};

END_FUNAMBOL_NAMESPACE

#endif
