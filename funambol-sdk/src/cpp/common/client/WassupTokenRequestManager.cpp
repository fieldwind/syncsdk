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

#include "WassupTokenRequestManager.h"
#include "base/globalsdef.h"
#include "StringOutputStream.h"
#include "base/util/XMLProcessor.h"
#include "base/util/StringBuffer.h"


USE_NAMESPACE

/*
 Paramenters:
                WassupURI: wassup service url -> the caller should retrieve it from customization parameters
                WassupUsrParam: username parameter name (e.g. wt-email=) -> the caller should retrieve it from customization parameters
                WassupPwdParam: password parameter name (e.g. wt-pwd=) -> the caller should retrieve it from customization parameters
                WassupAdditionalParam: additional parameters of the Wassup http request -> the caller should retrieve it from customization parameters
                config: client config instance needed to retrieve the user agent
 */
WassupTokenRequestManager::WassupTokenRequestManager(const char* WassupURI,
                                                     const char* WassupUsrParam,
                                                     const char* WassupPwdParam,
                                                     const char* WassupAdditionalParam,
                                                     const DMTClientConfig& config):
                                                            _wassupURI(WassupURI), _wassupUsrParam(WassupUsrParam), _wassupPwdParam(WassupPwdParam), _wassupAdditionalParam(WassupAdditionalParam)
{
    setUserAgent(config.getUserAgent());
    verifyServerSSL = config.getSSLVerifyServer();
}
WassupTokenRequestManager::~WassupTokenRequestManager()
{
}

/*
    Returns the token for wassup based authentication or an empty string if errors occur
    Parameters:
                username: as inserted by the user
                password: as inserted by the user
                err: output flag to check for errors
 */
std::string WassupTokenRequestManager::getToken(std::string username, std::string password, bool *err, int* requestCode)
{
    std::string token = "";
    *err = false;
    *requestCode = HTTP_OK;
    
    //request token over http
    LOG.debug("Getting wassup token");
    
    StringOutputStream response;
    HttpConnection *httpConnection = NULL;
    URL requestUrl;

    std::string formattedURL(_wassupURI);
    std::stringstream ss;
    std::string xmlResponse;

    //URL encode usr and pwd
    const char * usernameEncoded = URL::urlEncode(username.c_str());
    const char * passwordEncoded = URL::urlEncode(password.c_str());
    
    ss << _wassupURI << "?" << _wassupUsrParam << usernameEncoded << "&" << _wassupPwdParam << passwordEncoded << "&" << _wassupAdditionalParam;
    
    formattedURL = ss.str();
    
    requestUrl.setURL(formattedURL.c_str());
    
    httpConnection = new HttpConnection(_userAgent);
    httpConnection->setSSLVerifyServer(verifyServerSSL);
    
    if (httpConnection->open(requestUrl, HttpConnection::MethodGet, false)!= 0) {
        LOG.error("%s: error opening connection", __FUNCTION__);
        *err = true;
        *requestCode = -1;
        delete httpConnection;
   
        return "";
    } else {
        int requestStatus = HTTP_OK;
        
        if ((requestStatus = httpConnection->request(NULL, response, false)) != HTTP_OK) {
            LOG.error("%s: error sending Wassup access token request", __FUNCTION__);
            *err = true;
            if ((requestStatus == HttpConnection::StatusNetworkError) || 
                (requestStatus == HttpConnection::StatusReadingError) ||
                (requestStatus == HttpConnection::StatusTimeoutError) ||
                (requestStatus == HttpConnection::StatusWritingError)) {
                *requestCode = -1;
            } else {
                *requestCode = requestStatus;
            }
            
            httpConnection->close();
            delete httpConnection;
   
            return "";
        } else {
            xmlResponse.assign(response.getString().c_str());
            //LOG.debug("Wassup access token request response received: %s", xmlResponse.c_str());
        }
    }
    
    httpConnection->close();
    delete httpConnection;
    
    //parse response to extract token
    unsigned int startPos = 0;
    unsigned int endPos   = 0;
    bool found = false;
    while(!found & !*err)
    {
        if(XMLProcessor::getElementAttributes(xmlResponse.c_str(), "ident", &startPos, &endPos)==NULL)
        {
            LOG.error("%s: error parsing XML response", __FUNCTION__);
            *err = true;
        }
        else
        {
            std::string attributeName = xmlResponse.substr(startPos+6, 6);
            int tokenStartIndex = startPos + 21;
            int tokenEndIndex   = endPos - 3;
            int tokenLenght     = tokenEndIndex - tokenStartIndex + 1;
            
            if (attributeName=="cooses") {
                token = xmlResponse.substr(tokenStartIndex, tokenLenght);
                found = true;
            }
            else//try next tag
            {
                xmlResponse = xmlResponse.substr(endPos+1);
            }
        }
    }
        
    return token;
}
