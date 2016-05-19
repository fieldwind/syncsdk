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

#include "base/oauth2/OAuth2JsonParser.h"
#include "base/Log.h"
#include "base/util/utils.h"
#include <string.h>

BEGIN_FUNAMBOL_NAMESPACE

OAuth2JsonParser::OAuth2JsonParser()
{
    errorCode.reset();
    errorMessage.reset();
}   

OAuth2JsonParser::~OAuth2JsonParser() {}



StringBuffer OAuth2JsonParser::formatOAuth2CredentialData(const char* accessCode, const char* refreshCode, const char* clientType, 
                                       const int accessCodeExpiresInSecs, const time_t accessTokenSetTime) 
{
    time_t now = time(NULL);
     
    bool accessTokenValid = true;
    if ((accessTokenSetTime + accessCodeExpiresInSecs) < now) {
        LOG.info("OAuth2 access code expired: resfresh tokens required");
        accessTokenValid = false;
    }
       
    OAuth2Credentials oauth2Credentials(accessCode, refreshCode, clientType, accessTokenValid);
    char* oauth2CredentialsJson = NULL;   

    OAuth2JsonParser jsonParser;
    if (jsonParser.formatOAuth2Credentials(oauth2Credentials, &oauth2CredentialsJson, true) == false) {
        LOG.error("%s: error formatting oauth2 access token object", __FUNCTION__);
        return "";
    } 

    StringBuffer jsonAuthTokenEncoded;
    b64_encode(jsonAuthTokenEncoded, oauth2CredentialsJson, strlen(oauth2CredentialsJson));    
    return jsonAuthTokenEncoded;
}

OAuth2Credentials OAuth2JsonParser::parseOAuth2CredentialData(const char* data)
{
    OAuth2Credentials results;
    StringBuffer oauth2JsonEncoded = data;
    if (oauth2JsonEncoded.empty()) {
        LOG.error("%s: missing or empty oauth2 credentials from Server", __FUNCTION__);
        return results;
    }

    char* oauth2JsonDecoded = NULL;
    int oauth2JsonDecodedLen;
        
    if ((oauth2JsonDecoded = (char *)b64_decode(oauth2JsonDecodedLen, oauth2JsonEncoded.c_str())) == NULL) {
        LOG.error("%s: can't decode oauth2 token from Server", __FUNCTION__);
        return results;
    }

    EOAuth2Status errorCode = EOAuth2NoError;
    OAuth2JsonParser jsonParser;
    if (jsonParser.parseOAuth2Credentials(oauth2JsonDecoded, results, &errorCode) == false) {
            LOG.error("%s: error parsing OAuth2 credentials from Server", __FUNCTION__);  
    } 

    delete [] oauth2JsonDecoded;
    return results;
}


bool OAuth2JsonParser::formatOAuth2Credentials(const OAuth2Credentials& accessCredentials, char **jsonOAuth2AccessCredentials, bool prettyPrint)
{
    cJSON *root = NULL,
          *data = NULL;
    
    std::string access_token, refresh_token, client_type;
    bool valid = accessCredentials.isValid();
    
    *jsonOAuth2AccessCredentials = NULL;
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
 
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        cJSON_Delete(root);
  
        return false;
    }
    
    access_token = accessCredentials.getAccessToken();
    if (access_token.empty()) {
        cJSON_Delete(root);
        cJSON_Delete(data);
  
        return false;
    }
    
    cJSON_AddStringToObject(data, "accesstoken", access_token.c_str());
    cJSON_AddStringToObject(data, "valid", valid ? "true" : "false");
    
    refresh_token = accessCredentials.getRefreshToken();
    if (refresh_token.empty()) {
        cJSON_Delete(root);
        cJSON_Delete(data);
        
        return false;
    }
    
    cJSON_AddStringToObject(data, "refreshtoken", refresh_token.c_str());
    
    client_type = accessCredentials.getClientType();
    if (client_type.empty()) {
        cJSON_Delete(root);
        cJSON_Delete(data);
  
        return false;
    }
    
    cJSON_AddStringToObject(data, "platform", client_type.c_str());
    cJSON_AddItemToObject(root, "data", data);
  
    if (prettyPrint) {
        *jsonOAuth2AccessCredentials = cJSON_Print(root);
    } else {
        *jsonOAuth2AccessCredentials = cJSON_PrintUnformatted(root);
    }
   
    cJSON_Delete(root);
   
    if ((*jsonOAuth2AccessCredentials == NULL) || (strlen(*jsonOAuth2AccessCredentials) == 0)) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);

        return false;
    }
    
    //LOG.debug("%s: formatted JSON object for oauth2 request: %s", __FUNCTION__,
    //    *jsonOAuth2AccessCredentials);
    LOG.debug("%s: formatted JSON object for oauth2 request: ****", __FUNCTION__);

    return true;
}

bool OAuth2JsonParser::parseOAuth2Credentials(const char* accessTokenJson, OAuth2Credentials& oauth2Credentials, OAuth2ParserStatus* errCode)
{
    cJSON *root = NULL,
          *error = NULL,
          *data = NULL,
          *access_token = NULL,
          *refresh_token = NULL,
          *expires_in = NULL,
          *scope_token = NULL;
    
    *errCode = EOAuth2NoError;
    
    if ((accessTokenJson == NULL) || (strlen(accessTokenJson) == 0)) {
        *errCode = EOAuth2InvalidMessage;
        return false;
    }
    
    if ((root = cJSON_Parse(accessTokenJson)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = EOAuth2ParseError;
        return false;
    }
    
    if ((error = cJSON_GetObjectItem(root, "error")) != NULL) {
        const char* errorMsg = error->valuestring;
        
        LOG.error("%s: error in OAuth2 json object: %s", __FUNCTION__, errorMsg);
        cJSON_Delete(root);
        *errCode = EOAuth2NoError;
        return false;
    }

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: error in OAuth2 json object: cannot find 'data'", __FUNCTION__);
        return false;
    }
    
    if ((access_token = cJSON_GetObjectItem(data, "accesstoken")) != NULL) {
        const char* access_token_val = access_token->valuestring;
        
        if (access_token_val) {
            oauth2Credentials.setAccessToken(access_token_val);
        }
    }
    
    if ((refresh_token = cJSON_GetObjectItem(data, "refreshtoken")) != NULL) {
        const char* refresh_token_val = refresh_token->valuestring;
        
        if (refresh_token_val) {
            oauth2Credentials.setRefreshToken(refresh_token_val);
        }
    }
    
    if ((expires_in = cJSON_GetObjectItem(data, "expiresin")) != NULL) {
        const char* expires_in_val = expires_in->valuestring;
        
        if (expires_in_val) {
            int expiresNumVal = atoi(expires_in_val);
            oauth2Credentials.setExpiresIn(expiresNumVal);
        }
    }
    
    if ((scope_token = cJSON_GetObjectItem(data, "scope")) != NULL) {
        const char* scope_token_val = scope_token->valuestring;
        
        if (scope_token_val) {
            oauth2Credentials.setScope(scope_token_val);
        }
    }
    
    cJSON_Delete(root);

    return true;
}

END_FUNAMBOL_NAMESPACE
