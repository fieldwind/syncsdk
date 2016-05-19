/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2011 Funambol, Inc.
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

#include <set>

#include "MediaHub/MHSyncManager.h"
#include "MediaHub/MediaRequestManagerFactory.h"
#include "MediaHub/MHItemsStore.h"
#include "MediaHub/MHContentTypes.h"
#include "event/FireEvent.h"
#include "spds/spdsutils.h"
#include "spdm/constants.h"
#include "base/util/WString.h"
#include "http/URL.h"

USE_NAMESPACE

/// Returns true if passed syncMode is download enabled
static bool isDownloadEnabled(const SyncMode syncMode) {
    
    if (syncMode == SYNC_TWO_WAY || 
        syncMode == SYNC_ONE_WAY_FROM_SERVER ||
        syncMode == SYNC_REFRESH_FROM_SERVER) {
        return true;
    }
    return false;
}

/// Returns true if passed syncMode is upload enabled
static bool isUploadEnabled(const SyncMode syncMode) {
    
    if (syncMode == SYNC_TWO_WAY || 
        syncMode == SYNC_ONE_WAY_FROM_CLIENT ||
        syncMode == SYNC_REFRESH_FROM_CLIENT) {
        return true;
    }
    return false;
}


MHSyncManager::MHSyncManager(MHSyncSource& s, AbstractSyncConfig& c) : 
                                 source(s), config(c), report(s.getReport()),
                                 syncMode(SYNC_TWO_WAY), isSyncingItemChanges(false),
                                 downloadTimestamp(0) {

    // just for easy use in all methods.
    SyncSourceConfig& ssconfig = source.getConfig();
    sourceName = ssconfig.getName();
    sourceURI  = ssconfig.getURI();
    requestSessionId = "";
    
    //report.clear();
    
    URL url(config.getSyncURL());
    StringBuffer host = url.getHostURL();
    offsetClientServer = 0;
    
    MediaRequestManagerFactory* reqManFactory = MediaRequestManagerFactory::getInstance();
    mhMediaRequestManager = reqManFactory->getMediaRequestManager(host,
                                                      source.getSapiUri(),
                                                      source.getSapiArrayKey(),
                                                      source.getOrderField(),
                                                      source.createItemJsonParser(),
                                                      &config);
    // set http params, read from config
    mhMediaRequestManager->setRequestTimeout   (config.getMHRequestTimeout());
    mhMediaRequestManager->setResponseTimeout  (config.getMHResponseTimeout());
    mhMediaRequestManager->setUploadChunkSize  (config.getMHUploadChunkSize());
    mhMediaRequestManager->setDownloadChunkSize(config.getMHDownloadChunkSize());
    
    freeQuota = 0;
    userQuota = 0;    
}


MHSyncManager::~MHSyncManager() {

    delete mhMediaRequestManager;
}



bool MHSyncManager::initialize() {
    
    LOG.debug("[%s]", __FUNCTION__);
    
    report.clear();
    
    isSyncingItemChanges = defineSyncBehavior();
    
    // not done: cache should be reset at upper level when account changes, to reallyClean everything
    //checkLastUserAndServer();
    
    LOG.info("It's %sthe first sync of %s", isSyncingItemChanges? "NOT ":"", sourceName.c_str());
    
    return isSyncingItemChanges;
}

int MHSyncManager::performTwinDetection(int syncRes) {
    
    if (syncRes != ESMRSuccess) {
        LOG.info("%s Got error so cannot continue: %i", __FUNCTION__, syncRes);
        return syncRes;
    }

    LOG.info("Starting TWIN DETECTION on all local items for source %s", sourceName.c_str());
    
    int ret = source.performTwinDetection(config);
    
    if (ret) {
        report.setLastErrorCode(ret);
        report.setLastErrorMsg("Error checking twins with all local items");
    }
        
    LOG.info("TWIN DETECTION on all items completed for source %s", sourceName.c_str());
    LOG.debug("%s: exit code: %d", __FUNCTION__, report.getLastErrorCode());
    return report.getLastErrorCode();
}


int MHSyncManager::getAllServerMetadata() {

    LOG.info("Get all remote items metadata, for source %s", sourceName.c_str());
    
    int getAllStatus = -1;
    unsigned long syncMetaDataTimestamp = time(NULL) - offsetClientServer;
    
    // Get all items info: paging requests
    int limit  = MH_PAGING_LIMIT;
    int offset = 0;
    
    LOG.debug("%s: paging requests = %d", __FUNCTION__, limit);
    
    CacheItemsList itemsList;
    CacheLabelsMap labelsMap;
    
    // Read all of the items in cache at the beginning of the refresh
    // so we can quickly check which ones are
    CacheItemsList itemsInCache;
    MHItemsStore* cache = source.getMHItemsStore();
    cache->getAllEntries(itemsInCache);
    std::vector<MHStoreEntry*>& entries = itemsInCache.getItems();
    std::set<std::string> availableGuids;
    std::vector<MHStoreEntry*>::iterator entriesIt = entries.begin();
    for(;entriesIt != entries.end();++entriesIt) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*entriesIt;
        availableGuids.insert(itemInfo->getGuid().c_str());
    }
    itemsInCache.clear();
    
    while (1) 
    {
        itemsList.clear();
        labelsMap.clear();
        int itemsReceived = 0;
        
        LOG.debug("get items info from Server in range: [%d - %d]", offset, offset + limit);
        
        if (config.isToAbort()) {
            setSyncError(ESSMCanceled);
            goto finally; 
        }
        
        // Labels are allocated by the getAllItems method and must be deallocated afterward
        EMHMediaRequestStatus err = mhMediaRequestManager->getAllItems(itemsList, labelsMap,
                                                                       &downloadTimestamp, limit, offset);
        if (err == ESMRNetworkError) {
            err = retryGetAllItems(itemsList, labelsMap, limit, offset);
        }
        
        if (err != ESMRSuccess) {
            setSyncError(ESSMMHError, err);
            goto finally;
        }
        
        // Set the time offset between server and client 
        time_t clientTime = time(NULL);
        offsetClientServer = clientTime - downloadTimestamp;
        source.setClientServerTimeDrift(offsetClientServer);
        
        report.setState(SOURCE_ACTIVE);

        std::vector<MHStoreEntry*>& entries = itemsList.getItems();
        itemsReceived = entries.size();
        LOG.info("%d %s metadata received from server", itemsReceived, sourceName.c_str());
                
        // Filter items (size, date, extension, blacklisted)
        // If filtered, the remote item is removed from the list
        if (filterIncomingItems(&itemsList)) { 
            goto finally; 
        }
        
        // 
        // Add remote items to cache and refresh UI (just metadata)
        //
        if (entries.size() > 0) {
            LOG.debug("%s: adding %d remote items to cache", __FUNCTION__, entries.size());
            
            std::vector<MHOperationDescriptor*> addOperations;
            std::vector<MHOperationDescriptor*> updOperations;
            
            for (unsigned int j=0; j<entries.size(); j++) {
                
                MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entries[j];
                if (!itemInfo) continue;
                
                // for server items, the ID in the report is the GUID
                WString wguid;
                wguid = itemInfo->getGuid();
                
                itemInfo->setStatus(EStatusRemote);
                source.setContentTypeByExtension(itemInfo);

                std::set<std::string>::iterator existingGuidIt = availableGuids.find(itemInfo->getGuid().c_str());
                
                if (existingGuidIt != availableGuids.end()) {
                    // check if item already in cache (by GUID), and in case update it
                    // this is required to avoid dupes (i.e. if sync interrupted)
                    MHSyncItemInfo* itemInCache = source.getItemFromCache(MHItemsStore::guid_field_name,
                                                                          itemInfo->getGuid().c_str());
                    
                    // TODO: implement a proper merging mechanism
                    LOG.debug("%s: Merging information to update the item (%d)",__FUNCTION__,itemInCache->getId());
                    itemInfo->setId  (itemInCache->getId());
                    itemInfo->setLuid(itemInCache->getLuid());
                    if (itemInCache->getLocalItemETag().length() > 0) {
                        itemInfo->setLocalItemETag(itemInCache->getLocalItemETag().c_str());
                    }
                    if (itemInCache->getLocalThumbETag().length() > 0) {
                        itemInfo->setLocalThumbETag(itemInCache->getLocalThumbETag().c_str());
                    }
                    if (itemInCache->getLocalPreviewETag().length() > 0) {
                        itemInfo->setLocalPreviewETag(itemInCache->getLocalPreviewETag().c_str());
                    }
                    //////////
                    
                    
                    LOG.debug("%s: updating item %s (ID=%d, status=%d) to cache", 
                              __FUNCTION__, itemInfo->getName().c_str(), itemInfo->getId(), itemInfo->getStatus());
                    
                    std::vector<MHLabelInfo*>* itemLabels = labelsMap.getLabels(itemInfo);
                    std::vector<MHLabelInfo*>* emptyLabels = NULL;
                    if (itemLabels == NULL) {
                        emptyLabels = new std::vector<MHLabelInfo*>();
                        itemLabels = emptyLabels;
                    }
                    // Create an update operation (does not take the ownership of items, we must release them)
                    MHOperationDescriptor* opDesc = MHOperationDescriptor::createUpdateOperation(itemInCache, itemInfo, itemLabels);
                    updOperations.push_back(opDesc);
                }
                else {
                    LOG.debug("%s: adding item %s to cache", __FUNCTION__, itemInfo->getName().c_str());
                    report.addItem(CLIENT, COMMAND_ADD, wguid.c_str(), 0, NULL);
                    std::vector<MHLabelInfo*>* itemLabels = labelsMap.getLabels(itemInfo);
                    MHOperationDescriptor* opDesc = MHOperationDescriptor::createAddOperation(itemInfo, itemLabels);
                    addOperations.push_back(opDesc);
                }
            }
            
            // Apply batch operations and release memory
            source.addItemsToCache(addOperations);
            std::vector<MHOperationDescriptor*>::iterator addIt = addOperations.begin();
            for(;addIt != addOperations.end();++addIt) {
                MHOperationDescriptor* opDesc = *addIt;
                if (opDesc->getSuccess()) {
                    fireItemStatusEvent(sourceName.c_str(), (MHSyncItemInfo*)opDesc->getNewEntry(), ITEM_ADDED_BY_SERVER);
                }
            }
            
            source.updateItemsInCache(updOperations);
            std::vector<MHOperationDescriptor*>::iterator updIt = updOperations.begin();
            for(;updIt != updOperations.end();++updIt) {
                MHOperationDescriptor* opDesc = *updIt;
                if (opDesc->getSuccess()) {
                    fireItemStatusEvent(sourceName.c_str(), (MHSyncItemInfo*)opDesc->getNewEntry(), ITEM_UPDATED_BY_SERVER);
                }
            }
            
            releaseSourceOperations(addOperations);
            releaseSourceOperations(updOperations);
            
            fireItemStatusEvent(sourceName.c_str(), NULL, CACHE_UPDATED);
        }
        
        if (itemsReceived < limit) {
            break;      // means we reached the end on the server, no need to continue
        }
        offset += entries.size();
    }
    getAllStatus = 0;   // success
    
finally:
    
    LOG.info("Get all remote items metadata completed, for source %s", sourceName.c_str());
    
    if (getAllStatus != 0) {
        // error
        getAllStatus = report.getLastErrorCode();
    }
    else {
        // ok
        LOG.info("Set last timestamp for %ss (S2C): %lu (%s)",
                  sourceName.c_str(), syncMetaDataTimestamp, unixTimeToString(syncMetaDataTimestamp, true).c_str()); 
        source.getConfig().setLongProperty(PROPERTY_DOWNLOAD_LAST_TIME_STAMP, (long)syncMetaDataTimestamp);
        
        // Update the server last check time
        LOG.debug("%s: Setting server last sync time to: %lu", __FUNCTION__, syncMetaDataTimestamp);
        source.getConfig().setLastSyncServerTime(syncMetaDataTimestamp);
    }
    
    LOG.debug("%s: sending METADATA_SYNC_END notification", __FUNCTION__);
    fireSyncSourceEvent(sourceURI.c_str(), sourceName.c_str(), syncMode, 0, METADATA_SYNC_END);
    
    return getAllStatus;
}

void MHSyncManager::releaseSourceOperations(std::vector<MHOperationDescriptor*>& operations) {
    std::vector<MHOperationDescriptor*>::iterator it  = operations.begin();
    std::vector<MHOperationDescriptor*>::iterator end = operations.end();
    // New items are automatically deleted once the CacheItemsList gets deallocated
    for(;it != end;++it) {
        MHOperationDescriptor* opDesc = *it;
        MHStoreEntry* oldItem = opDesc->getOldEntry();
        if (oldItem != NULL) {
            delete oldItem;
        }
        delete opDesc;
    }
}


int MHSyncManager::getServerMetadataChanges() {
    
    LOG.info("Get remote metadata changes since last sync, for source %s", sourceName.c_str());
    
    // NOTE: must set the last server timestamp to the start time of the whole task to avoid losing items!
    time_t clientTime = time(NULL);
    
    unsigned long serverLast = readServerLastTimestamp();
    StringBuffer fromDate = unixTimeToString(serverLast, true);
    bool cacheUpdated = false;

    int getChangesStatus = -1;
    LOG.debug("%s: paging = %d, from date %s", __FUNCTION__, MH_PAGING_LIMIT_IDS, fromDate.c_str());
    
    // Get the lists of item GUIDs (only the keys)
    ArrayList newIDs, modIDs, delIDs;
    EMHMediaRequestStatus err = mhMediaRequestManager->getItemsChanges(newIDs, modIDs, delIDs, fromDate, &downloadTimestamp);
    
    // Set the time offset between server and client 
    offsetClientServer = time(NULL) - downloadTimestamp;
    source.setClientServerTimeDrift(offsetClientServer);
    report.setState(SOURCE_ACTIVE);
    
    unsigned long syncMetaDataTimestamp = clientTime - offsetClientServer;
    
    if (err == ESMRNetworkError) {
        err = retryGetItemsChanges(newIDs, modIDs, delIDs, fromDate);
    }
    
    if (err != ESMRSuccess) {
        setSyncError(ESSMMHError, err);
        goto finally;
    }
    
    
    // Check the list of MOD items 
    if (modIDs.size() > 0) {
        LOG.debug("%s: checking %d updated items from the Server", __FUNCTION__, modIDs.size());
        for (int i=0; i<modIDs.size(); i++) {
            StringBuffer* guid = (StringBuffer*)modIDs.get(i);
            if (!guid) continue;
            
            MHSyncItemInfo* itemInCache = source.getItemFromCache(MHItemsStore::guid_field_name, guid->c_str());
            if (!itemInCache) {
                // MOD item not found in cache: it is a NEW
                LOG.debug("%s: MOD server item with GUID=%s NOT found in cache -> moving to NEW", 
                          __FUNCTION__, guid->c_str());
                newIDs.add(*guid);
                modIDs.removeElementAt(i);
                i--;
                continue;
            } else {
                EItemInfoStatus itemStatus = itemInCache->getStatus();
                
                if (itemStatus == EStatusLocallyRemoved) {
                    LOG.debug("%s: discarding updates on locally removed item %d",
                        __FUNCTION__, itemInCache->getId());
                        
                    modIDs.removeElementAt(i);
                    i--;
                }
            }
            
            delete itemInCache;
        }
    }

    
    LOG.info("Remote %ss changes: [NEW,MOD,DEL] = [%d,%d,%d]", sourceName.c_str(), newIDs.size(), modIDs.size(), delIDs.size());
    
    //
    // NEW items from server: get metadata from IDs (paging requests) and add to cache
    //
    if (newIDs.size() > 0) {
        LOG.info("Reading %d metadata of NEW %ss from the Server", newIDs.size(), sourceName.c_str());
        CacheItemsList newServerItems;
        CacheLabelsMap labelsMap;
        if (getServerItemsFromIds(newServerItems, labelsMap, newIDs)) {
            goto finally;
        }
        
        std::vector<MHStoreEntry*> entries = newServerItems.getItems();
        LOG.info("%d new %ss metadata received from server", entries.size(), sourceName.c_str());
    }
    
    //
    // MOD items from server: get metadata from IDs (paging requests) and add to cache
    //
    if (modIDs.size() > 0) {
        LOG.info("Reading %d metadata of UPDATED %ss from the Server", modIDs.size(), sourceName.c_str());
        CacheItemsList modServerItems;
        CacheLabelsMap labelsMap;
        if (getServerItemsFromIds(modServerItems, labelsMap, modIDs)) {
            goto finally;
        }
                
        std::vector<MHStoreEntry*> entries = modServerItems.getItems();
        LOG.info("%d updated %ss metadata received from server", entries.size(), sourceName.c_str());
    }

    //
    // DEL items from server: delete items locally
    //
    for (int i=0; i < delIDs.size(); i++) 
    {
        LOG.info("Checking %d DELETED %ss from the Server", delIDs.size(), sourceName.c_str());
        StringBuffer* guid = (StringBuffer*)delIDs.get(i);
        if (!guid || guid->empty()) {
            LOG.error("%s: missing or empty guid (%s %d)", __FUNCTION__, sourceName.c_str(), i);
            continue;
        }
        
        // find the corresponding item
        MHSyncItemInfo* itemInfo = source.getItemFromCache(MHItemsStore::guid_field_name, guid->c_str());
        if (!itemInfo) {
            LOG.debug("%s: cannot delete %s item with GUID=%s: item not found in cache", 
                      __FUNCTION__, sourceName.c_str(), guid->c_str());
            continue;
        }
        
        
        // DELETE item locally
        int err = source.deleteItem(itemInfo);
        if (err == 0) {
            // should we care errors on deletes?
            cacheUpdated = true;    // TODO: fire item deleted single event
            fireItemStatusEvent(sourceName.c_str(), itemInfo, ITEM_DELETED_BY_SERVER);
        }
        
        // update report
        WString wguid;
        wguid = *guid;
        report.addItem(CLIENT, COMMAND_DELETE, wguid.c_str(), err, NULL);
        
        delete itemInfo;
    }
    
    getChangesStatus = 0;   // success
    
finally:
    
    if (getChangesStatus != 0) {
        // error
        getChangesStatus = report.getLastErrorCode();
    }
    else {
        // ok
        LOG.info("Set last timestamp for %ss (S2C): %lu (%s)",
                 sourceName.c_str(), syncMetaDataTimestamp, unixTimeToString(syncMetaDataTimestamp, true).c_str()); 

        source.getConfig().setLongProperty(PROPERTY_DOWNLOAD_LAST_TIME_STAMP, (long)syncMetaDataTimestamp);
        // Update the server last check time
        LOG.debug("%s: Setting server last sync time to: %lu", __FUNCTION__, syncMetaDataTimestamp);
        source.getConfig().setLastSyncServerTime(syncMetaDataTimestamp);
    }
    
    return getChangesStatus;
}



int MHSyncManager::getServerItemsFromIds(CacheItemsList& items, CacheLabelsMap& labels, const ArrayList& itemsIDs) {
    
    int offset = 0;
    int limit = MH_PAGING_LIMIT_IDS;
    int ret = 1;
    
    while (1)
    {
        // copy N IDs into the pagedIds array (from offset to limit)
        ArrayList pagedIds;
        for (int i = offset; i < offset+limit; i++) {
            ArrayElement* e = itemsIDs.get(i);
            if (!e) { break; }
            pagedIds.add(*e);
        }
        
        // clear the output array
        items.clear();
        labels.clear();
        
        if (pagedIds.size() == 0) {
            break;  // means we reached the end on the server, no need to continue
        }
        if (config.isToAbort()) {
            setSyncError(ESSMCanceled);
            goto finally; 
        }
        
        LOG.debug("get %d items from Server (paging offset=%d, limit=%d)", pagedIds.size(), offset, limit);
        EMHMediaRequestStatus err = mhMediaRequestManager->getItemsFromId(items, labels, pagedIds);
        if (err == ESMRNetworkError) {
            err = retryGetItemsFromId(items, labels, pagedIds);
        }
        if (err != ESMRSuccess) {
            setSyncError(ESSMMHError, err);
            goto finally;
        }
        
        
        // *** no TWIN detection on NEW ***
        
        
        // Filter items (size, date, extension, blacklisted)
        // If filtered, the remote item is removed from the list
        if (filterIncomingItems(&items)) { 
            goto finally; 
        }
        
        std::vector<MHStoreEntry*> entries = items.getItems();
        
        // 
        // Add/update remote items to cache and refresh UI (just metadata)
        //
        if (entries.size() > 0) {
            WString wguid;
            
            LOG.debug("%s: %d remote %ss to be analyzed", __FUNCTION__, entries.size(), sourceName.c_str());
            
            int numPruned = 0;
            for (unsigned int j=0; j<entries.size(); j++) {
                MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entries[j];
                if (!itemInfo) continue;
                
                itemInfo->setStatus(EStatusRemote);
                
                // check if item already in cache (by GUID), and in case update it
                // this is required to avoid dupes (i.e. if sync interrupted)
                MHSyncItemInfo* itemInCache = source.getItemFromCache(MHItemsStore::guid_field_name, itemInfo->getGuid().c_str());
                if (itemInCache) {
                    //
                    // prune (fake) remote update
                    //
                    if (itemInfo->getServerLastUpdate() == itemInCache->getServerLastUpdate()) {
                        LOG.debug("%s: PRUNING update of remote %s %s (ID=%d): server last update is not changed",
                                  __FUNCTION__, sourceName.c_str(), itemInfo->getName().c_str(), itemInfo->getId());
                        delete itemInCache;
                        numPruned ++;
                        continue;
                    }
                    //
                    // In some scenarios the item could be uploaded properly but the response is not received yet from the server.
                    // So the item is in uploading status and receives an update from the server. In this case don't take any action
                    // but not consider as update. Then in the flow it will be consider as a resume from the client but, since the size is
                    // the same on the server, the item will complete immediately and the notification are correct
                    //
                    if (itemInCache->getStatus() == EStatusUploading && (itemInfo->getSize() == itemInCache->getSize())) {
                        LOG.debug("%s: PRUNING based on size update of remote %s %s (ID=%d): local status updating, size it the same (%lld)",
                                  __FUNCTION__, sourceName.c_str(), itemInfo->getName().c_str(), itemInfo->getId(), itemInfo->getSize());
                        delete itemInCache;
                        continue;
                    }
                    
                    itemInfo->setId(itemInCache->getId());
                   
                    LOG.debug("%s: server from server modification date: %ld - local item modification date: %ld", __FUNCTION__,
                        itemInfo->getModificationDateSecs(), itemInCache->getModificationDateSecs() - offsetClientServer);
                    
                    // TODO FIXME MARCO (Merge local info)
                    //
                    // [toccy] here we have to keep all local fields of itemInCache (like localPaths)
                    // best would be to write ONLY remote fields into the existing item in cache (not overwriting all fields)
                    itemInfo->setLocalItemPath   (itemInCache->getLocalItemPath());
                    itemInfo->setLocalThumbPath  (itemInCache->getLocalThumbPath());
                    itemInfo->setLocalPreviewPath(itemInCache->getLocalPreviewPath());
                    itemInfo->setLuid            (itemInCache->getLuid());

                    // fixes bug 12993 (updates on metadata caused a download of the binary)
                    itemInfo->setLocalItemETag   (itemInCache->getLocalItemETag().c_str());
                    itemInfo->setLocalPreviewETag(itemInCache->getLocalPreviewETag().c_str());
                    itemInfo->setLocalThumbETag  (itemInCache->getLocalThumbETag().c_str());

                    // fixes bug 14070 (rename check on updates C2S)
                    itemInfo->setCreationDate    (itemInCache->getCreationDate());
                    
                    
                    //////// Should we check if we have changed both client/server side and possibly merge the changes
                    //////// and deciding who wins?
                    if ((itemInfo->getServerLastUpdate() > itemInCache->getServerLastUpdate()) ||
                        (itemInfo->getModificationDateSecs() > (itemInCache->getModificationDateSecs() - offsetClientServer))) { 
                        //
                        // UPDATE local item
                        //
                        LOG.info("%s: setting item info size: %lld", __FUNCTION__, itemInfo->getSize());
                        
                        itemInfo->setModificationDateSecs(itemInfo->getModificationDateSecs() + offsetClientServer);
                        wguid = itemInCache->getGuid();
                        report.addItem(CLIENT, COMMAND_REPLACE, wguid.c_str(), err, NULL);
                        LOG.info("%s: remote update from server", __FUNCTION__);
                        std::vector<MHLabelInfo*>* itemLabels = labels.getLabels(itemInfo);
                        std::vector<MHLabelInfo*>* emptyLabels = NULL;
                        if (itemLabels == NULL) {
                            emptyLabels = new std::vector<MHLabelInfo*>();
                            itemLabels = emptyLabels;
                        }                        
                        source.updateItemInCache(itemInCache, itemInfo, itemLabels);
                        delete emptyLabels;
                    } else {
                        //
                        // UPDATE remote item  (copy local fields)
                        //
                        itemInfo->setId(itemInCache->getId());
                        itemInfo->setLuid(itemInCache->getLuid());
                        itemInfo->setLocalThumbPath(itemInCache->getLocalThumbPath());
                        itemInfo->setLocalPreviewPath(itemInCache->getLocalPreviewPath());
                        itemInfo->setContentType(itemInCache->getContentType());
                        LOG.debug("%s: updating item %s (ID=%d) to cache", 
                                  __FUNCTION__, itemInfo->getName().c_str(), itemInfo->getId());
                        std::vector<MHLabelInfo*>* itemLabels = labels.getLabels(itemInfo);
                        std::vector<MHLabelInfo*>* emptyLabels = NULL;
                        if (itemLabels == NULL) {
                            emptyLabels = new std::vector<MHLabelInfo*>();
                            itemLabels = emptyLabels;
                        }                        
                        source.updateItemInCache(itemInfo, itemLabels);
                        delete emptyLabels;
                                    
                        // update report after pruning 
                        wguid = itemInCache->getGuid();
                        report.addItem(SERVER, COMMAND_REPLACE, wguid.c_str(), err, NULL);
                    }
                    delete itemInCache;
                } else {
                    //
                    // ADD new remote item
                    //
                    source.setContentTypeByExtension(itemInfo);
                                    
                    // update report
                    wguid = itemInfo->getGuid();
                    report.addItem(CLIENT, COMMAND_ADD, wguid.c_str(), err, NULL);

                    LOG.debug("%s: adding item %s to cache", __FUNCTION__, itemInfo->getName().c_str());
                    std::vector<MHLabelInfo*>* itemLabels = labels.getLabels(itemInfo);
                    source.addItemToCache(itemInfo, itemLabels);
                }
            }
            
            if (numPruned > 0) {
                LOG.info("%d %s updates discarded (pruning): server last update is not changed", numPruned, sourceName.c_str());
            }

            fireItemStatusEvent(sourceName.c_str(), NULL, CACHE_UPDATED);
        }
        
        offset += pagedIds.size();
    }
    
    ret = 0;
    
finally:
    return ret;
}


// -------------------------------------

int MHSyncManager::getServerItemFromId(CacheItemsList& items, CacheLabelsMap& labels, StringBuffer guid) {
    
    ArrayList pageIds;
    EMHMediaRequestStatus err;
    
    std::vector<MHStoreEntry*> entries;
    
    if (config.isToAbort()) {
        setSyncError(ESSMCanceled);
        goto finally; 
    }
    
    pageIds.add(guid);
    
    err = mhMediaRequestManager->getItemsFromId(items, labels, pageIds);

    if (err != ESMRSuccess) {
        setSyncError(ESSMMHError, err);
        goto finally;
    }
    
    entries = items.getItems();
    
    if (entries.size() == 0) {
        LOG.info("No metadata returned by server: %s with GUID=%s deleted remotely?", sourceName.c_str(), guid.c_str());
        setSyncError(ESSMMHError, err);
        goto finally;
    }

    if (filterIncomingItems(&items)) {
        LOG.info("%s: item with guid %s filtered", __FUNCTION__, guid.c_str());
        goto finally;
    }
    
    return err;
    
finally:
    return report.getLastErrorCode();
}

int MHSyncManager::filterIncomingItems(CacheItemsList* serverItems) {

    if (!serverItems) { 
        setSyncError(ESSMGenericSyncError);
        return report.getLastErrorCode();
    }
    
    int res = 0;
    
    std::vector<MHStoreEntry*> entries = serverItems->getItems();
    std::vector<MHStoreEntry*>::iterator it = entries.begin();
    std::vector<MHStoreEntry*>::iterator endIt = entries.end();
    
    while(it != endIt) {
        if (config.isToAbort()) { 
            setSyncError(ESSMCanceled);
            res = ESSMCanceled;
            goto finally; 
        }
        
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*it;
        if (!itemInfo) continue;

        if (source.filterIncomingItem(*itemInfo, offsetClientServer)) {
            LOG.debug("skip server item %s (doesn't verify current incoming filters)", itemInfo->getName().c_str());
            it = entries.erase(it);
            endIt = entries.end();
        } else {
            ++it;
        }
    }

finally:
    return res;
}



//
// ------------------------ retry mechanism ---------------------------
//

ESMRStatus MHSyncManager::retryGetItemsChanges(ArrayList& newIDs, ArrayList& modIDs,
                                               ArrayList& delIDs, 
                                               const StringBuffer& fromDate) {
    ESMRStatus err = ESMRNetworkError;
    int maxRetries = config.getMHMaxRetriesOnError();
    if (maxRetries == 0) {
        return err;
    }

    int attempt = 0;
    while (attempt < maxRetries) 
    {
        if (config.isToAbort()) {
            setSyncError(ESSMCanceled);
            return err;
        }

        LOG.info("Retry getItemsChanges (%d of %d)...", attempt+1, maxRetries);
        //fireSyncSourceEvent(sourceURI.c_str(), sourceName.c_str(), syncMode, attempt+1, SYNC_SOURCE_RETRY);

        long sleepMsec = config.getMHSleepTimeOnRetry();
        if (sleepMsec) {
            LOG.debug("sleep %li msec", sleepMsec);
            sleepMilliSeconds(sleepMsec);
        }

        newIDs.clear();
        modIDs.clear();
        delIDs.clear();
        
        err = mhMediaRequestManager->getItemsChanges(newIDs, modIDs, delIDs, fromDate, &downloadTimestamp);
        
        if (err == ESMRNetworkError) {
            attempt ++;     // Network error
            continue;
        } 
        else {
            break;          // all other errors
        }
    }
    return err;
}

ESMRStatus MHSyncManager::retryGetItemsFromId(CacheItemsList& items, CacheLabelsMap& labels, const ArrayList& itemsIDs) {

    ESMRStatus err = ESMRNetworkError;
    int maxRetries = config.getMHMaxRetriesOnError();

    if (maxRetries == 0) {
        return err;
    }

    int attempt = 0;
    while (attempt < maxRetries) 
    {
        if (config.isToAbort()) {
            setSyncError(ESSMCanceled);
            return err;
        }

        LOG.info("Retry getItemsFromIDs (%d of %d)...", attempt+1, maxRetries);
        //fireSyncSourceEvent(sourceURI.c_str(), sourceName.c_str(), syncMode, attempt+1, SYNC_SOURCE_RETRY);

        long sleepMsec = config.getMHSleepTimeOnRetry();
        if (sleepMsec) {
            LOG.debug("sleep %li msec", sleepMsec);
            sleepMilliSeconds(sleepMsec);
        }

        items.clear();
        labels.clear();
        err = mhMediaRequestManager->getItemsFromId(items, labels, itemsIDs);
        
        if (err == ESMRNetworkError) {
            attempt ++;     // Network error
            continue;
        } 
        else {
            break;          // all other errors
        }
    }
    return err;
}

ESMRStatus MHSyncManager::retryGetAllItems(CacheItemsList& items, CacheLabelsMap& labelsMap, int limit, int offset) {
    
    ESMRStatus err = ESMRNetworkError;
    int maxRetries = config.getMHMaxRetriesOnError();
    if (maxRetries == 0) {
        return err;
    }

    int attempt = 0;
    while (attempt < maxRetries) 
    {
        if (config.isToAbort()) {
            setSyncError(ESSMCanceled);
            return err;
        }

        LOG.info("Retry getAllItems (%d of %d)...", attempt+1, maxRetries);
        //fireSyncSourceEvent(sourceURI.c_str(), sourceName.c_str(), syncMode, attempt+1, SYNC_SOURCE_RETRY);

        long sleepMsec = config.getMHSleepTimeOnRetry();
        if (sleepMsec) {
            LOG.debug("sleep %li msec", sleepMsec);
            sleepMilliSeconds(sleepMsec);
        }

        items.clear();
        labelsMap.clear();
        err = mhMediaRequestManager->getAllItems(items, labelsMap, &downloadTimestamp, limit, offset); 
        if (err == ESMRAccessDenied) {
            break;
        }
        
        if (err == ESMRNetworkError) {
            attempt ++;     // Network error
            continue;
        } 
        else {
            
            break;          // all other errors
        }
    }
    return err;
}


// ------------ private utility methods -------------
unsigned long MHSyncManager::readServerLastTimestamp() {

    bool err = false;
    unsigned long serverLast = source.getConfig().getLongProperty(PROPERTY_DOWNLOAD_LAST_TIME_STAMP, &err);
    LOG.debug("%s: reading PROPERTY_DOWNLOAD_LAST_TIME_STAMP: %lu", __FUNCTION__, serverLast); 

    if (err) { serverLast = 0; }
    return serverLast;
}


bool MHSyncManager::defineSyncBehavior() {

    // Read all source filters and last sync times.
    unsigned long clientLast = source.getConfig().getLast();
    unsigned long serverLast = readServerLastTimestamp();

    if (clientLast == 0 && serverLast == 0) {
        // Very 1st sync: sync ALL (must check the twins)
        return false;
    }

    if (isDownloadEnabled(syncMode)) {
        if (serverLast == 0) {
            // First download ever: sync ALL
            return false;
        }
    }

    if (isUploadEnabled(syncMode)) {
        if (clientLast == 0) {
            // First upload ever: sync ALL
            return false;
        }
    }

    // A filter by date is set on an active sync direction
    // (and it's not the first sync)
    return true;
}


bool MHSyncManager::checkLastUserAndServer() {

    StringBuffer lastSyncUrl  = source.getConfig().getProperty(PROPERTY_LAST_SYNC_URL);
    StringBuffer lastSyncUser = source.getConfig().getProperty(PROPERTY_LAST_SYNC_USERNAME);

    if (lastSyncUrl  != config.getSyncURL() ||
        lastSyncUser != config.getUsername()) 
    {
        LOG.info("Last server url or username has changed: cleanup local cache & timestamps");
        isSyncingItemChanges = false;
        source.resetCache();
        
        source.getConfig().setLast(0);
        source.getConfig().setLongProperty(PROPERTY_DOWNLOAD_LAST_TIME_STAMP, 0);
        
        // Set immediately the new values
        source.getConfig().setProperty(PROPERTY_LAST_SYNC_URL, config.getSyncURL());
        source.getConfig().setProperty(PROPERTY_LAST_SYNC_USERNAME, config.getUsername());
        return true;
    }
    return false;
}


void MHSyncManager::setSyncError(const ESSMError errorCode, const int data) {

    ESSMError code = errorCode;
    StringBuffer msg("");

    if (config.isToAbort()) {
        // Cancel sync error wins over other errors
        code = ESSMCanceled;
    }

    switch (code)
    {
        case ESSMSuccess:
        {
            // reset error
            report.setState(SOURCE_ACTIVE);
            report.setLastErrorType("");
            break;
        }
        case ESSMCanceled:
        {
            msg = "Operation canceled by the user";
            break;
        }
        case ESSMConfigError:
        {
            msg.sprintf("Configuration error for source %s", sourceName.c_str());
            break;
        }

        // MH errors
        case ESSMMHError:
        {
            //
            // filter MH error codes (data) -> set high level errors (http/invalid MH/...)
            //
            ESMRStatus MHStatus = (ESMRStatus)data;
            switch (MHStatus)
            {
                case ESMRAccessDenied:
                {
                    code = ESSMAuthenticationError;
                    msg.sprintf("MH authentication error: code %s%d", ERROR_TYPE_MH_REQUEST_MANAGER, data);
                    break;
                }
                case ESMRProxyAuthenticationRequired:
                {
                    code = ESSMProxyAuthenticationError;
                    msg.sprintf("MH authentication error: code %s%d", ERROR_TYPE_MH_REQUEST_MANAGER, data);
                    break;
                }

                case ESMRHTTPFunctionalityNotSupported:
                case ESMRMHNotSupported:
                {
                    code = ESSMMHNotSupported;
                    msg.sprintf("MH request not implemented by server: code %s%d", ERROR_TYPE_MH_REQUEST_MANAGER, data);
                    
                    break;
                }
                
                case ESMRConnectionSetupError:
                case ESMRGenericHttpError:
                case ESMRNetworkError:
                case ESMRRequestTimeout:
                {
                    code = ESSMNetworkError;
                    msg.sprintf("Network error: code %s%d", ERROR_TYPE_MH_REQUEST_MANAGER, data);
                    break;
                }
                case ESMRQuotaExceeded:
                {
                    code = ESSMServerQuotaExceeded;
                    msg.sprintf("Server quota exceeded for source %s", sourceName.c_str()); // add info about quota??
                    break;
                }
                
                case ESMROperationCanceled:
                {
                    code = ESSMCanceled;
                    msg = "Operation canceled by the user";
                    break;
                }
                
                case ESMRPaymentRequired:
                {
                    code = ESSMPaymentRequired;
                    msg.sprintf("Payment required for source %s", sourceName.c_str()); 
                    break;
                }
                    
                case ESMRUnknownMediaException:
                {
                    code = ESSMUnknownMediaException;
                    msg.sprintf("Unknown media exception");
                    break;
                }

                case ESMRErrorCodeLoginFailure750:
                {
                    code = ESSMErrorCodeLoginFailure750;
                    msg.sprintf("Login failure code 750");
                    break;
                }
                case ESMRErrorCodeLoginFailure751:
                {
                    code = ESSMErrorCodeLoginFailure751;
                    msg.sprintf("Login failure code 751");
                    break;
                }
                case ESMRErrorCodeLoginFailure752:
                {
                    code = ESSMErrorCodeLoginFailure752;
                    msg.sprintf("Login failure code 752");
                    break;
                }
                case ESMRErrorCodeLoginFailure753:
                {
                    code = ESSMErrorCodeLoginFailure753;
                    msg.sprintf("Login failure code 753");
                    break;
                }
                case ESMRErrorCodeLoginFailure754:
                {
                    code = ESSMErrorCodeLoginFailure754;
                    msg.sprintf("Login failure code 754");
                    break;
                }
                case ESMRErrorCodeLoginFailure755:
                {
                    code = ESSMErrorCodeLoginFailure755;
                    msg.sprintf("Login failure code 755");
                    break;
                }
                case ESMRErrorCodeLoginFailure756:
                {
                    code = ESSMErrorCodeLoginFailure756;
                    msg.sprintf("Login failure code 756");
                    break;
                }
                case ESMRErrorCodeLoginFailure757:
                {
                    code = ESSMErrorCodeLoginFailure757;
                    msg.sprintf("Login failure code 757");
                    break;
                }

                default:
                {
                    msg.sprintf("MH request error: code %s%d", ERROR_TYPE_MH_REQUEST_MANAGER, data);
                    break;
                }
            }
            break;
        }

        // SyncSource related errors
        case ESSMBeginSyncError:
        {
            msg.sprintf("Error in beginSync of source %s (code %d)", sourceName.c_str(), data);
            break;
        }
        case ESSMEndSyncError:
        {
            msg.sprintf("Error in endSync of source %s (code %d)", sourceName.c_str(), data);
            break;
        }
        case ESSMGetItemError:
        {
            msg.sprintf("Error getting item of source %s (code %d)", sourceName.c_str(), data);
            break;
        }
        case ESSMSetItemError:
        {
            msg.sprintf("Error setting item of source %s (code %d)", sourceName.c_str(), data);
            break;
        }
        case ESSMItemNotSupportedBySource:
        {
            msg.sprintf("Item not supported by source %s (code %d)", sourceName.c_str(), data);
            break;
        }
        case ESSMNetworkError:
        {
            if (data > 0) {
                msg.sprintf("Network error (%d attempts failed)", data);
            } else {
                msg.sprintf("Network error");
            }
            break;
        }
        case ESSMClientQuotaExceeded:
        {
            msg.sprintf("Local storage space is full for source %s", sourceName.c_str()); // add info about quota??
            break;
        }
        
        default:
        {
            msg = "MHSyncManager error";
            break;
        }
    }

    report.setLastErrorCode((int)code);
    report.setLastErrorMsg(msg.c_str());

    if (code != ESSMSuccess) {
        report.setState(SOURCE_ERROR);
        report.setLastErrorType(ERROR_TYPE_MH_SYNC_MANAGER);
        //fireSyncEvent(report.getLastErrorMsg(), SYNC_ERROR);
    }
}

ESMRStatus MHSyncManager::deleteRemoteItem(const char* guid)
{
    ESMRStatus err = ESMRNetworkError;
    
    err = mhMediaRequestManager->deleteItem(guid);
    return err;
}


int MHSyncManager::performUploads(MHSyncSource* source) {

	int lastError = ESSMSuccess;

    // Scan all the items in the cache and trigger the required operations to fix the items
    CacheItemsList cacheItemsList;
    source->getItemsFromCache(cacheItemsList);
    std::vector<MHStoreEntry*> entries = cacheItemsList.getItems();
    
    // check and launch pending tasks
    int size = entries.size();
    LOG.info("Checking for pending uploads for source %s refresh command (%d items in cache)",
             source->getConfig().getName(), size);
    
    // just for statistics
    int numLocal=0;
    int numLocalNotUploaded=0;
 
    LOG.debug("--- CACHE content for source %s ---", source->getConfig().getName());
    for (int i=0; i < size; i++) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entries[i];
        const char *itemGuid = NULL, *itemLuid = NULL;
        int itemId = 0;
        
        if (itemInfo == NULL) {
            continue;
        }
        
        itemGuid = itemInfo->getGuid().c_str();
        itemLuid = itemInfo->getLuid().c_str();
        
        itemId = itemInfo->getId();
        LOG.debug("  > #%03d: ID=%d, status=%d, %s", i, itemId, itemInfo->getStatus(), itemInfo->getName().c_str());
        
        if (itemInfo->getStatus() == EStatusLocal ||
            itemInfo->getStatus() == EStatusUploading ||
            itemInfo->getStatus() == EStatusLocalNotUploaded ||
            itemInfo->getStatus() == EStatusLocalMetaDataChanged)
        {
            if (itemInfo->getStatus() == EStatusLocalNotUploaded) {
                // previusly refused for quota exceeded
                numLocalNotUploaded++;
            } else {
                numLocal++;
            }
            // update report
            WString wluid;
            wluid = itemInfo->getLuid();
            // If this item has already a guid, then this is an update
            if (itemInfo->getGuid() == NULL || strlen(itemInfo->getGuid()) == 0) {
                report.addItem(SERVER, COMMAND_ADD, wluid.c_str(), 0, NULL);
            } else {
                // consider the item always as add even if it might be an update                     
                report.addItem(SERVER, COMMAND_ADD, wluid.c_str(), 0, NULL);
                //report.addItem(SERVER, COMMAND_REPLACE, wluid.c_str(), 0, NULL);
            }
            
            LOG.debug("%s: Requiring item upload for item %d in source %s", 
                      __FUNCTION__, itemInfo->getId(), source->getConfig().getName());

            if (fitsInTheCloud(itemInfo)) {

                int result = source->startUploadItem(itemInfo, config);

				if (result != ESSMSuccess) {
					lastError = result;

					if (result == ESSMNetworkError || 
						result == ESSMAuthenticationError) {
						// for these errors we stop all the uploads!
						LOG.error("%s: fatal error (code %d) uploading %s item %s: stop all uploads (%d out of %d done)", 
							__FUNCTION__, result, source->getConfig().getName(), itemInfo->getName().c_str(), i, size);
						break;
					}
					else {
						// for all other errors, just remember the code and continue with next item
						LOG.error("%s: error (code %d) uploading %s item %s: continue", 
							__FUNCTION__, result, source->getConfig().getName(), itemInfo->getName().c_str());
					}
				}

            } else {
				LOG.error("%s: cannot upload %s item %s: online quota exceeded (item size = %llu bytes)", 
					__FUNCTION__, source->getConfig().getName(), itemInfo->getName().c_str(), itemInfo->getSize());
                source->handleServerQuota(itemInfo);
				lastError = ESSMServerQuotaExceeded;          
            }
        }
    }
    
    // just for statistics
    numLocal -= numLocalNotUploaded;
    LOG.debug("%s: Number of uploads requests  = %d",__FUNCTION__, numLocal);
    
	// propagate last error (if 2 or more errors occurred, only the last one is returned!)
    if (lastError) {
		LOG.error("%s: Uploads phase completed with error: last error code = %d", __FUNCTION__, lastError);
    }
    return lastError;
}

int MHSyncManager::sync(MHSyncSource* source, SyncSourceConfig* srcConf, SyncDirection direction) 
{
    bool incrementalSync = initialize();
    int syncManRes = 0;
    time_t tstamp = time(NULL);
    

	// Get source local changes
    // Can stop the sync here if beginSync returns a code != 0
    int ret = source->beginSync(incrementalSync);
    if (ret != 0) {
        LOG.debug("%s: beginSync returned code %d: exit the sync of %s source", __FUNCTION__, ret, source->getConfig().getName());
        return ret;
    }

    // sets the tstamp used for local checks only.
    // next sync, will be incremental from this tstamp for local items.
    source->getConfig().setLast(tstamp);


	// get online quota info (it's used during performUploads)
	LOG.info("Checking online quota...");
    EMHMediaRequestStatus quotaStatus = mhMediaRequestManager->getQuotaInfo(&freeQuota, &userQuota);
    if (quotaStatus != ESMRSuccess) {
        LOG.error("%s cannot calculate the quota due to error: %i", __FUNCTION__, quotaStatus);
    } else {
		LOG.info("Online quota info: free = %llu, total = %llu [bytes]", freeQuota, userQuota);
	}

    if (incrementalSync) {
        // Now get the remote changes
        if (direction == TWO_WAYS || direction == SERVER_TO_CLIENT) {
            syncManRes = getServerMetadataChanges();

            // Allow the source to complete the download phase (if operations are required)
            syncManRes = source->downloadPhaseCompleted(syncManRes);
        }
        
        if (syncManRes == ESMRSuccess) {
            if (direction == TWO_WAYS || direction == CLIENT_TO_SERVER) {
                source->uploadPhaseStarted();
                syncManRes = performUploads(source);
                source->uploadPhaseCompleted(syncManRes);
            }
        }
    } else {
        // TODO: get all source items. At the moment we do not support an CLIENT_TO_SERVER
        // only refresh on the first one. This is because we apply items as soon as we get
        // them from the server and we need to get them to compute twins.
        syncManRes = this->getAllServerMetadata();
        
        // TODO: twin detection shall be done differently
        syncManRes = performTwinDetection(syncManRes);

        if (direction == TWO_WAYS || direction == SERVER_TO_CLIENT) {
            syncManRes = source->downloadPhaseCompleted(syncManRes);
        }
        if (syncManRes == ESMRSuccess) {
            if (direction == TWO_WAYS || direction == CLIENT_TO_SERVER) {
                source->uploadPhaseStarted();
                syncManRes = performUploads(source);
                source->uploadPhaseCompleted(syncManRes);
            }
        }
    }

    source->endSync(syncManRes);
    
    LOG.info("Sync of %s metadata completed with code %d", sourceName.c_str(), syncManRes);
    
    switch (syncManRes) {
        case ESSMSuccess:
		case ESSMNoLocalChangesDetected:
            srcConf->setLastSourceError(0);
            break;
            
        case ESSMNetworkError:
            srcConf->setLastSourceError(ERR_NETWORK_INIT);
            break;
            
        case ESSMCanceled:
            srcConf->setLastSourceError(ERR_OPERATION_CANCELED);
            break;
            
        case ESSMPaymentRequired:
            srcConf->setLastSourceError(ERR_HTTP_STATUS_NOT_OK);
            break;            
        case ESSMAuthenticationError:
            srcConf->setLastSourceError(ERR_HTTP_STATUS_NOT_OK);
            break;
        case ESSMProxyAuthenticationError:
            srcConf->setLastSourceError(ERR_HTTP_STATUS_NOT_OK);
            break;
        case ESSMServerQuotaExceeded:
            srcConf->setLastSourceError(ERR_SERVER_QUOTA_EXCEEDED);
            break;
        case ESSMClientQuotaExceeded:
            srcConf->setLastSourceError(ERR_CLIENT_QUOTA_EXCEEDED);
            break;   
		case ESSMReadLocalItemsError:
			srcConf->setLastSourceError(ERR_FILE_READ);
			break;
        default:
            srcConf->setLastSourceError(ERR_NETWORK_INIT);
            break;
    }
    return syncManRes;
}

bool MHSyncManager::fitsInTheCloud(MHSyncItemInfo* itemInfo) {

    bool fit = true; 
    if (userQuota == 0 && freeQuota == 0) {
        LOG.info("%s, user e free quota are 0. Probably error (network?) calculating them. Consider there is space enough.", __FUNCTION__);
        return fit;
    }
    
    int64_t currentItemSize = itemInfo->getSize();
    if (currentItemSize < freeQuota) { // item fits
        freeQuota -= currentItemSize;
    } else {
        fit = false;
    }
    return fit;

}
