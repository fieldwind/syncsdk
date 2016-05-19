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

#include "MHFilesSourcePlugin.h"
#include "SourcePluginManager.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/Log.h"
#include "CacheReader.h"
#include "mediaHub/MHFileSyncSource.h"

namespace Funambol {


MHFilesSourcePlugin::MHFilesSourcePlugin(const char* tag,
                                         const char* label,
                                         const char* name,
                                         const char* globalLabelsStoreName_,
                                         const char* localItemsFolderName_,
                                         const char* defaultSpoolFolderName_,
                                         DMTClientConfig* clientConfig_,
                                         const char* sapiUri_,
                                         const char* sapiArrayKey_,
                                         const char* itemsStoreName_,
                                         const char* itemsTemporarySpoolFolder_,
                                         OrderField order) : SourcePlugin(tag, label, name),
                                            localItemsFolderName(localItemsFolderName_),
                                            defaultSpoolFolderName(defaultSpoolFolderName_),
                                            itemsStoreName(itemsStoreName_),
                                            globalLabelsStoreName(globalLabelsStoreName_), 
                                            itemsTemporarySpoolFolder(itemsTemporarySpoolFolder_),
                                            clientConfig(clientConfig_)
{

    sapiUri = sapiUri_; 
    sapiArrayKey = sapiArrayKey_;
    orderField = order;
}

MHFilesSourcePlugin::~MHFilesSourcePlugin()
{
}

bool MHFilesSourcePlugin::init() 
{
    struct stat st;
    
    // Init the plugin. This method is invoked after the basic of the application has been
    // setup, so we can access most resources
    LOG.debug("%s: initializing %s plugin ...", __FUNCTION__, sourceName.c_str());
    
    // Use a standard configuration
    config = new SourcePluginConfig(clientConfig, sourceName.c_str());
        
    media = true;
    selectiveUploadEnabled = false; 
    maxOutgoingItemSize = 0;//MAX_FILE_ITEM_SIZE;
        
    StringBuffer cache_path;
    const char* configFolder = PlatformAdapter::getConfigFolder();

    if (configFolder) {
        createFolder(configFolder);
    }
        
    cache_path.reset();
    cache_path.sprintf("%s/%s.db", configFolder, itemsStoreName.c_str());
    // FIXME - these value should be read from config as getFunambolSavedVersionNum
    // and getFunambolCurrentVersionNum
    cache = new MHItemsStore(itemsStoreName.c_str(), cache_path.c_str(), 0, 0);
 
    cache_path.reset();
    cache_path.sprintf("%s/%s.db", configFolder, globalLabelsStoreName.c_str());
    labelsStore = new MHLabelsStore(globalLabelsStoreName.c_str(), cache_path.c_str(), 0, 0);
 
    // create the sync source
    mediaSyncSource = createMHSyncSource();
    pimSyncSource = NULL;
  
    localItemsSpoolPath.sprintf("%s/%s/", PlatformAdapter::getHomeFolder().c_str(), localItemsFolderName.c_str());
        
    memset(&st, 0, sizeof(struct stat));
        
    if (statFile(localItemsSpoolPath.c_str(), &st) != 0) {
        if (errno == ENOENT) {
            if (createFolder(localItemsSpoolPath.c_str())) {
                LOG.error("%s: error creating local spool folder \"%s\": %s\n",
                    __FUNCTION__, localItemsSpoolPath.c_str(), strerror(errno));
                    
                return false;
            }
         } else {
            LOG.error("%s: error stating local spool folder \"%s\": %s\n",
                __FUNCTION__, localItemsSpoolPath.c_str(), strerror(errno));
                    
            return false;
        }
    }

    LOG.debug("%s: initialization done", __FUNCTION__);
    
    return true;
}

bool MHFilesSourcePlugin::canDownloadItem(MHSyncItemInfo* itemInfo) 
{
    return true;
}


MHSyncSource* MHFilesSourcePlugin::createMHSyncSource() {
    
    MHSyncSource* syncSource = NULL;
    
    LOG.debug("%s: creating new MH syncSource", __FUNCTION__);
    
    SyncSourceConfig* sc = clientConfig->getSyncSourceConfig(sourceName);
    
    if (!sc) {
        LOG.error("%s: sync source for files not found (%d)", 
                  __FUNCTION__, clientConfig->getNumSources());
                  
        return NULL;
    }
    
    SyncSourceReport* sapiReport = new SyncSourceReport(sourceName.c_str());
    unsigned long incomingFilterDate = 0;
    unsigned long outgoingFilterDate = 0;
    
    StringBuffer tmpSpoolPath = PlatformAdapter::getConfigFolder();
    tmpSpoolPath.append("/");
    tmpSpoolPath.append(itemsTemporarySpoolFolder.c_str());
  
    StringBuffer itemsSpoolPath;
    itemsSpoolPath.sprintf("%s/%s", PlatformAdapter::getHomeFolder().c_str(), defaultSpoolFolderName.c_str());

    if (localItemsSpoolPath.empty()) {
        localItemsSpoolPath = itemsSpoolPath;
        LOG.debug("%s: client local items spool folder: %s", __FUNCTION__, localItemsSpoolPath.c_str());
    }
    
    syncSource = createSource(*sc, *sapiReport, incomingFilterDate,
                             outgoingFilterDate, tmpSpoolPath);

    return syncSource;
}

}
