/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2009 Funambol, Inc.
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

#include "MediaHub/MHSyncSource.h"
#include "MediaHub/MHContentTypes.h"
#include "MediaHub/MHSyncManager.h"
#include "MediaHub/MHLabelInfo.h"
#include "MediaHub/MHLabelsStore.h"
#include "MediaHub/MediaRequestManagerFactory.h"
#include "event/FireEvent.h"
#include "spds/AbstractSyncConfig.h"

BEGIN_FUNAMBOL_NAMESPACE


/// Returns true if passed syncMode is upload enabled
static bool isUploadEnabled(const SyncMode syncMode) {
    
    if (syncMode == SYNC_TWO_WAY || 
        syncMode == SYNC_ONE_WAY_FROM_CLIENT ||
        syncMode == SYNC_REFRESH_FROM_CLIENT) {
        return true;
    }
    return false;
}


bool MHSyncSource::init(const char* sapiSourceUri, const char* sapiSourceArrayKey,
                        const char* sourceOrderField,
                        SyncSourceConfig& sc)
{
    sapiRemoteUri = sapiSourceUri;
    sapiArrayKey = sapiSourceArrayKey;
    orderField   = sourceOrderField;
    newItemInfo     = NULL;     
    updatedItemInfo = NULL;     
    deletedItemInfo = NULL;
    isSyncingItemChanges = false;
    
    // By default at the moment we work in backward compatibility mode to avoid
    // breaking the interface
    backwardCompatibilityMode = true;
    bulkOperationsSize = 50;
   
    //LOG.debug("%s: initializing MHsyncsource", __FUNCTION__);

    // get the default directory of the 
    StringBuffer localStorage("");
    StringBuffer name = sc.getName();
    StringBuffer fileName;

    const char* ext = sc.getProperty(PROPERTY_EXTENSION);
    if (ext) {
        StringBuffer vals(ext);
        vals.split(extension, ",");
    }
    
    if (extension.size() == 0) {
        LOG.debug("[%s] No filter on extension: all allowed outgoing", __FUNCTION__);
    }
    
    return true;
}


MHSyncSource::MHSyncSource(const char* sapiSourceUri, const char* sapiSourceArrayKey,
                           const char* sourceOrderField,
                           SyncSourceConfig& sc, SyncSourceReport& rep, 
                           MHItemsStore* itemsStore,
                           MHLabelsStore* labelsStore,
                           size_t incomingFilterD, size_t outgoingFilterD,
                           bool recursive_) 
                : sapiRemoteUri(sapiSourceUri), sapiArrayKey(sapiSourceArrayKey),
                  orderField(sourceOrderField),
                  config(sc), report(rep), mhItemsStore(itemsStore), mhLabelsStore(labelsStore),
                  incomingFilterDate(incomingFilterD), outgoingFilterDate(outgoingFilterD), recursive(recursive_) {
    
    if (init(sapiSourceUri, sapiSourceArrayKey, sourceOrderField, sc) == false) {
        LOG.debug("[%s] - initialization failed", __FUNCTION__);
    }
}



MHSyncSource::~MHSyncSource() {

    delete newItemInfo;
    delete updatedItemInfo;
    delete deletedItemInfo;
}



bool MHSyncSource::filterByExtension(MHSyncItemInfo& itemInfo) {
 
    if (extension.size() == 0) {        
        return false; // wee keep it
    }
    
    StringBuffer s = itemInfo.getName();
    s.lowerCase();
    
    bool allowed = true;    
    
    if (extension.size() > 0) {
        StringBuffer* val = (StringBuffer*)extension.get(0);
        if (val && strcmp(val->c_str(), "!") == 0) {
            allowed = false;
        }
    }    
    
    bool ret;
    int i;
    
    if (allowed) { ret = true;   i = 0; } 
    else         { ret = false;  i = 1; }
    
    for (; i < extension.size(); i++) {
        if (s.endsWith(((StringBuffer*)extension.get(i))->c_str())) {
            if (allowed) { ret = false; } 
            else         { ret = true;  }            
            break;
        }
    }

    return ret;
}

bool MHSyncSource::filterFromBlackList(MHSyncItemInfo& itemInfo) {
    //
    // TODO
    //
    //LOG.info("%s: not implemented", __FUNCTION__);
    return false;
}


bool MHSyncSource::filterBySize(MHSyncItemInfo& itemInfo) {
    
    // Check for item size  (0 means disabled)
    
    bool err;
    int64_t maxItemSize = config.getLongLongProperty(PROPERTY_SYNC_ITEM_MAX_SIZE, &err);
    
    if (!err && maxItemSize>0) {
        int64_t itemSize = itemInfo.getSize();
        
        if (itemSize > maxItemSize) {
            LOG.info("%s: discarding local item of size %lu (maximum item size allowed exceeded)", __FUNCTION__, itemSize);
            return true;
        }
    }
    
    return false;
}

bool MHSyncSource::filterOutgoingItem(MHSyncItemInfo& clientItemInfo) {
        
    //
    // 1. Check for item size  (0 means disabled)
    //
    if (filterBySize(clientItemInfo)) {
        return true;
    }
    
    //
    // 2. Check if the modification date is over the filter date
    //
    if (outgoingFilterDate > 0) {
        if (clientItemInfo.getModificationDateSecs() >= outgoingFilterDate) {
            return false;  // we keep it
        } else {
            StringBuffer modDate   = unixTimeToString((unsigned long)clientItemInfo.getModificationDateSecs(), false);
            StringBuffer outFilter = unixTimeToString((unsigned long)outgoingFilterDate, false);
            LOG.debug("%s: item %s discarded (mod date local = %s, current outgoing filter = %s)",
                    __FUNCTION__, clientItemInfo.getName().c_str(), modDate.c_str(), outFilter.c_str());
            return true;   // we discard it
        }
    }

    return false;
}

MHSyncItemInfo* MHSyncSource::twinDetection(MHSyncItemInfo& serverItemInfo, std::vector<MHStoreEntry*> localEntries) {
    MHSyncItemInfo* ret = NULL;
    std::vector<MHStoreEntry*>::iterator it = localEntries.begin();
    std::vector<MHStoreEntry*>::iterator endIt = localEntries.end();
    while(it != endIt) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)*it;
        if (itemInfo->getSize() == serverItemInfo.getSize()) {
            if (itemInfo->getName() != serverItemInfo.getName()) {
                LOG.debug("%s: Item size is the same but name is different. Server: %s - Client: %s", 
                              __FUNCTION__, serverItemInfo.getName().c_str(), itemInfo->getName().c_str());
            }

            ret = (MHSyncItemInfo*)new MHSyncItemInfo(*itemInfo);
            it = localEntries.erase(it);
            endIt = localEntries.end();
            break;
        } else {
            ++it;
        }
    }
    return ret;
} 

bool MHSyncSource::filterIncomingItem(MHSyncItemInfo& serverItemInfo, time_t offsetTime) {

    bool ret = false;  // we want to keep it from the server
    bool err;
   
    //
    // 1. (blacklisted item) if the item is not supported by sync source don't to download it.      *** TODO ***
    //
    ret = filterFromBlackList(serverItemInfo);
    if (ret == true) {
        LOG.info("Discarding server item '%s' (blacklisted for source %s)", 
            serverItemInfo.getName().c_str(), getConfig().getName());
        return ret;
    }
    
    //
    // 2. if the extension is not supported we don't want to download it.
    //
    ret = filterByExtension(serverItemInfo);
    if (ret == true) {
        LOG.info("Discarding server item '%s' (extension not supported for source %s)", 
            serverItemInfo.getName().c_str(), getConfig().getName());
        return ret;
    }
    
    //
    // 3. Check for item size (0 means disabled)
    //
    int64_t maxItemSize = config.getLongLongProperty(PROPERTY_SYNC_ITEM_MAX_SIZE, &err);
    
    if (!err  && maxItemSize>0) {
        int64_t itemSize = serverItemInfo.getSize();
        
        if (itemSize > maxItemSize) {
            LOG.info("Discarding server item of size %llu (maximum item size allowed exceeded for source %s)", 
                itemSize, getConfig().getName()); 
            return true;
        }
    }
    
    //
    // 4. if the date is less then the filter one, we don't want it
    //
    if (incomingFilterDate > 0) {
        if (serverItemInfo.getModificationDateSecs() < (incomingFilterDate - offsetTime)) {
            ret = true;  // we discard it from the server
            StringBuffer modDate  = unixTimeToString((unsigned long)serverItemInfo.getModificationDateSecs(), false);
            StringBuffer inFilter = unixTimeToString((unsigned long)(incomingFilterDate - offsetTime), false);
            LOG.debug("%s: item %s discarded (mod date from server = %s, current incoming filter = %s)",
                    __FUNCTION__, serverItemInfo.getName().c_str(), modDate.c_str(), inFilter.c_str());
        }
    }

    return ret;
    
}

int MHSyncSource::deleteItem(MHSyncItemInfo* itemInfo) 
{
    LOG.info("The %s is not implemented...", __FUNCTION__);
    return -1;    
}

int MHSyncSource::getItemInfoIndex(const StringBuffer& luid, std::vector<MHStoreEntry*> list) {
    
    for (unsigned int i=0; i<list.size(); i++) {
        MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)list[i];
        if (!itemInfo) continue;
        
        if (itemInfo->getLuid() == luid) {  
            // found!
            return i;
        }                
    } 
    // not found
    return -1;
}


bool MHSyncSource::isErrorCode(int code) {    
    if (code == 0) {
        return false;
    } else {
        return true;
    }
}

int MHSyncSource::beginSync(bool incremental) {
	return 0;
}

void MHSyncSource::endSync(int res) {   
}


void MHSyncSource::normalizeSlash(StringBuffer& s) {
    s.replaceAll("\\", "/");
}

int MHSyncSource::cleanTemporarySpoolItem(const StringBuffer& item) {
    
    LOG.info("The %s is not implemented...", __FUNCTION__);
    return -2;
}

int MHSyncSource::blackListItem(MHSyncItemInfo* itemInfo)
{
    LOG.info("The %s is not implemented...", __FUNCTION__);
    return 0;
}
      


bool MHSyncSource::resetCache() {
    
    return mhItemsStore->removeAllEntries();
}

bool MHSyncSource::addItemsToCache(std::vector<MHOperationDescriptor*>& operations) {
    
    if (mhItemsStore == NULL) {
        LOG.error("%s: no item cache defined", __FUNCTION__);
        return false;
    }

    
    bool res = true;
    if (backwardCompatibilityMode) {
        // Add the operation 1 by 1
        std::vector<MHOperationDescriptor*>::iterator it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            bool success = addItemToCache((MHSyncItemInfo*)opDesc->getNewEntry(), opDesc->getLabels());
            opDesc->setSuccess(success);
            if (!success) {
                res = false;
            }
        }
    } else {
        std::vector<MHStoreEntry*> entries;
        std::vector<MHOperationDescriptor*>::iterator it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            entries.push_back(opDesc->getNewEntry());
            if (entries.size() == bulkOperationsSize) {
                mhItemsStore->addEntries(entries);
                entries.clear();
            }
        }
        if (entries.size() > 0) {
            mhItemsStore->addEntries(entries);
        }
        // Now handle labels
        it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            std::vector<MHLabelInfo*>* labels = opDesc->getLabels();
            if (labels != NULL) {
                if (mhLabelsStore == NULL) {
                    LOG.info("%s: Ignoring labels because source is not configured to support them",__FUNCTION__);
                } else {
                    // First of all we check if all the labels already exist. If not they are created.
                    addMissingLabels(labels);
                    // Now associate the item to the label
                    addLabelsToItem((MHSyncItemInfo*)opDesc->getNewEntry(), labels);
                }
            }
        }
    }
    return res;
}


bool MHSyncSource::addItemToCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels)
{
    if (mhItemsStore == NULL) {
        LOG.error("%s: no item cache defined", __FUNCTION__);
        return false;
    }
        
    if (itemInfo == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return false;
    }

    mhItemsStore->AddEntry(itemInfo);    
    
    // Handle labels if supported by this source
    if (labels != NULL) {
        if (mhLabelsStore == NULL) {
            LOG.info("%s: Ignoring labels because source is not configured to support them",__FUNCTION__);
        } else {
            // First of all we check if all the labels already exist. If not they are created.
            addMissingLabels(labels);
            // Now associate the item to the label
            addLabelsToItem(itemInfo, labels);
        }
    }
    
    return true;
}

bool MHSyncSource::addMissingLabels(std::vector<MHLabelInfo*>* labels) {
    std::vector<MHLabelInfo*>::iterator labelIt = labels->begin();
    bool added = false;
    for(;labelIt != labels->end();++labelIt) {
        MHLabelInfo* label = *labelIt;
        uint32_t labelGuid = label->getGuid();
        StringBuffer guidStr; guidStr.sprintf("%u",labelGuid);
        // char guidStr[32];
        // snprintf(guidStr, 32, "%u",labelGuid);
        MHLabelInfo* exLabel = (MHLabelInfo*)mhLabelsStore->getEntry(MHLabelsStore::guid_field_name, guidStr.c_str());
        if (exLabel == NULL) {
            // The label does not exist, we need to create it
            LOG.debug("%s: Adding label %s",__FUNCTION__,label->getName().c_str());
            added = mhLabelsStore->AddEntry(label);
        } else {
            // The label already exists, but it may have been updated
            label->setLuid(exLabel->getLuid());
            LOG.debug("%s: Updating existing label %s",__FUNCTION__,exLabel->getName().c_str());
            mhLabelsStore->UpdateEntry(label);
        }
        delete exLabel;
    }
    return added;
}

bool MHSyncSource::addLabelsToItem(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels) {
    MHItemsStore* itemsStore = (MHItemsStore*)mhItemsStore;
    int32_t numAdded = itemsStore->addLabelsToItem(itemInfo, labels);
    return numAdded == labels->size();
}

bool MHSyncSource::clearLabelsForItem(MHSyncItemInfo* itemInfo) {
    MHItemsStore* itemsStore = (MHItemsStore*)mhItemsStore;
    int32_t numRemoved = itemsStore->clearItemLabels(itemInfo);
    return numRemoved >= 0;
}

bool MHSyncSource::updateItemsInCache(std::vector<MHOperationDescriptor*>& operations) {
    LOG.debug("%s: Updating items in cache",__FUNCTION__);
    bool res = true;
    if (backwardCompatibilityMode) {
        // Add the operation 1 by 1
        std::vector<MHOperationDescriptor*>::iterator it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            bool success = updateItemInCache((MHSyncItemInfo*)opDesc->getOldEntry(),
                                             (MHSyncItemInfo*)opDesc->getNewEntry(), opDesc->getLabels());
            opDesc->setSuccess(success);
            if (!success) {
                res = false;
            }
        }
    } else {
        std::vector<MHStoreEntry*> entries;
        std::vector<MHOperationDescriptor*>::iterator it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            entries.push_back(opDesc->getNewEntry());
            if (entries.size() == bulkOperationsSize) {
                mhItemsStore->updateEntries(entries);
                entries.clear();
            }
        }
        if (entries.size() > 0) {
            mhItemsStore->updateEntries(entries);
        }
        // Now handle labels
        it = operations.begin();
        for(;it != operations.end();++it) {
            MHOperationDescriptor* opDesc = *it;
            std::vector<MHLabelInfo*>* labels = opDesc->getLabels();
            if (labels != NULL) {
                if (mhLabelsStore == NULL) {
                    LOG.info("%s: Ignoring labels because source is not configured to support them",__FUNCTION__);
                } else {
                    clearLabelsForItem((MHSyncItemInfo*)opDesc->getNewEntry());
                    // First of all we check if all the labels already exist. If not they are created.
                    addMissingLabels(labels);
                    // Now associate the item to the label
                    addLabelsToItem((MHSyncItemInfo*)opDesc->getNewEntry(), labels);
                }
            }
        }
    }
    return res;
}


bool MHSyncSource::updateItemInCache(MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels)
{
    return updateItemInCache(NULL, itemInfo, labels);
}


bool MHSyncSource::updateItemInCache(MHSyncItemInfo* oldItemInfo, MHSyncItemInfo* itemInfo, std::vector<MHLabelInfo*>* labels)
{
    if (mhItemsStore == NULL) {
        LOG.error("%s: no item cache defined", __FUNCTION__);
        return false;
    }
    
    if (itemInfo == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return false;
    }
    
    mhItemsStore->UpdateEntry(itemInfo);
    
    if (labels != NULL) {
        if (mhLabelsStore == NULL) {
            LOG.info("%s: Ignoring labels because source is not configured to support them",__FUNCTION__);
        } else {
            // First of all we clear all the labels associated to this item and recreate them after
            clearLabelsForItem(itemInfo);
            // First of all we check if all the labels already exist. If not they are created.
            addMissingLabels(labels);
            // Now associate the item to the label
            addLabelsToItem(itemInfo, labels);
        }
    }
    
    return true;
}

bool MHSyncSource::removeItemFromCache(MHSyncItemInfo* itemInfo)
{
    if (mhItemsStore == NULL) {
        LOG.error("%s: no MH Items store defined", __FUNCTION__);
        return false;
    }
    
    if (itemInfo == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return false;
    }
    
    bool ret = mhItemsStore->RemoveEntry(itemInfo);
    return ret;
}

void MHSyncSource::setContentTypeByExtension(MHSyncItemInfo* itemInfo)
{
    unsigned int pos = itemInfo->getName().rfind(".");
    if (pos != StringBuffer::npos) {            
        StringBuffer extension = itemInfo->getName().substr(pos);    
        StringBuffer mime = MHContentType::getContentTypeByExtension(extension);
        itemInfo->setContentType(mime.c_str());
    }
}

int MHSyncSource::getCountFromCache() {     
    return mhItemsStore->getCount();
}

bool MHSyncSource::getItemsFromCache(CacheItemsList& itemsInfoList)
{ 
    return mhItemsStore->getAllEntries(itemsInfoList);
}

MHSyncItemInfo* MHSyncSource::getItemFromCache(const unsigned long itemID)
{
    return (MHSyncItemInfo*)mhItemsStore->getEntry(itemID);
}

MHSyncItemInfo* MHSyncSource::getItemFromCache(const char* fieldName,
                                               const char* fieldValue)
{
    return (MHSyncItemInfo*)mhItemsStore->getEntry(fieldName, fieldValue);
}

int MHSyncSource::startUploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& mainConfig)
{
    return uploadItem(itemInfo, mainConfig, NULL);
}

int MHSyncSource::downloadPhaseCompleted(int res) {
    return 0;
}


void MHSyncSource::uploadPhaseStarted() {

}

int MHSyncSource::uploadPhaseCompleted(int res) {
    return 0;
}

int MHSyncSource::performTwinDetection(AbstractSyncConfig& config) {
    return 0;
}

int MHSyncSource::uploadItem(MHSyncItemInfo* itemInfo, AbstractSyncConfig& mainConfig,
                             HttpConnectionUploadObserver* uploadObserver) 
{
    StringBuffer guid, luid;
    EMHMediaRequestStatus err = ESMRSuccess;
    UploadMHSyncItem* uploadItem = NULL;
    EItemInfoStatus itemStatus = EStatusUndefined;
    MHSyncItemInfo* responseItem;
    
    int uploadStatus = -1;
    time_t lastUpdate = 0;
    bool isUpdate = false;
    
    if (!itemInfo) {
        LOG.error("Internal error: NULL upload item info");
        return ESSMGenericSyncError;
    }
    
    if (mainConfig.isToAbort()) {
        return ESSMCanceled;
    }
    
    luid = itemInfo->getLuid();
    if (luid.empty()) {
        LOG.error("Internal error: empty upload item LUID");
        return ESSMGenericSyncError;
    }
    
    // safe check (content type is required)
    if (itemInfo->getContentType().empty()) {
        setContentTypeByExtension(itemInfo);
    }
    
    LOG.debug("%s: Uploading item with name: %s",__FUNCTION__,itemInfo->getName().c_str());
    
    uploadItem = createUploadItem(itemInfo);

    bool resumeRequired = false;
    if (uploadItem == NULL) {
        LOG.error("error creating upload item for id %lu", itemInfo->getId());
        return ESSMMediaHubPathNotFound;
    }
    
    URL url(mainConfig.getSyncURL());
    StringBuffer host = url.getHostURL();
    MediaRequestManagerFactory* reqManFactory = MediaRequestManagerFactory::getInstance();
    MHMediaRequestManager* mhMediaRequestManager = reqManFactory->getMediaRequestManager(host,
                                                getSapiUri(),
                                                getSapiArrayKey(),
                                                getOrderField(),
                                                createItemJsonParser(),
                                                &mainConfig);
                               
    // set http params, read from config
    mhMediaRequestManager->setRequestTimeout   (mainConfig.getMHRequestTimeout());
    mhMediaRequestManager->setResponseTimeout  (mainConfig.getMHResponseTimeout());
    mhMediaRequestManager->setUploadChunkSize  (mainConfig.getMHUploadChunkSize());
    mhMediaRequestManager->setDownloadChunkSize(mainConfig.getMHDownloadChunkSize());
    
    //
    // ----- Check if resume upload ----
    //
    int64_t firstByteToSend = 0;
    itemStatus = itemInfo->getStatus();
    
    if (itemStatus == EStatusUploading) {
        int resumeStatus = resumeUpload(*uploadItem, mainConfig, *mhMediaRequestManager,
                                        firstByteToSend);
        if (resumeStatus == 1) {
            // whole item already uploaded
            uploadStatus = 0;
            goto finally;
        } else {
            if (uploadItem->getMHSyncItemInfo()->getGuid().empty()) {
                LOG.info("%s: reset guid to upload new item", __FUNCTION__);                
                itemInfo->setGuid("");
            } else {
                resumeRequired = true;
            }
        }
    }
    
    
    //
    // ----- Save item metadata on Server -----
    //
    guid = itemInfo->getGuid();
    if (guid.empty() == false && !resumeRequired) {
        LOG.info("%s This item already has its GUID, this is either an upload or a resume metadata: %s (GUID = '%s')", __FUNCTION__,
                 itemInfo->getName().c_str(), itemInfo->getGuid().c_str());
        isUpdate = true;
    } 

    if (resumeRequired) {
        if (uploadObserver != NULL && firstByteToSend > 0) {
            uploadObserver->setStartProgress(firstByteToSend - 1);
        }
    } else {
        bool updateMetaDataOnly = false;
        
        LOG.info("Uploading %s metadata: %s (GUID = '%s')", getConfig().getName(), 
                 itemInfo->getName().c_str(), itemInfo->getGuid().c_str());
        
        if (itemStatus == EStatusLocalMetaDataChanged) {
            updateMetaDataOnly = true;
        }
        
        err = mhMediaRequestManager->uploadItemMetaData(uploadItem, updateMetaDataOnly);
        if (err == ESMRQuotaExceeded) {
            LOG.info("Can't upload item %s: server quota exceeded. Update status in cache", itemInfo->getName().c_str());
            uploadStatus = ESSMServerQuotaExceeded;
            itemInfo->setStatus(EStatusLocalNotUploaded);
            updateItemInCache(itemInfo, NULL);
            goto finally;
        }

        if (err == ESMRErrorRetrievingMediaItem) {
            // retry from scratch if MED-1005 (GUID not found server side)
            LOG.info("Item '%s' not found remotely: retry uploading item from scratch (clear the GUID)", 
                uploadItem->getMHSyncItemInfo()->getName().c_str());
            uploadItem->getMHSyncItemInfo()->setGuid("");

            // It's no more an update, it's a new item (so the GUID is correctly set later) and we must upload also the binary.
            isUpdate = false;
            updateMetaDataOnly = false;
            itemStatus = EStatusLocal;
            itemInfo->setStatus(EStatusLocal);

            err = mhMediaRequestManager->uploadItemMetaData(uploadItem, updateMetaDataOnly);
        }
        
        if (err != ESMRSuccess) {
            uploadStatus = getUploadSpecificError(err);
            goto finally;
        }
    }

    if (isUpdate == false) {
        // safe check on server guid generation
        guid = uploadItem->getMHSyncItemInfo()->getGuid();
        if (guid.empty()) {
            LOG.error("%s: empty upload item GUID, cannot proceed with update", __FUNCTION__);
            uploadStatus = ESSMGenericSyncError;
            goto finally;
        }
    }
   
    if (itemStatus == EStatusLocalMetaDataChanged) {
        LOG.info("Item metadata updated for item '%s': no need to upload binary", itemInfo->getName().c_str());
        itemInfo->setStatus(EStatusRemote);
        updateItemInCache(itemInfo, NULL);
        uploadStatus = 0;
        
        goto finally;
    }
    
    if (mainConfig.isToAbort()) {
        uploadStatus = ESSMCanceled;
        goto finally;
    }
    
    //
    // ----- Upload item data content -----
    //
    LOG.info("Uploading %s data: %s (GUID = '%s')...", getConfig().getName(),
             itemInfo->getName().c_str(), guid.c_str());
    
    // update item in cache
    if (isUpdate == false) {
        itemInfo->setGuid(guid);
    }
    
    itemInfo->setStatus(EStatusUploading);
    updateItemInCache(itemInfo, NULL);
        
    responseItem = itemInfo->clone();
    err = mhMediaRequestManager->uploadItemData(uploadItem, &lastUpdate, isUpdate, uploadObserver, responseItem);
    
    if (err != ESMRSuccess) {
        if (err == ESMRQuotaExceeded) {
            LOG.info("Can't upload item %s: server quota exceeded", itemInfo->getName().c_str());
        } else if (err == ESMRUnknownMediaException || 
                   err == ESMRErrorRetrievingMediaItem) {
            LOG.info("Media exception on Server (SMR code %d), reset the guid so it will be retried from scratch next time", err);
            itemInfo->setGuid("");
            updateItemInCache(itemInfo, NULL);
        }
        uploadStatus = getUploadSpecificError(err);
        goto finally;
    }
    
    
    // upload was ok: update item status in cache
    itemInfo->setServerLastUpdate(lastUpdate);
    
    // The upload was successfull, so we mark the item as remote even if due to content
    // validation it might be unavailable.
    itemInfo->setStatus(EStatusRemote);
    itemInfo->setValidationStatus(responseItem->getValidationStatus());
    
    // if the validation status is "uploaded or copyrighted" then the responseItem must be valid
    if (responseItem->getId() == -1) {
        // The server did not provide the uploaded meta data info
        if (itemInfo->getValidationStatus() == EValidationStatusValid || itemInfo->getValidationStatus() == EValidationStatusCopyrighted) {
            LOG.error("%s: The server returned an item in status %d but did not return the full item in its upload response",__FUNCTION__,itemInfo->getValidationStatus());
        }
    } else {
        itemInfo->setRemoteItemETag(responseItem->getRemoteItemETag().c_str());
        itemInfo->setRemoteThumbETag(responseItem->getRemoteThumbETag().c_str());
        itemInfo->setRemotePreviewETag(responseItem->getRemotePreviewETag().c_str());
        itemInfo->setRemoteItemUrl(responseItem->getRemoteItemUrl());
        itemInfo->setLocalItemETag(responseItem->getRemoteItemETag().c_str());
        itemInfo->setServerUrl(responseItem->getServerUrl());
        itemInfo->setRemoteThumbUrl(responseItem->getRemoteThumbUrl());
        itemInfo->setRemotePreviewUrl(responseItem->getRemotePreviewUrl());
        itemInfo->setRemoteItemUrl(responseItem->getRemoteItemUrl());
    }
    updateItemInCache(itemInfo, NULL);
    
    delete responseItem;
    
    // delete tmp local item
    cleanTemporarySpoolItem(itemInfo->getLuid());
    
    LOG.info("Upload of %s item '%s' complete (guid = %s)", getConfig().getName(),
             itemInfo->getName().c_str(), guid.c_str());
    uploadStatus = 0;   // success
    
finally:
    delete uploadItem;
    delete mhMediaRequestManager;
    
    return uploadStatus;
}


int MHSyncSource::resumeUpload(UploadMHSyncItem& resumeItem,
                               AbstractSyncConfig& mainConfig,
                               MHMediaRequestManager& mhMediaRequestManager,
                               int64_t& firstByteToSend) 
{
    int64_t offset = 0;
    firstByteToSend = 0;
    EMHMediaRequestStatus err;
    InputStream* stream = NULL;
    int resumeStatus = -1;
    
    MHSyncItemInfo* itemInfo = resumeItem.getMHSyncItemInfo();
    if (!itemInfo) {
        LOG.error("Internal error: NULL upload item info");
        return ESSMGenericSyncError;
    }
    
    StringBuffer luid = itemInfo->getLuid();
    StringBuffer guid = itemInfo->getGuid();
    
    if (mainConfig.isToAbort()) {
        resumeStatus = ESSMCanceled;
        goto finally;
    }
    
    
    // Get info about how much data the Server already have
    offset = 0;
    LOG.info("Get resume informations on item to upload: %s", itemInfo->getName().c_str());
    err = mhMediaRequestManager.getItemResumeInfo(&resumeItem, &offset);
    
    if (err != ESMRSuccess) {
        if (err == ESMRInvalidParam ||                  
            err == ESMRMHMessageFormatError || 
            err == ESMRHTTPFunctionalityNotSupported || 
            err == ESMRMHNotSupported || 
            err == ESMRInvalidContentRange  ||          
            err == ESMRUnknownMediaException ||
            err == ESMRGenericHttpError) {
            // For these error codes, we DON'T resume operation but retry item upload from scratch  
            
			LOG.info("%s item '%s' can't be resumed, will be uploaded from scratch", getConfig().getName(), itemInfo->getName().c_str());
            if (err == ESMRUnknownMediaException) {
                LOG.info("The item is no more valid on server so also the metadata will be renew");
                itemInfo->setGuid("");
            }
            
            resumeStatus = 0;
            goto finally;
        }
        
        resumeStatus = getUploadSpecificError(err);
        goto finally;
    }
    
    if (mainConfig.isToAbort()) {
        resumeStatus = ESSMCanceled;
        goto finally;
    }
    
    // 'offset' is the last byte the server already has
    firstByteToSend = offset + 1;
    
    if (firstByteToSend >= itemInfo->getSize()) {
        LOG.info("Server already has all the item's data: skip upload of %s", itemInfo->getName().c_str());
        
        itemInfo->setStatus(EStatusRemote);
        updateItemInCache(itemInfo, NULL);
        
        resumeStatus = 1;
        goto finally;
    }
    
    // Set the offset into the item
    LOG.info("Resume upload of item %s (from byte %lld to byte %lld)", 
             itemInfo->getName().c_str(), firstByteToSend, itemInfo->getSize());
    stream = resumeItem.getStream();
    if (stream) {
        stream->setPosition(firstByteToSend);
    }
    
    // All OK
    resumeStatus = 0;
    
finally:
    return resumeStatus;
}

int MHSyncSource::getUploadSpecificError(int err) {
    ESMRStatus status = (ESMRStatus)err;
    int code;
    switch (status)
    {
        case ESMRAccessDenied:
        {
            code = ESSMAuthenticationError;
            break;
        }
            
        case ESMRHTTPFunctionalityNotSupported:
        case ESMRMHNotSupported:
        {
            code = ESSMMHNotSupported;
            break;
        }
            
        case ESMRConnectionSetupError:
        case ESMRGenericHttpError:
        case ESMRNetworkError:
        case ESMRRequestTimeout:
        {
            code = ESSMNetworkError;
            break;
        }
        case ESMRQuotaExceeded:
        {
            code = ESSMServerQuotaExceeded;
            break;
        }
            
        case ESMROperationCanceled:
        {
            code = ESSMCanceled;
            break;
        }
            
        case ESMRPaymentRequired:
        {
            code = ESSMPaymentRequired;
            break;
        }
            
        case ESMRUnknownMediaException:
        case ESMRErrorRetrievingMediaItem:
        {
            code = ESSMUnknownMediaException;
            break;
        }

		case ESMRCannotReadInputStream:
		{
			code = ESSMReadLocalItemsError;
			break;
		}
            
        default:
        {
            code = ESSMGenericSyncError;
            break;
        }
    }
    return code;
}

MHItemJsonParser* MHSyncSource::createItemJsonParser() {
    return new MHItemJsonParser();
}


void MHSyncSource::handleServerQuota(MHSyncItemInfo* itemInfo) {
    itemInfo->setStatus(EStatusLocalNotUploaded);
    updateItemInCache(itemInfo, NULL);
}


END_FUNAMBOL_NAMESPACE

