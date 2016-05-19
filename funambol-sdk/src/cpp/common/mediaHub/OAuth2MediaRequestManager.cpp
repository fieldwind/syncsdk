/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2013 Funambol, Inc.
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

#include "MediaHub/OAuth2MediaRequestManager.h"
#include <string>
#include <map>
#include "MediaHub/MHMediaJsonParser.h"
#include "MediaHub/MHItemJsonParser.h"
#include "http/HttpConnection.h"
#include "http/BasicAuthentication.h"
#include "http/URL.h"
#include "http/BasicAuthentication.h"
#include "ioStream/BufferInputStream.h"
#include "ioStream/StringOutputStream.h"
#include "ioStream/BufferOutputStream.h"
#include "event/FireEvent.h"
#include "base/oauth2/OAuth2JsonParser.h"
#include "base/oauth2/OAuth2Credentials.h"

BEGIN_FUNAMBOL_NAMESPACE

static const char* SapiLoginUrlFmt    = "%s/sapi/login?action=login&responsetime=true&syncdeviceid=%s";
static const char* SapiLogoutUrlFmt   = "%s/sapi/login?action=logout&responsetime=true";
static const char* ServerHeaderAuthorizationType = "oauth";

OAuth2MediaRequestManager::OAuth2MediaRequestManager(const char* url,
                                             const char* sapiSourceUri,
                                             const char* sapiArrayKey,
                                             const char* orderFieldValue,
                                             MHItemJsonParser* itemParser,
                                             AbstractSyncConfig* config_) : MHMediaRequestManager(url, sapiSourceUri, sapiArrayKey,
                                                                                        orderFieldValue, itemParser, config_)
{

}


OAuth2MediaRequestManager::OAuth2MediaRequestManager(const char* url, AbstractSyncConfig* config_) : MHMediaRequestManager(url, config_)
{
}

OAuth2MediaRequestManager::~OAuth2MediaRequestManager()
{
}

//
// authentication methods
//
EMHMediaRequestStatus OAuth2MediaRequestManager::login(const char* device_id, time_t* serverTime, unsigned long * expiretime, StringMap* sourcesStringMap, StringMap* propertyStringMap)
{
    int status = 0;
    StringOutputStream response;            // sapi response buffer
    StringBuffer loginUrl;
    URL requestUrl;
    const char* MHLoginResponse = NULL;
    ESMPStatus parserStatus;
    StringBuffer savedSessionId = sessionID; // save session id for MHSessionAlreadyOpen errors
    
    const char* accessCode     = config->getOAuth2AccessToken();
    const char* refreshCode    = config->getOAuth2RefreshToken();
    const char* clientType     = config->getOAuth2ClientType();
    int accessCodeExpires = atoi(config->getOAuth2ExpiresIn());
    time_t accessTokenSetTime  = config->getOAuth2AccessTokenSetTime();

    resetValidationKey();
    
    // check params
    if ((device_id == NULL) || (strlen(device_id) == 0)) {
        LOG.error("%s: invalid device id parameter for sapi login", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    

    // Urlencode the deviceId parameter (may contain unacceptable chars)
    const char* deviceIdEncoded = URL::urlEncode(device_id);
    loginUrl.sprintf(SapiLoginUrlFmt, serverUrl.c_str(), deviceIdEncoded);
    free((void *)deviceIdEncoded);
    
    // Request additional "details" for Service profiling
    if (sourcesStringMap != NULL && propertyStringMap != NULL) {
        loginUrl.append("&details=true");
    }

    requestUrl = loginUrl.c_str();
    LOG.debug("Performing oauth2 login with url: %s", loginUrl.c_str());
    
    httpConnection->setKeepAlive(false);
    httpConnection->setRequestHeader(HTTP_HEADER_ACCEPT,      "*/*");
    httpConnection->setAuthentication(NULL);

    // set the JSESSIONID cookie even in the Login SAPI (bug 14065)
    // It is necessary for resume uploads to work (otherwise we may be redirected to another server)
    if (!sessionID.empty()) {
        StringBuffer sessionIdCookie;
        sessionIdCookie.sprintf("JSESSIONID=%s", sessionID.c_str());
        httpConnection->setRequestHeader(HTTP_HEADER_COOKIE, sessionIdCookie.c_str());
    }
    
    if ((status = httpConnection->open(requestUrl, AbstractHttpConnection::MethodPost, false)) != 0) {
        LOG.error("%s: error opening connection", __FUNCTION__);
        return ESMRConnectionSetupError; // malformed URI, etc
    }

    OAuth2JsonParser jsonParser;
    StringBuffer credentialsEncoded = jsonParser.formatOAuth2CredentialData(accessCode, refreshCode, clientType, accessCodeExpires, accessTokenSetTime);
    if (credentialsEncoded.empty()) {
        LOG.error("%s: could not set oauth2 session auth params for SAPI Login request", __FUNCTION__);
        
        return ESMRMHMessageFormatError;
    }
        
    StringBuffer oAuth2HeaderValue;
    oAuth2HeaderValue.sprintf("oauth %s", credentialsEncoded.c_str());
    httpConnection->setRequestHeader(HTTP_HEADER_AUTHORIZATION, oAuth2HeaderValue.c_str());
    
    status = httpConnection->request(NULL, response, false);
    
    // always parse response headers
    lastHttpResponseHeaders = httpConnection->getResponseHeaders();
    processResponseHeaders();

    if (status  != HTTP_OK) {
        httpConnection->close();
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    httpConnection->close();
        
    if ((MHLoginResponse = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi login response", __FUNCTION__);
        return ESMRMHInvalidResponse;
    }
    
    //LOG.debug("response returned = %s", MHLoginResponse);       // don't log it (validation key is here!)
    
    int loginStatusCode = 0;
    // get session ID from JSON object
    if ((status = jsonMHMediaObjectParser->parseLogin(MHLoginResponse, sessionID, validationKey, serverTime, &parserStatus, expiretime, sourcesStringMap, propertyStringMap, &loginStatusCode)) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi login response", __FUNCTION__);
       
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        return ESMRMHMessageParseError;
    }
    
    config->setSessionId(sessionID.c_str());
    config->setSessionIdSetTime(time(NULL));
    config->setValidationKey(validationKey);    // will be encrypted
    
    // LOG.debug("%s: sapi session id: \"%s\"", __FUNCTION__, sessionID.c_str());
    LOG.debug("%s: sapi session id: ******", __FUNCTION__);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus OAuth2MediaRequestManager::login(time_t* serverTime)
{
    EMHMediaRequestStatus requestStatus = ESMRSuccess;
    
    pthread_mutex_lock(&sessionIDAccessMutex);
    requestStatus = login(deviceID, serverTime);
    pthread_mutex_unlock(&sessionIDAccessMutex);
    
    return requestStatus;
}

EMHMediaRequestStatus OAuth2MediaRequestManager::logout()
{
    int status = 0;
    StringOutputStream response;            // sapi response buffer
    StringBuffer logoutUrl;
    URL requestUrl;
    const char* MHLogoutResponse = NULL;
    
    logoutUrl.sprintf(SapiLogoutUrlFmt, serverUrl.c_str());
    
    requestUrl.setURL(logoutUrl.c_str());
    
    httpConnection->setKeepAlive(false);
    httpConnection->setRequestHeader(HTTP_HEADER_ACCEPT,      "*/*");
    
    setRequestSessionId();
    setRequestAuthentication();
    
    if ((status = httpConnection->open(requestUrl, HttpConnection::MethodGet)) != 0) {
        LOG.error("%s: error opening connection", __FUNCTION__);
        
        return ESMRConnectionSetupError; // malformed URI, etc
    }
    
    if ((status = httpConnection->request(NULL, response)) != HTTP_OK) {
        httpConnection->close();
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    httpConnection->close();
        
    if ((MHLogoutResponse = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi logout response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    config->setSessionId("");
    config->setSessionIdSetTime(0);
  
    LOG.debug("response returned = %s", MHLogoutResponse);
    
    return ESMRSuccess;
}

void OAuth2MediaRequestManager::setRequestAuthentication()
{
    if (httpConnection) {
        LOG.debug("%s: setting OAuth2 authorization header to http request", __FUNCTION__);

        const char* accessCode    = config->getOAuth2AccessToken();
        const char* refreshCode   = config->getOAuth2RefreshToken();
        const char* clientType    = config->getOAuth2ClientType();
        time_t accessTokenSetTime = config->getOAuth2AccessTokenSetTime();
        int accessCodeExpires     = atoi(config->getOAuth2ExpiresIn());

        OAuth2JsonParser jsonParser;
        StringBuffer credentialsEncoded = jsonParser.formatOAuth2CredentialData(accessCode, refreshCode, clientType, accessCodeExpires, accessTokenSetTime);
        if (credentialsEncoded.empty()) {
            LOG.error("%s: could not set oauth2 session auth params for SAPI request", __FUNCTION__);
            return;
        }

        StringBuffer oAuth2HeaderValue;
        oAuth2HeaderValue.sprintf("oauth %s", credentialsEncoded.c_str());
        httpConnection->setRequestHeader(HTTP_HEADER_AUTHORIZATION, oAuth2HeaderValue.c_str());
    }
}

void OAuth2MediaRequestManager::processResponseHeaders()
{
    StringBuffer serverAuthorizationHeader = lastHttpResponseHeaders[HTTP_HEADER_AUTHORIZATION];
    
    if (serverAuthorizationHeader.empty() == false) {
        size_t oauth2TokenIndex = serverAuthorizationHeader.find(ServerHeaderAuthorizationType);
        
        if (oauth2TokenIndex == StringBuffer::npos) {
            LOG.error("%s: can't find oauth2 token in server authorization header response",
                __FUNCTION__);
            
            return;
        }
        
        size_t serverHeaderAuthorizationTypeLen = strlen(ServerHeaderAuthorizationType);
        size_t serverHeaderOAuthTokenStartIndex = oauth2TokenIndex + serverHeaderAuthorizationTypeLen + 1;
        
        if (serverHeaderOAuthTokenStartIndex >= serverAuthorizationHeader.length()) {
            LOG.error("%s: can't find oauth2 token in server authorization header response",
                __FUNCTION__);
            
            return;
        }

        OAuth2JsonParser jsonParser;
        OAuth2Credentials oauth2Credentials = jsonParser.parseOAuth2CredentialData(serverAuthorizationHeader.substr(serverHeaderOAuthTokenStartIndex));
        
        // update config with new values
        std::string oauth2AccessToken  = oauth2Credentials.getAccessToken();
        std::string oauth2RefreshToken = oauth2Credentials.getRefreshToken();
        int oauth2ExpiresIn            = oauth2Credentials.getExpiresIn();

        if ((oauth2AccessToken.empty() == false) && (oauth2RefreshToken.empty() == false) && (oauth2ExpiresIn > 0)) {
            LOG.info("Successfully refreshed OAuth2 tokens (parsed from server response): set to config");
            config->setOauth2Params(oauth2Credentials, time(NULL));
        } else {
            LOG.error("%s: invalid OAuth2 credentials in server response header", __FUNCTION__);
        }
    }
}


END_FUNAMBOL_NAMESPACE
