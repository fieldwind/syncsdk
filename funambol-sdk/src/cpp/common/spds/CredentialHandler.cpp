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


#include "spds/CredentialHandler.h"
#include "base/globalsdef.h"
#include "base/Log.h"
#include "base/oauth2/OAuth2JsonParser.h"

USE_NAMESPACE

/*
 * Default constructor
 */
CredentialHandler::CredentialHandler() {
    initialize();
}

/**
 * Initializes private members
 */
void CredentialHandler::initialize() {
    username       = NULL;
    password       = NULL;
    clientAuthType = NULL;
    clientNonce    = NULL;

    serverID       = NULL;
    serverPWD      = NULL;
    serverAuthType = NULL;
    serverNonce    = NULL;
}

/*
 * Destructor. Free the allocated memory (if any)
 */
CredentialHandler::~CredentialHandler() {
    safeDel(&username       );
    safeDel(&password       );
    safeDel(&clientAuthType );
    safeDel(&clientNonce    );

    safeDel(&serverID       );
    safeDel(&serverPWD      );
    safeDel(&serverAuthType );
    safeDel(&serverNonce    );

}

void CredentialHandler::setUsername(const char* t) {
    safeDel(&username);
    username = stringdup(t);
}

const char *CredentialHandler::getUsername() {
    return username;
}

void CredentialHandler::setPassword(const char* t) {
    safeDel(&password);
    password = stringdup(t);
}

const char *CredentialHandler::getPassword() {
    return password;
}

void CredentialHandler::setClientAuthType(const char* t){
    safeDel(&clientAuthType);
    clientAuthType = stringdup(t);
}

const char* CredentialHandler::getClientAuthType(){
    return clientAuthType;
}


void CredentialHandler::setClientNonce(const char* t){
    safeDel(&clientNonce);
    clientNonce = stringdup(t);
}

const char* CredentialHandler::getClientNonce() {
    return clientNonce;
}

void CredentialHandler::setServerID(const char* t) {
    safeDel(&serverID);
    serverID = stringdup(t);
}

void CredentialHandler::setServerPWD(const char* t) {
    safeDel(&serverPWD);
    serverPWD = stringdup(t);
}

void CredentialHandler::setServerAuthType(const char* t) {
    safeDel(&serverAuthType);
    serverAuthType = stringdup(t);
}

const char* CredentialHandler::getServerAuthType() {
    return serverAuthType;
}

void CredentialHandler::setServerNonce(const char* t) {
    safeDel(&serverNonce);
    serverNonce = stringdup(t);
}

const char* CredentialHandler::getServerNonce() {
    return serverNonce;
}

void CredentialHandler::setServerAuthRequired(bool t) {
    isServerAuthRequired = t;
}

bool CredentialHandler::getServerAuthRequired() {
    return isServerAuthRequired;
}

Cred* CredentialHandler::getClientCredential() {

    Authentication* auth = NULL;
    char* credential  = NULL;

    if (strcmp(clientAuthType, AUTH_TYPE_MD5) == 0) {
        credential = MD5CredentialData(username, password, clientNonce);
        auth = new Authentication(AUTH_TYPE_MD5, credential);
        // overwrite the username that for MD5 auth is the same as data
        auth->setUsername(username);
        auth->setPassword(password);
        if (credential) { delete [] credential; credential = NULL; }
    }
    else if (strcmp(clientAuthType, AUTH_TYPE_OAUTH) == 0) {
        OAuth2JsonParser jsonParser;
        StringBuffer creds = jsonParser.formatOAuth2CredentialData(oauth2Credentials.getAccessToken(), oauth2Credentials.getRefreshToken(),
                                                                   oauth2Credentials.getClientType(), oauth2Credentials.getExpiresIn(), oauth2AccessTokenSetTime);
        if (creds.empty()) {
            LOG.error("%s: could not set oauth2 auth credentials for SyncML request", __FUNCTION__);
            return NULL;
        }

        auth = new Authentication(AUTH_TYPE_OAUTH, creds.c_str());
        auth->setUsername(username);
    }
    else if (strcmp(clientAuthType, AUTH_TYPE_RADIUS_PROXY) == 0) {
        auth = new Authentication(AUTH_TYPE_RADIUS_PROXY, username, "Funambol");
    }
    else {
        auth = new Authentication(AUTH_TYPE_BASIC, username, password);
    }

    Cred* cred = new Cred(auth);

    deleteAuthentication(&auth);
    return cred;

}

/*
* it returns the server credential. The method is used both during the request of authentication
* and the creation of the status as response of server authentication.
* Therefore, if the server is arleady authenticated, no credential are sent back.
*/


Cred* CredentialHandler::getServerCredential() {

    Authentication* auth = NULL;
    Cred* cred           = NULL;
    char* credential  = NULL;
    if (strcmp(serverAuthType, AUTH_TYPE_BASIC) == 0) {
        auth = new Authentication(AUTH_TYPE_BASIC, serverID, serverPWD);
    } else {
        credential = MD5CredentialData(serverID, serverPWD, serverNonce);
        auth = new Authentication(AUTH_TYPE_MD5, credential);
    }

    cred = new Cred(auth);

    deleteAuthentication(&auth);
    return cred;

}

bool CredentialHandler::performServerAuth(Cred* cred) {

    bool ret = false;
    Cred* currentCred = getServerCredential();

    if (cred == NULL || currentCred == NULL) {
        goto finally;
    }

    if (strcmp(cred->getData(), currentCred->getData()) == 0) {
        ret = true;
    }
finally:

    return ret;
}

Chal* CredentialHandler::getServerChal(bool isServerAuthenticated) {

    Chal* chal = NULL;

    if (strcmp(serverAuthType, AUTH_TYPE_BASIC) == 0 && isServerAuthenticated == false) {
        chal = Chal::getBasicChal();

    } else if (strcmp(serverAuthType, AUTH_TYPE_MD5) == 0) { // MD5
        chal = Chal::getMD5Chal();
        char nonce[16];
        generateNonce(nonce);
        NextNonce* nextNonce = new NextNonce(nonce, 16);
        chal->setNextNonce(nextNonce);
        setServerNonce(nextNonce->getValueAsBase64());
    }

    return chal;
}


// private
void CredentialHandler::generateNonce(char nonce[16]) {
    srand((unsigned int)time(NULL));
    for (unsigned int i = 0; i < 16; i++) {
        nonce[i] = ((rand()%100) * (rand()%100))%100;

        if (nonce[i] < 32) {
            nonce[i] +=96;
        }
    }
}


void CredentialHandler::setOAuth2AccessTokenSetTime(const time_t timestamp)
{
    this->oauth2AccessTokenSetTime = timestamp;
}

void CredentialHandler::setOAuth2Credentials(const char* accessToken, const char* refreshToken,
                                             const char* clientType, bool valid, int expiresIn) 
{
    this->oauth2Credentials.setAccessToken (accessToken);
    this->oauth2Credentials.setRefreshToken(refreshToken);
    this->oauth2Credentials.setClientType  (clientType);
    this->oauth2Credentials.setValid       (valid);
    this->oauth2Credentials.setExpiresIn   (expiresIn);
}

void CredentialHandler::setOAuth2Credentials(const char* jsonCredentials)
{
    if (jsonCredentials == NULL) {
        return;
    }

    OAuth2JsonParser jsonParser;
    this->oauth2Credentials = jsonParser.parseOAuth2CredentialData(jsonCredentials);
}

const OAuth2Credentials& CredentialHandler::getOAuth2Credentials()
{
    return this->oauth2Credentials;
}
