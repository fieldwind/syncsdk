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

#include "MediaHub/MediaRequestManagerFactory.h"
#include "MediaHub/MHMediaRequestManager.h"
#include "MediaHub/SapiMediaRequestManager.h"
#include "MediaHub/RadiusProxyMediaRequestManager.h"
#include "MediaHub/OAuth2MediaRequestManager.h"
#include "MediaHub/WassupMediaRequestManager.h"
#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"

namespace Funambol {

MediaRequestManagerFactory* MediaRequestManagerFactory::instance = NULL;

MediaRequestManagerFactory* MediaRequestManagerFactory::getInstance()
{
    if (instance == NULL) {
        instance = new MediaRequestManagerFactory();
    }

    return instance;
}

MHMediaRequestManager* MediaRequestManagerFactory::getMediaRequestManager(const char* url, const char* sapiSourceUri, const char* sapiArrayKey,
                              const char* orderFieldValue, MHItemJsonParser* itemParser, AbstractSyncConfig* config)
{
    MHMediaRequestManager* reqMan = NULL;
    std::string authenticationType = config->getAuthenticationType();

    if (authenticationType == AUTH_TYPE_OAUTH) {
        reqMan = new OAuth2MediaRequestManager(url, sapiSourceUri, sapiArrayKey, orderFieldValue, itemParser, config);

    } else if(authenticationType == AUTH_TYPE_RADIUS_PROXY) {
        reqMan = new RadiusProxyMediaRequestManager(url, sapiSourceUri, sapiArrayKey, orderFieldValue, itemParser, config);
    
    } else if (authenticationType == AUTH_TYPE_WASSUP) {
        reqMan = new WassupMediaRequestManager(url, sapiSourceUri, sapiArrayKey,orderFieldValue, itemParser, config);
    
    } else {
        reqMan = new SapiMediaRequestManager(url, sapiSourceUri, sapiArrayKey,orderFieldValue, itemParser, config);
    } 

    return reqMan;
}


MHMediaRequestManager* MediaRequestManagerFactory::getMediaRequestManager(const char* url, AbstractSyncConfig* config)
{
    MHMediaRequestManager* reqMan = NULL;
    std::string authenticationType = config->getAuthenticationType();
     
    if (authenticationType == AUTH_TYPE_OAUTH) {
        reqMan = new OAuth2MediaRequestManager(url, config);
    
    } else if(authenticationType == AUTH_TYPE_RADIUS_PROXY) {
        reqMan = new RadiusProxyMediaRequestManager(url, config);
    
    } else if (authenticationType == AUTH_TYPE_WASSUP) {
        reqMan = new WassupMediaRequestManager(url, config);
    
    } else {
        reqMan = new SapiMediaRequestManager(url, config);
    }

    return reqMan;
}

MediaRequestManagerFactory::MediaRequestManagerFactory() {}
MediaRequestManagerFactory::~MediaRequestManagerFactory() {}

}
