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
#include "http/constants.h"
#include "http/HTTPHeader.h"
#include "http/TransportAgent.h"
#include "base/util/KeyValuePair.h"
#include "base/util/utils.h"
#include "base/globalsdef.h"

const char* BEGIN_VCARD         = "BEGIN:VCARD";
const char* END_VCARD           = "END:VCARD";
const char* BEGIN_VCAL          = "BEGIN:VCALENDAR";
const char* END_VCAL            = "END:VCALENDAR";
const char* BEGIN_CRED          = "<Cred>";
const char* BEGIN_CRED_DATA     = "<Data>";
const char* END_CRED_DATA       = "</Data>";
const char* END_CRED            = "</Cred>";
const char* MOREDATA            = "<MoreData/>";


USE_NAMESPACE

TransportAgent::TransportAgent() {
    timeout = DEFAULT_MAX_TIMEOUT;
    maxmsgsize = DEFAULT_MAX_MSG_SIZE;
    readBufferSize = DEFAULT_INTERNET_READ_BUFFER_SIZE;
    compression = false;
    SSLServerCertificates = "";
    SSLVerifyServer = true;
    SSLVerifyHost = true;
    responseSize = 0;
    responseCode = -1;
    // Set the default content type in the SyncManager::initTransportAgent    
}

TransportAgent::TransportAgent(const URL& newURL,
                               Proxy& newProxy,
                               unsigned int timeout,
                               unsigned int maxmsgsize) {

    url = newURL;
    proxy.setProxy(newProxy);
    this->timeout = timeout;
    this->maxmsgsize  = maxmsgsize;
    readBufferSize = DEFAULT_INTERNET_READ_BUFFER_SIZE;
    compression = false;
    SSLServerCertificates = "";
    SSLVerifyServer = true;
    SSLVerifyHost = true;
    responseSize = 0;
    responseCode = -1;
    // Set the default content type in the SyncManager::initTransportAgent     
}


TransportAgent::~TransportAgent() {
}

void TransportAgent::setURL(const URL& newURL) {
    url = newURL;
}

URL& TransportAgent::getURL() {
    return url;
}

void TransportAgent::setTimeout(unsigned int t) {
    timeout = t;
}

unsigned int TransportAgent::getTimeout() {
    return timeout;
}

void TransportAgent::setMaxMsgSize(unsigned int t) {
    maxmsgsize = t;
}

unsigned int TransportAgent::getMaxMsgSize() {
    return maxmsgsize;
}

void TransportAgent::setReadBufferSize(unsigned int t) {
    readBufferSize = t;
}

unsigned int TransportAgent::getReadBufferSize() {
    return readBufferSize;
}

const char* TransportAgent::getResponseProperty(const char *pname) {
    return responseProperties[pname].c_str();
}

unsigned int TransportAgent::getResponseSize() {
	return responseSize;
}

void TransportAgent::setResponseCode(int respCode){
    responseCode = respCode;
}

int TransportAgent::getResponseCode() {
    return responseCode;
}

void TransportAgent::setUserAgent(const char* ua) {
    userAgent = ua;
}

const char* TransportAgent::getUserAgent() {
    return userAgent;
}


const char*  TransportAgent::getPhoneidentify() {
    return phoneidentify;
}
void TransportAgent::setPhoneidentify(const char*  v){
    phoneidentify = v;
}

const char*  TransportAgent::getTokenauth() {
    return tokenauth;
}
void TransportAgent::setTokenauth(const char*  v){
    tokenauth = v;
}


void TransportAgent::setCompression(bool newCompression){
    compression = newCompression;
}

bool TransportAgent::getCompression(){
    return compression;
}

void TransportAgent::setProperty(const char *propName, const char * const propValue){
    requestProperties.put(propName, propValue);
}

char* TransportAgent::query(ArrayList& httpHeaders, long* protocolResponseCode){
    responseCode = 0;
    return 0;
}

std::string TransportAgent::removeSensibleData(const char* msg) {

    std::string message = "";
    if (msg == NULL) {
        return message;
    }

    if (strlen(msg) == 0) {
        return message;
    }

    message = msg;

    for (int i = 0; i < 3; i++) {
        size_t startDataPosition = 0;
        size_t endDataPosition = 0;
        std::string beginType = BEGIN_VCARD;
        std::string endType = END_VCARD;
        
        if (i == 1) {
            beginType   = BEGIN_VCAL;
            endType     = END_VCAL;
        }
        if (i == 2) {
            beginType   = BEGIN_CRED;
            endType     = END_CRED;
        }
        
        //check if the message continues from previous (vCard broken into two SyncML messages)
        startDataPosition = message.find(beginType);
        endDataPosition = message.find(endType);
        if(endDataPosition < startDataPosition)
        {
            message.replace(endDataPosition, endType.length(), "");
            LOG.debug("Confidential data continues from previous SyncML message", __FUNCTION__);
        }
        
        // try to find vcard and substitute
        while ((startDataPosition = message.find(beginType)) != std::string::npos)
        {
            std::string replacement = "*****";
            bool found = true;
            bool foundCred = false;
            
            endDataPosition = message.find(endType);

            if (endDataPosition == std::string::npos) {
                endDataPosition = message.find(MOREDATA);
                if (endDataPosition == std::string::npos) {
                    LOG.debug("%s something wrong in passed msg: leave as is.", __FUNCTION__);
                    break;
                }
                LOG.debug("Confidential data continues in next SyncML message", __FUNCTION__);
                replacement = "***** </data>";
            }
            else
            {
                endDataPosition = endDataPosition + endType.length();

                while (found == true)
                {
                    found = false;
                    if (message.at(endDataPosition+1) == '\r' || 
                        message.at(endDataPosition+1) == '\n') {
                        endDataPosition++;
                        found = true;
                    }                
                }
                //Parse credential tag
                if(beginType == BEGIN_CRED)
                {
                    std::string credMsg = message.substr(startDataPosition, endDataPosition - startDataPosition);
                    size_t startCredDataPosition = 0;
                    size_t endCredDataPosition = 0;
                    if((startCredDataPosition = credMsg.find(BEGIN_CRED_DATA)) != std::string::npos)
                    {
                        startCredDataPosition = startCredDataPosition + strlen(BEGIN_CRED_DATA);
                        endCredDataPosition = credMsg.find(END_CRED_DATA);
                        foundCred = true;
                        if (endCredDataPosition == std::string::npos)
                        {
                            LOG.debug("%s something wrong in passed msg: leave as is.", __FUNCTION__);
                            return "";
                        }
                        
                        credMsg = credMsg.replace(startCredDataPosition, endCredDataPosition - startCredDataPosition, replacement.c_str());
                        replacement = credMsg;
                    }
                }
            }//else moredata

            message = message.replace(startDataPosition, endDataPosition - startDataPosition, replacement.c_str());
            if(foundCred) break;
            
        }//end while
    }
    return message;
}
