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

#include "FilesSourcePlugin.h"
#include "SourcePluginManager.h"
#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/Log.h"
#include "CacheReader.h"
#include "WinMHFileSyncSource.h"
#include "ClientConfig.h"
#include "WinClientConstants.h"

USE_FUNAMBOL_NAMESPACE

class FilesInitializer {
public:
    FilesInitializer() {
        // create customiz
        // CustomizationParams* params = CustomizationParams::getInstance();

        FilesSourcePlugin* plugin = new FilesSourcePlugin("files", "files",
                                                          FILES_SOURCE_NAME); //
        SourcePluginManager::getInstance()->registerSource(plugin);
    }
};

static const char* FILE_PENDING_DELETES_CACHE_NAME = "file_pending_deletes";

// This is used to automatically register the source into the source plugin manager
static FilesInitializer* initializer = new FilesInitializer();


FilesSourcePlugin::FilesSourcePlugin(const char* tag,
                                     const char* label,
                                     const char* name) : SourcePlugin(tag, label, name),
                                     localItemsSpoolPath("")
{
}

FilesSourcePlugin::~FilesSourcePlugin()
{
}

bool FilesSourcePlugin::init() 
{
    // Init the plugin. This method is invoked after the basic of the application has been
    // setup, so we can access most resources
    LOG.debug("%s: initializing files source...", __FUNCTION__);
    
    if (FILES_SOURCE_VISIBLE) {
        ClientConfig* clientConfig = ClientConfig::getInstance();
        struct stat st;
        
        // Use a standard configuration
        config = new SourcePluginConfig(sourceName);
        
        sapiUri = "file";
        sapiArrayKey = "files";
        // Order by last modified date
        orderField = orderByModificationDate;
        
        media = true;
        selectiveUploadEnabled = false; 
        maxOutgoingItemSize = MAX_FILE_ITEM_SIZE;
        
        StringBuffer cache_path;
        const char* configFolder = PlatformAdapter::getConfigFolder();
        
        cache_path.reset();
        cache_path.sprintf("%s/%s.db", configFolder, FILE_MANAGER_CACHE_NAME);
        // FIXME - these value should be read from config as getFunambolSavedVersionNum
        // and getFunambolCurrentVersionNum
        cache = new MHItemsStore(FILE_MANAGER_CACHE_NAME, cache_path.c_str(), 0, 0);
        
        // Create the table for pending deletes
        cache_path.reset();
        cache_path.sprintf("%s/%s.db", configFolder, FILE_PENDING_DELETES_CACHE_NAME);
        pendingDeletesCache = new MHItemsStore(FILE_PENDING_DELETES_CACHE_NAME, cache_path.c_str(),
                                0,
                                0);
        // create the sync source
        mediaSyncSource = createMHSyncSource();
        pimSyncSource = NULL;
  
        localItemsSpoolPath.sprintf("%s/%s/", PlatformAdapter::getHomeFolder().c_str(), FILES_DEFAULT_PATH);
        
        memset(&st, 0, sizeof(struct stat));
        
        if (stat(localItemsSpoolPath.c_str(), &st) != 0) {
            if (errno == ENOENT) {
                if (createFolder(localItemsSpoolPath.c_str())) {
                    LOG.error("%s: error creating local spool folder \"%s\"",
                        __FUNCTION__, localItemsSpoolPath.c_str());
                    
                    return false;
                }
            } else {
                LOG.error("%s: error stating local spool folder \"%s\"",
                    __FUNCTION__, localItemsSpoolPath.c_str());
                    
                return false;
            }
        }

        LOG.debug("%s: initialization done", __FUNCTION__);
    
        return true;
    }
    
    return false;
}

bool FilesSourcePlugin::canDownloadItem(MHSyncItemInfo* itemInfo) 
{
    return true;
}


MHSyncSource* FilesSourcePlugin::createMHSyncSource() {
    
    WinMHFileSyncSource* filesSyncSource = NULL;
    
    LOG.debug("%s: creating new fileSyncSource", __FUNCTION__);
    
    ClientConfig* config = ClientConfig::getInstance();
    SyncSourceConfig* sc = config->getSyncSourceConfig(sourceName.c_str());
    
    sc = config->getSyncSourceConfig(FILES_SOURCE_NAME);
    if (!sc) {
        LOG.error("%s: sync source for files not found (%d)", 
                  __FUNCTION__, config->getNumSources());
        return NULL;
    }
    
    SyncSourceReport* sapiReport = new SyncSourceReport(sourceName.c_str());
    unsigned long incomingFilterDate = 0;
    unsigned long outgoingFilterDate = 0;
    
    // get the 2 items store from CacheReader and pass them to the ssource
    MHItemsStore* itemsStore         = CacheReader::getInstance()->getCache(sourceName.c_str());
    
    StringBuffer tmpSpoolPath = PlatformAdapter::getConfigFolder();
    tmpSpoolPath.append("/");
    tmpSpoolPath.append(FILES_TEMPORARY_SPOOL);
  
    StringBuffer itemsSpoolPath;
    itemsSpoolPath.sprintf("%s/%s", PlatformAdapter::getHomeFolder().c_str(), FILES_DEFAULT_PATH);

    if (localItemsSpoolPath.empty()) {
        localItemsSpoolPath = itemsSpoolPath;
        LOG.debug("%s: client local items spool folder: %s", __FUNCTION__, localItemsSpoolPath.c_str());
    }
    
    filesSyncSource = new WinMHFileSyncSource(sapiUri.c_str(),
                                                sapiArrayKey.c_str(),
                                                getOrderFieldAsString(),
                                                *sc, *sapiReport, itemsStore, 
                                                incomingFilterDate,
                                                outgoingFilterDate,
                                                tmpSpoolPath,
                                                itemsSpoolPath.c_str(),
                                                localItemsSpoolPath.c_str(), config);
    
    fillExcludedItemsMetadata = false;
    requiresThumbnailRefreshOnUploadTermination = false;
    
    return filesSyncSource;
}
