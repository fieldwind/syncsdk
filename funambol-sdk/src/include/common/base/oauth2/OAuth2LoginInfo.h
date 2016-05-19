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

#ifndef __OAUTH2_LOGIN_INFO_H__
#define __OAUTH2_LOGIN_INFO_H__

#include <string>

namespace Funambol {
 
class OAuth2LoginInfo {
    public: 
        OAuth2LoginInfo() : authorizationCodeUrl(""),
                            accessTokenUrl(""),
                            refreshTokenUrl("") {}
    
    
        OAuth2LoginInfo(const std::string& authorizationCodeUrl_,
                        const std::string& accessTokenUrl_,
                        const std::string& refreshTokenUrl_) :
                    authorizationCodeUrl(authorizationCodeUrl_),
                    accessTokenUrl(accessTokenUrl_),
                    refreshTokenUrl(refreshTokenUrl_) {}
    
    
        void setAuthorizationCodeUrl(const std::string& authorizationCodeUrl_) { authorizationCodeUrl = authorizationCodeUrl_; }
        void setAccessTokenUrl(const std::string& accessTokenUrl_)             { accessTokenUrl = accessTokenUrl_; }
        void setRefreshTokenUrl(const std::string& refreshTokenUrl_)           { refreshTokenUrl = refreshTokenUrl_; }
    
    
        const char* getAuthorizationCodeUrl() const { return authorizationCodeUrl.c_str(); }
        const char* getAccessTokenUrl() const { return accessTokenUrl.c_str(); }
        const char* getRefreshTokenUrl() const { return refreshTokenUrl.c_str(); }
    
    private:
        std::string authorizationCodeUrl;
        std::string accessTokenUrl;
        std::string refreshTokenUrl;
};


}

#endif
