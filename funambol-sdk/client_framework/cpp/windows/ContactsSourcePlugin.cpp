/*
 * Funambol is a mobile platform developed by Funambol, Inc.
 * Copyright (C) 2012 Funambol, Inc.
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

#include "ContactsSourcePlugin.h"
#include "SourcePluginManager.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/Log.h"
#include "ClientConfig.h"
#include "WindowsSyncSource.h"

using namespace Funambol;

ContactsSourcePlugin::ContactsSourcePlugin(const char* tag,
                                           const char* label,
                                           const char* name) : SourcePlugin(tag, label, name)
{
}

ContactsSourcePlugin::~ContactsSourcePlugin()
{
}

bool ContactsSourcePlugin::init() {
    // Init the plugin. This method is invoked after the basic of the application has been
    // setup, so we can access most resources
    LOG.debug("Initializing contacts source");
  
    clientConfig = ClientConfig::getInstance();
    
    //if (clientConfig->isSourceEnabledByClient(sourceName.c_str())) {
        // Use a standard configuration
        config = new SourcePluginConfig(clientConfig, sourceName);
        
        sapiUri = "contact";
        sapiArrayKey = "contacts";
        
        // assign the right sync source
        pimSyncSource = createPIMSyncSource();
        mediaSyncSource = NULL;
            
        LOG.debug("%s: initialization done", __FUNCTION__);
    
        return true;
    //}
    
    //LOG.info("Contacts not visible");
    
    //return false;
}

SyncSource* ContactsSourcePlugin::createPIMSyncSource() {

    WCHAR* wname = toWideChar(sourceName.c_str());
    SyncSourceConfig* sc = clientConfig->getSyncSourceConfig(sourceName.c_str());
    WindowsSyncSource* source = new WindowsSyncSource(wname, sc);
    // WindowsSyncClient winClient(source);
    return source;
}

long ContactsSourcePlugin::getTotalNumberOfLocalItems() { 
    long ret = 0;
    
    //if (pimSyncSource) {
    //    ret = ((AddressBookSyncSource*)pimSyncSource)->getTotalNumberOfLocalItems();
    //}
    return ret;
}

bool ContactsSourcePlugin::requiresDeepSync() {
    if (pimSyncSource->hasLocalChanges()) {
        LOG.debug("%s: PIM source %s has local changes, perform a deep sync", __FUNCTION__, pimSyncSource->getName());
        return true;
    } else {
        return false;
    }
}

const char* ContactsSourcePlugin::getToolbarIdentifier() const {
    return "";
}

const char* ContactsSourcePlugin::getToolbarIconPath() const {
    return "";
}

const char* ContactsSourcePlugin::getToolbarWarningIconPath() const {
    return "";
}

const char* ContactsSourcePlugin::getToolbarTitle() const {
    return "";
}
