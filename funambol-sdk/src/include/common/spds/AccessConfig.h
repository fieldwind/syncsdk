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
#ifndef INCL_ACCESS_CONFIG
#define INCL_ACCESS_CONFIG
/** @cond DEV */

#include "base/fscapi.h"
#include "spds/constants.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include <string>

BEGIN_NAMESPACE


/*
 * -------------------------- AccessConfig class -----------------------------
 * This class groups all configuration properties to estabilish a
 * connection with a sync server.
 * AccessConfig is a part of SyncManagerConfig (along with DeviceConfig
 * and an array of SyncSourceConfig).
 */
class AccessConfig {
    public:
    
        typedef enum EncryptionMode {
            NOT_ENCRYPTED          = 0,
            ENCRYPTED              = 1
        } EncryptionMode;
    
    private:

        /// "sapi" or "oauth"
        std::string authenticationType;
    
        char*           username            ;
        char*           password            ;
        char*           phonenum            ;
        bool            useProxy            ;
        char*           proxyHost           ;
        int             proxyPort           ;
        char*           proxyUsername       ;
        char*           proxyPassword       ;
        char*           syncURL             ;
        unsigned long   beginTimestamp      ;
        unsigned long   endTimestamp        ;
        SyncMode        firstTimeSyncMode   ;
        char*           serverNonce         ;
        char*           clientNonce         ;
        char*           serverID            ;
        char*           serverPWD           ;
        char*           clientAuthType      ;
        char*           serverAuthType      ;
        bool            isServerAuthRequired;
        unsigned long   maxMsgSize          ;
        unsigned long   readBufferSize      ;
        char*           userAgent           ;
        bool            checkConn           ;
        unsigned int    responseTimeout     ;
        bool            compression         ;
    
    char*           phoneidentify           ;
    char*           tokenauth           ;


        /// Indicates if the credentials were stored encrypted or not.
        /// If 1 (ENCRYPTED), credentials are read and saved encrypted.
        /// If 0 (NOT_ENCRYPTED), credentials are expected and read as plain text, but saved encrypted (e.g. for upgrades or testing).
        AccessConfig::EncryptionMode encryptionMode;

        // oauth2 parameters
        std::string oauth2AccessToken;
        time_t oauth2AccessTokenSetTime;
        std::string oauth2RefreshToken;
        std::string oauth2ExpiresIn;
        std::string oauth2Scope;
        std::string oauth2ClientType;
   
        // wassup parameter
        std::string wassupAuthToken;
        
        /**
         * Dirty flag.
         * Used to execute the save operations only when really necessary.
         */
        unsigned int dirty;

        /**
         * Sets the given buffer with the given value, dealing correctly with
         * NULL values. If a NULL value is passed, the empty string is used.
         *
         * @param buf the destination buffer
         * @param v the new value (CAN BE NULL)
         */
        void set(char* * buf, const char*  v);

    public:
    

        AccessConfig();
        AccessConfig(AccessConfig& s);
        ~AccessConfig();
    
        const char*  getPhoneidentify() const;
        void setPhoneidentify(const char*  phoneidentify);

        const char*  getTokenauth() const;
        void setTokenauth(const char*  tokenauth);


        /**
         * Returns the username value.
         *
         * @return The username value. The caller MUST NOT release
         *         the memory itself.
         *
         */
        const char*  getUsername() const;

        /**
         *  Sets the username value. The given data are copied in an internal
         *  buffer so that the caller is assured that the given address can be
         *  released after the call.
         *
         *  @param username the new username value
         */
        void setUsername(const char*  username);

        /**
         * Returns the password value.
         */
        const char*  getPassword() const;

        /**
         * Sets a new password value. The given data are copied in an internal
         * buffer so that the caller is assured that the given address can be
         * released after the call.
         *
         * @param password the new password value
         */
        void setPassword(const char*  password);
    
        /**
         * Returns the phonenum value.
         *
         * @return The phonenum value. The caller MUST NOT release
         *         the memory itself.
         *
         */
        const char*  getPhoneNumber() const;
    
        /**
         *  Sets the phonenum value. The given data are copied in an internal
         *  buffer so that the caller is assured that the given address can be
         *  released after the call.
         *
         *  @param phonenum the new phonenum value
         */
        void setPhoneNumber(const char*  phone);

        /**
         * Returns the SyncMode that the sync engine should use the first time
         * a source is synced
         */
        SyncMode getFirstTimeSyncMode() const;

        /**
         * Sets the SyncMode that the sync engine should use the first time
         * a source is synced
         *
         * @param syncMode the new sync mode
         */
        void setFirstTimeSyncMode(SyncMode syncMode);

        /**
         * Should the sync engine use a HTTP proxy?
         */
        bool getUseProxy() const;

        /**
         * Sets if the sync engine should use a HTTP proxy to access the server.
         *
         * @param useProxy false for not use a proxy, true otherwise
         */
        void setUseProxy(bool useProxy);

        /**
         * Returns the proxyHost value.
         */
        const char*  getProxyHost() const;

        /**
         * Sets a new proxyHost value.
         *
         * @param proxyHost the new proxyHost value
         */
        void setProxyHost(const char*  proxyHost);

        int getProxyPort() const;
        void setProxyPort(int v);

        /**
         * Returns the proxyUsername value.
         */
        const char* getProxyUsername() const;

        /**
         * Sets a new proxyUsername value.
         *
         * @param proxyUsername the new proxyUsername value
         */
        void setProxyUsername(const char*  proxyUsername);

        /**
         * Returns the proxyPassword value.
         */
        const char* getProxyPassword() const;

        /**
         * Sets a new proxyPassword value.
         *
         * @param proxyPassword the new proxyPassword value
         */
        void setProxyPassword(const char*  proxyPassword);

        /**
         * Returns the syncURL value. If the URL does not start with http://
         * (or HTTP://) or https:// (or HTTPS://), http:// is prepended to the
         * given string.
         */
        const char*  getSyncURL() const;

        /**
         * Sets a new the syncURL value. The given data are copied in an internal
         * buffer so that the caller is assured that the given address can be
         * released after the call.
         *
         * @param syncURL the new syncURL value
         */
        void setSyncURL(const char*  syncURL);

        /**
         * Sets the new "beginSync" timestamp.
         *
         * @param timestamp the beginSync timestamp
         */
        void setBeginSync(unsigned long timestamp);

        /**
         * Returns the beginSync timestamp
         */
        unsigned long getBeginSync() const;

        /**
         * Sets the new "endSync" timestamp.
         *
         * @param timestamp the endSync timestamp
         */
        void setEndSync(unsigned long timestamp);

        /**
         * Returns the endSync timestamp
         */
        unsigned long getEndSync() const;

        bool getServerAuthRequired() const;

        void setServerAuthRequired(bool v);

        const char*  getClientAuthType() const;

        void setClientAuthType(const char*  v);

        const char*  getServerAuthType() const;

        void setServerAuthType(const char*  v);

        const char*  getServerPWD() const;

        void setServerPWD(const char*  v);

        const char*  getServerID() const;

        void setServerID(const char*  v);

        const char*  getServerNonce() const;

        void setServerNonce(const char*  v);

        const char*  getClientNonce() const;

        void setClientNonce(const char*  v);

        void setMaxMsgSize(unsigned long msgSize);

        unsigned long getMaxMsgSize() const;

        void setReadBufferSize(unsigned long bufferSize);

        unsigned long getReadBufferSize() const;

        const char*  getUserAgent() const;

        void setUserAgent(const char*  v);

        void setCompression(bool  v);

        bool  getCompression() const;

        //void setCompression(bool v);

        void setCheckConn(bool v);
        /** @todo remove this, it is obsolete */
        bool getCheckConn() const;

        void setResponseTimeout(unsigned int bufferSize)   ;
        unsigned int getResponseTimeout() const            ;
    
        AccessConfig::EncryptionMode getEncryptionMode() const;
        void setEncryptionMode(AccessConfig::EncryptionMode mode);

        /**
         * Has some of this values changed?
         */
        unsigned int getDirty() const;

         //
        // OAuth2 support
        //
        void setAuthenticationType(const char* authType) { authenticationType = authType; }
        const char* getAuthenticationType() const        { return authenticationType.c_str();     }

    
        void setOAuth2AccessToken(const char* accessToken)   { oauth2AccessToken = accessToken;    }
        void setOAuth2AccessTokenSetTime(time_t setTime)     { oauth2AccessTokenSetTime = setTime; }
        void setOAuth2RefreshToken(const char* refreshToken) { oauth2RefreshToken = refreshToken;  }
        void setOAuth2ExpiresIn(const char* expiresIn)       { oauth2ExpiresIn  = expiresIn;       }
        void setOAuth2Scope(const char* scope)               { oauth2Scope  = scope;               }
        void setOAuth2ClientType(const char* clientType)     { oauth2ClientType = clientType;      }
    
        const char* getOAuth2AccessToken(void)   const       { return oauth2AccessToken.c_str();  }
        time_t getOAuth2AccessTokenSetTime(void) const       { return oauth2AccessTokenSetTime;   }
        const char* getOAuth2RefreshToken(void)  const       { return oauth2RefreshToken.c_str(); }
        const char* getOAuth2ExpiresIn(void)     const       { return oauth2ExpiresIn.c_str();    }
        const char* getOAuth2Scope(void)         const       { return oauth2Scope.c_str();        }
        const char* getOAuth2ClientType(void)    const       { return oauth2ClientType.c_str();   }
  

        void setWassupAuthToken(const std::string& authToken) { wassupAuthToken = authToken;      }
        const char* getWassupAuthToken()         const        { return wassupAuthToken.c_str();   }
        
        /**
         * Sets the values of this object with with the values from the given
         * AccessConfig source object.
         *
         * @param config the new value.
         */
        void assign(const AccessConfig& s);

        /*
         * Assign operator
         */
        AccessConfig& operator = (const AccessConfig& ac) {
            assign(ac);
            return *this;
        }

};


END_NAMESPACE

/** @endcond */
#endif
