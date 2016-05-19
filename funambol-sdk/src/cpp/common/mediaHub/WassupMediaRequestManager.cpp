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

#include "MediaHub/WassupMediaRequestManager.h"
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
#include "spds/SyncSourceConfig.h"

BEGIN_FUNAMBOL_NAMESPACE

static const char* SapiLoginUrlFmt    = "%s/sapi/login?action=login&responsetime=true&syncdeviceid=%s";
static const char* SapiLogoutUrlFmt   = "%s/sapi/login?action=logout&responsetime=true";

WassupMediaRequestManager::WassupMediaRequestManager(const char* url,
                                             const char* sapiSourceUri,
                                             const char* sapiArrayKey,
                                             const char* orderFieldValue,
                                             MHItemJsonParser* itemParser,
                                             AbstractSyncConfig* config_) : MHMediaRequestManager(url, sapiSourceUri, sapiArrayKey,
                                                                                        orderFieldValue, itemParser, config_)
{
    wassupAuthToken = config->getWassupAuthToken();
    credInfo = config->getCredInfo();
}


WassupMediaRequestManager::WassupMediaRequestManager(const char* url, AbstractSyncConfig* config_) : MHMediaRequestManager(url, config_)
{
    wassupAuthToken = config->getWassupAuthToken();
    credInfo = config->getCredInfo();
}

WassupMediaRequestManager::~WassupMediaRequestManager()
{
}

//
// sapi authentication methods
//
EMHMediaRequestStatus WassupMediaRequestManager::login(const char* device_id, time_t* serverTime, unsigned long * expiretime, StringMap* sourcesStringMap, StringMap* propertyStringMap)
{
    int status = 0;
    StringOutputStream response;            // sapi response buffer
    StringBuffer MHLoginUrl;
    URL requestUrl;
    const char* MHLoginResponse = NULL;
    ESMPStatus parserStatus;
    StringBuffer savedSessionId = sessionID; // save session id for MHSessionAlreadyOpen errors

    resetValidationKey();
    
    // check params
    if ((device_id == NULL) || (strlen(device_id) == 0)) {
        LOG.error("%s: invalid device id parameter for wassup login", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    // check class members 
    if ((wassupAuthToken == NULL) || (strlen(wassupAuthToken) == 0)) {
        LOG.error("%s: invalid parameters for wassup login", __FUNCTION__);
        
        return ESMRInvalidParam;
    }

    // Urlencode the deviceId parameter (may contain unacceptable chars)
    const char* deviceIdEncoded = URL::urlEncode(device_id);
    MHLoginUrl.sprintf(SapiLoginUrlFmt, serverUrl.c_str(), deviceIdEncoded);
    free((void *)deviceIdEncoded);
    
    // Request additional "details" for Service profiling
    if (sourcesStringMap != NULL && propertyStringMap != NULL) {
        MHLoginUrl.append("&details=true");
    }

    requestUrl = MHLoginUrl.c_str();
    LOG.debug("Performing login with url: %s", MHLoginUrl.c_str());
    
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
 
    const char* wassupAuthTokenEncoded = URL::urlEncode(wassupAuthToken);
    
    if ((wassupAuthTokenEncoded == NULL) || (strlen(wassupAuthTokenEncoded) == 0)) {
        httpConnection->close();
        LOG.error("%s: error encoding wassup authentication token", __FUNCTION__);

        return ESMRInvalidParam;
    }

    std::string authToken;
    authToken.append("login=").append(wassupAuthTokenEncoded).append("&password=desktop");;
    
    if (credInfo != NULL && strlen(credInfo) >0) {
        //LOG.debug("cred-info: %s", credInfo);
        const char* credInfoUE = URL::urlEncode(credInfo); 
        authToken.append("&cred-info=").append(credInfoUE);   
        
        free((void *)credInfoUE);
    }
    
    free((void *)wassupAuthTokenEncoded);
    
    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");
    
    if ((status = httpConnection->request(authToken.c_str(), response, false)) != HTTP_OK) {
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
    
    // LOG.debug("response returned = %s", MHLoginResponse);       // don't log it (validation key is here!)
    LOG.debug("response returned = ******");      

    int loginStatusCode = 0;
    // get session ID from JSON object
    if ((status = jsonMHMediaObjectParser->parseLogin(MHLoginResponse, sessionID, validationKey, serverTime, &parserStatus, expiretime, sourcesStringMap, propertyStringMap, &loginStatusCode)) == false) {
        if (loginStatusCode != 0) {
            LOG.error("%s: authentication failed with code: %d", __FUNCTION__, loginStatusCode);
            switch (loginStatusCode) {
                case 419:
                    return ESMRErrorCodeLoginFailure751;

                case 420:
                    return ESMRErrorCodeLoginFailure752;
                    
                case 421:
                    return ESMRErrorCodeLoginFailure753;
                    
                case 422:
                    return ESMRErrorCodeLoginFailure754;
                    
                case 423:
                    return ESMRErrorCodeLoginFailure755;

                case 424:
                    return ESMRErrorCodeLoginFailure756;

                case 425:
                    return ESMRErrorCodeLoginFailure757;
                    
                case 418:
                default:
                    return ESMRErrorCodeLoginFailure750;
            }
        } else {
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
            
            if (!strcmp(errorCode, MHMediaRequestManager::MHSecurityException)) {
                return ESMRSecurityException;
            } else if (!strcmp(errorCode, MHMediaRequestManager::MHUserIdMissing)) {
                return ESMRUserIdMissing;
            } else if (!strcmp(errorCode, MHMediaRequestManager::MHSessionAlreadyOpen)) {
                // LOG.debug("%s: session already opened - resetting session id %s", __FUNCTION__, savedSessionId.c_str());
                    sessionID = savedSessionId;
                return ESMRSuccess;
            } else if (!strcmp(errorCode, MHMediaRequestManager::MHInvalidAuthSchema)) {
                return ESMRInvalidAuthSchema;
            } 
            
            return ESMRMHMessageParseError;
        }
    }
    
    config->setSessionId(sessionID.c_str());
    config->setSessionIdSetTime(time(NULL));
    config->setValidationKey(validationKey);    // will be encrypted
    
    // LOG.debug("%s: sapi session id: \"%s\"", __FUNCTION__, sessionID.c_str());
    LOG.debug("%s: sapi session id: *****", __FUNCTION__);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus WassupMediaRequestManager::login(time_t* serverTime)
{
    EMHMediaRequestStatus requestStatus = ESMRSuccess;
    
    pthread_mutex_lock(&sessionIDAccessMutex);
    requestStatus = login(deviceID, serverTime);
    pthread_mutex_unlock(&sessionIDAccessMutex);
    
    return requestStatus;
}
//
// authentication methods
//


EMHMediaRequestStatus WassupMediaRequestManager::logout()
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

void WassupMediaRequestManager::setRequestAuthentication()
{
    
}

void WassupMediaRequestManager::processResponseHeaders()
{
    
}

void WassupMediaRequestManager::setUploadItemExtendedHeaders(UploadMHSyncItem* item)
{
    MHSyncItemInfo* iteminfo = item->getMHSyncItemInfo();
    std::string localItemPathstd = iteminfo->getLocalItemPath().c_str();
    //StringBuffer localItemPath = localItemPathstd.c_str();
    std::string path = getFileNormalizedFilePathForHeader(localItemPathstd);
    httpConnection->setRequestHeader("x-funambol-file-path", path.c_str());
}


std::string WassupMediaRequestManager::getFileNormalizedFilePathForHeader(std::string filePath) {
    
    std::string ret("");

    if (config == NULL || filePath.empty()) {
        return ret;
    }

    // bad code but it is too specific to be generalized...
    // the other source name are the same of the uri (apart audio that is music)
    StringBuffer sname = mhMediaSourceName;
    if (sname == "audio") {
        sname = "music";
    }

    SyncSourceConfig* ssconfig = (SyncSourceConfig*)config->getAbstractSyncSourceConfig(sname);

    if (ssconfig == NULL) {
        LOG.debug("%s - ssconfig is NULL for source %s", __FUNCTION__, sname.c_str());
        return ret;
    }

    const char* filelist = ssconfig->getProperty("itemsUploadDirectoriesList");
    
    StringBuffer buf(filelist);
    LOG.debug("%s - list of folders: %s", __FUNCTION__, buf.c_str());
    LOG.debug("%s - file to find: %s", __FUNCTION__, filePath.c_str());


    ArrayList tokens;
    buf.split(tokens, ",");
    bool found = false;
    std::string folderPath = "";
    size_t res = 0;
    for (int i = 0; i < tokens.size(); i++) {

        StringBuffer* tmp = (StringBuffer*)tokens[i];

        folderPath = tmp->c_str();
        if (folderPath.find_first_of("|") == 0 || folderPath.find_first_of("!") == 0) {
            folderPath = folderPath.substr(1);
        }

        if (folderPath.find_first_of("|") == 0 || folderPath.find_first_of("!") == 0) {
            folderPath = folderPath.substr(1);
        }

        res = filePath.find(folderPath.c_str());
        if (res != StringBuffer::npos) {           
            LOG.debug("%s - found file to submit: %s", __FUNCTION__, filePath.c_str());
            found = true;
            res = folderPath.size();
            break;
        }
    }

    if (found) {

        std::string onlyFilePath = filePath.substr(res);
        if (onlyFilePath.find_first_of("/") != 0) {
            onlyFilePath = onlyFilePath.insert(0, "/");
        }
        ret = onlyFilePath;
        /*
        std::string allpath = filePath.substr(0, res);
        std::string onlyFilePath = filePath.substr(res);
       
        size_t firstRightFound = allpath.rfind("/");
        size_t secondRightFound = 0;
        if (firstRightFound != StringBuffer::npos) {
            std::string s = allpath.substr(0, firstRightFound);
            secondRightFound = s.rfind("/");
        }        

        if (secondRightFound != StringBuffer::npos) {
            ret = allpath.substr(secondRightFound);
            ret += onlyFilePath;
        } else {
            ret = filePath;
        }    
        */
        LOG.debug("%s - found file to submit in the header: %s", __FUNCTION__, ret.c_str());
    }

    // convert b64 of path
    TransformationInfo info;
    info.size = ret.length();
    DataTransformer* b64Transformer = DataTransformerFactory::getEncoder(DT_B64);
    char* result = b64Transformer->transform((char*)ret.c_str(), info);
    ret = result;

    if (info.newReturnedData) {
        delete [] result;
    }

    return ret;
}


END_FUNAMBOL_NAMESPACE
