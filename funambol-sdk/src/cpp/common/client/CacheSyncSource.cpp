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

#include "base/fscapi.h"
#include "base/Log.h"
#include "client/CacheSyncSource.h"
#include "base/adapter/PlatformAdapter.h"

#include "spds/spdsutils.h"
#include "spds/SyncSourceConfig.h"

#include "base/util/KeyValuePair.h"
#include "base/util/PropertyFile.h"
#include "base/util/ArrayListEnumeration.h"

BEGIN_NAMESPACE

// the name of the repository
#define CACHE_FOLDER    "item_cache"
#define CACHE_FILE_EXT  ".dat"


// Compose the cache folder taking the config folder from
// the platform adapter and tries to create it if not present.
static bool initCacheDir(StringBuffer& dir) {
    
    dir = PlatformAdapter::getConfigFolder();
    int len = dir.length();
    
    if (len == 0) {
        return false;
    }
    
    char pathEnd = dir[len - 1];
    // check if cache dir path terminates with slash or backslash
    if ((pathEnd != '/') && (pathEnd != '\\')) {
        dir += "/";
    }
    
    dir += CACHE_FOLDER;

    if (createFolder(dir)){
        LOG.error("initCacheDir: error creating cache directory '%s'", dir.c_str());
        dir = NULL;
        return false;
    }
    return true;
}


CacheSyncSource::CacheSyncSource(const WCHAR* sourceName,
                                 AbstractSyncSourceConfig *sc,
                                 KeyValueStore* cache) : SyncSource(sourceName, sc) {
   
    allKeys = NULL;
    newKeys = NULL; 
    updatedKeys = NULL; 
    deletedKeys = NULL;   

    if (cache) {
        this->cache = cache;
    } else {
        // get the default directory of the 
        StringBuffer cacheFileName;
        StringBuffer name;
        name.convert(sourceName);

        if(initCacheDir(cacheFileName)) {                
            cacheFileName += "/";
            cacheFileName += name;
            cacheFileName += CACHE_FILE_EXT;

            LOG.debug("CacheSyncSource: cache pathname is %s", cacheFileName.c_str());
            this->cache = new PropertyFile(cacheFileName);
        }
        else {
            setErrorF(ERR_FILE_SYSTEM, "Cannot create cache file, sync not done.");
            this->cache = NULL; //cannot create the cache file
        }
    }
}

/**
 * Release dynamically allocated resources
 */
CacheSyncSource::~CacheSyncSource() {   

    if (newKeys)     { delete newKeys;     } 
    if (updatedKeys) { delete updatedKeys; } 
    if (deletedKeys) { delete deletedKeys; } 
    if (allKeys)     { delete allKeys;     }
    if (cache)       { delete cache;       }
}

/**
* Fill a key value pair object and set into the partialModification array list.
* This is used to set the changes in the cache or in the listener based sync source.
* If there is an error this error is reported in the saving procedure and there
* it is decided how procede
*/
void CacheSyncSource::setItemStatus(const WCHAR* wkey, int status, const char* command) {
    
    StringBuffer key;
    key.convert(wkey);
    
    KeyValuePair vp;
    if (!isErrorCode(status)) {
        LOG.debug("[%s], Received success status code from server for %s on item with key %s - code: %d", getConfig().getName(), command, key.c_str(), status);        
        vp.setKey(key.c_str());
        if (strcmp(command, DEL)) {
            vp.setValue(getItemSignature(key));
        }
    } else {
        if (status == STC_CHUNKED_ITEM_ACCEPTED) {
            // code 213 is not an error
            LOG.debug("[%s], Chunk accepted for %s on item with key %s - code: %d", getConfig().getName(), command, key.c_str(), status);  
        } else {
            // 500 etc...
            LOG.info("[%s], Received failed status code from server for %s on item with key %s - code: %d", getConfig().getName(), command, key.c_str(), status);
            // error. it doesn't update the cache
        }
       
    }    
    if (vp.getKey()) {
        updateInCache(vp, command);
    }

}


StringBuffer CacheSyncSource::getItemSignature(StringBuffer& key) 
{
    StringBuffer s("");
    void* content       = NULL;
    size_t size         = 0;
    
    if (key.length() <= 0) {
        return s;
    }
    
    LOG.debug("[%s] Getting signature for item with key %s", getConfig().getName(), key.c_str());
    
    content = getItemContent(key, &size);                      
    
    s.sprintf("%ld", calculateCRC(content, size));
    if (content) { delete [] (char*)content; content = NULL; }    
    return s;
}



SyncItem* CacheSyncSource::fillSyncItem(StringBuffer* key, const bool fillData) {
    
    SyncItem* syncItem  = NULL;    
    size_t size         = 0;
    void* content       = NULL;
    
    if (!key) {
        return NULL;
    }
    
    //LOG.debug("[%s] Filling item with key %s", getConfig().getName(), key->c_str());
    
    WCHAR* wkey = toWideChar(key->c_str());
    syncItem = new SyncItem(wkey);
    
    if (fillData) {
        content = getItemContent((*key), &size);
        syncItem->setData(content, size);
    }
    
    if (wkey) { delete [] wkey; wkey = NULL; }
    if (content) { delete [] (char*)content; content = NULL; }

    return syncItem;

}

/**
 * Return the first SyncItem of all.
 * It is used in case of slow sync and retrieve the entire 
 * data source content.
 */
SyncItem* CacheSyncSource::getFirstItem() {
    
    // A slow sync is started -> clear the cache
    clearCache();

    allKeys = getAllItemList();   

    /* @TODO. some reset of the iterator??
    // it is an Enumeration so we have to loop on the keys
    int count = 0;
    while (allKeys && allKeys->hasMoreElement()) {
        count++;
        allKeys->getNextElement();
    }
    fireClientTotalNumber(count);
    */

    return getNextItem();

};

 /**
 * Return the next SyncItem of all.
 * It is used in case of slow sync
 * and retrieve the entire data source content.
 */
SyncItem* CacheSyncSource::getNextItem() {
    
    SyncItem* syncItem = NULL;
    if (allKeys && allKeys->hasMoreElement()) {
        StringBuffer* s = (StringBuffer*)allKeys->getNextElement();    
        syncItem = fillSyncItem(s);
    }
    
    if (syncItem) {
        StringBuffer key;
        key.convert(syncItem->getKey());
        LOG.debug("[%s] Sending item: key = %s", getConfig().getName(), key.c_str());
    }
    else {
        LOG.debug("There are no more items to be exchanged. Return NULL");     
    }
    return syncItem;

}
/**
 * Return the first new SyncItem. It is used in case of fast sync
 * and retrieve the new data source content.
 */
SyncItem* CacheSyncSource::getFirstNewItem() {    
    fillItemModifications();   
    
    /* @TODO some reset of iterator???
    // get the total number of items
    int count = 0;
    while (newKeys && newKeys->hasMoreElement()) {
        count++;
        newKeys->getNextElement();
    }
    while (updatedKeys && updatedKeys->hasMoreElement()) {
        count++;
        updatedKeys->getNextElement();
    }
    while (deletedKeys && deletedKeys->hasMoreElement()) {
        count++;
        deletedKeys->getNextElement();
    }
    fireClientTotalNumber(count);
    */
    return getNextNewItem();    

}

/**
 * Return the next SyncItem of new one. It is used in case of fast sync
 * and retrieve the new data source content.
 */
SyncItem* CacheSyncSource::getNextNewItem() {
    
    SyncItem* syncItem = NULL;
    if (newKeys && newKeys->hasMoreElement()) {
        StringBuffer* s = (StringBuffer*)newKeys->getNextElement();    
        syncItem = fillSyncItem(s);
    }
    
    if (syncItem) {
        StringBuffer key;
        key.convert(syncItem->getKey());
        LOG.debug("[%s] Sending new item: key = %s", getConfig().getName(), key.c_str());
    }
    else {
        LOG.debug("There are no more new items to be exchanged. Return NULL");     
    }
    return syncItem;   
}

/**
 * Return the first SyncItem of updated one. It is used in case of fast sync
 * and retrieve the new data source content.
 */
SyncItem* CacheSyncSource::getFirstUpdatedItem() {
       
    return getNextUpdatedItem();
}


SyncItem* CacheSyncSource::getNextUpdatedItem() {

    SyncItem* syncItem = NULL;
    if (updatedKeys && updatedKeys->hasMoreElement()) {
        StringBuffer* s = (StringBuffer*)updatedKeys->getNextElement();    
        syncItem = fillSyncItem(s);
    }
    
    if (syncItem) {
        StringBuffer key;
        key.convert(syncItem->getKey());
        LOG.debug("[%s] Sending updated item: key = %s", getConfig().getName(), key.c_str());
    }
    else {
        LOG.debug("There are no more updated items to be exchanged. Return NULL");     
    }
    return syncItem;    
}

SyncItem* CacheSyncSource::getFirstDeletedItem() {
       
    return getNextDeletedItem();
}


/**
 * Return the next SyncItem of updated one. It is used in case of fast sync
 * and retrieve the new data source content.
 */
SyncItem* CacheSyncSource::getNextDeletedItem() {
    
    SyncItem* syncItem = NULL;
    if (deletedKeys && deletedKeys->hasMoreElement()) {
        StringBuffer* s = (StringBuffer*)deletedKeys->getNextElement();  
        syncItem = fillSyncItem(s, false);  // Don't set SyncItem data, just the key.
    }
    
    if (syncItem) {
        StringBuffer key;
        key.convert(syncItem->getKey());
        LOG.debug("[%s] Sending deleted item: key = %s", getConfig().getName(), key.c_str());
    }
    else {
        LOG.debug("There are no more deleted items to be exchanged. Return NULL");     
    }
         
    return syncItem;
}

// Check the status of the source before starting the sync.
int CacheSyncSource::beginSync() {
    if(cache == NULL) {
        LOG.error("Cache file not initialized.");
        return -1;      // block the sync if the cache store is not valid.
    }

    return 0;
}

int CacheSyncSource::endSync() {            
    saveCache();    
    return report->getLastErrorCode();
}

/**
* The way to calculate the cache is the follow:
* loop on the current element against an array list
* that has the cache. It is the copy of the original cache.
* When an current element is found in the cache, it is removed
* from the cache copy. At the end the remained element
* in the cache are the deleted ones.
*/
bool CacheSyncSource::fillItemModifications() {
    
    // all the current items keys list
    Enumeration* items = (Enumeration*)getAllItemList();
    bool syncAborted = false;
    
    if (!items) {
        LOG.error("Error in fillItemModification");
        return false;
    }

    // all the action are done on the copy so we can delete
    // the element found. The remained are the deleted by the user.        
    Enumeration& e = cache->getProperties();
    ArrayList cacheCopy;
    while(e.hasMoreElement()) {
        if (checkAbortSync()) {
            return false;
        }
        
        cacheCopy.add(*e.getNextElement());
    }

    StringBuffer* key;
    KeyValuePair* kvp;

    ArrayListEnumeration *newitems = new ArrayListEnumeration(),
                         *moditems = new ArrayListEnumeration(),
                         *delitems = new ArrayListEnumeration();

    if (items) {
        while(items->hasMoreElement()) {
            key = (StringBuffer*)items->getNextElement();
            int size = cacheCopy.size();
            bool foundnew = true;

            if (checkAbortSync()) {
                syncAborted = true;
                break;
            }
            
            for (int i = 0; i < size; i++) {
                if (checkAbortSync()) {
                    syncAborted = true;
                    break;
                }
                
                kvp = (KeyValuePair*)(cacheCopy[i]);
                if (strcmp(kvp->getKey(), key->c_str()) == 0) {
                    foundnew = false;
                    // see if it is updated.
                    StringBuffer sign = getItemSignature(*key);
                    if (sign != kvp->getValue()) {
                        // there is an update. if equal nothing to do...
                        moditems->add(*key);                      
                    }
                    cacheCopy.removeElementAt(i);
                    break;
                }
            }
            if (foundnew) {
                newitems->add(*key);
            }
        }
    }
        
    for(kvp=(KeyValuePair*)cacheCopy.front();kvp;kvp=(KeyValuePair*)cacheCopy.next()){
        if (isReallyDeleted((StringBuffer&)kvp->getKey())) {
            delitems->add((StringBuffer&)kvp->getKey());
        }
        if (checkAbortSync()) {
            syncAborted = true;
            break;
        }
    }
    
    int count = newitems->size() + moditems->size() + delitems->size();
    fireClientTotalNumber(count);
    
    newKeys = newitems;
    updatedKeys = moditems;           
    deletedKeys = delitems;
    
    delete items; 
    return (syncAborted ? false : true);
}

/**
* Save the current status of the cache arrayList
* 
*/
int CacheSyncSource::saveCache() {
    
    LOG.debug("[%s] Saving cache", getConfig().getName());
    
    int ret = cache->close();
    return ret;
}

int CacheSyncSource::updateInCache(KeyValuePair& k, const char* action) {

    if (strcmp(action, ADD    ) == 0 ||
        strcmp(action, REPLACE) == 0) {        
        cache->setPropertyValue(k.getKey(), k.getValue());
    } else if (strcmp(action, DEL) == 0) {
        cache->removeProperty(k.getKey());
    }
    return 0;
}

void CacheSyncSource::getKeyAndSignature(SyncItem& item, KeyValuePair& kvp) {
    
    StringBuffer t;
    t.convert(item.getKey());
    StringBuffer sign = getItemSignature(t);
    
    kvp.setKey(t.c_str());
    kvp.setValue(sign);
}


int CacheSyncSource::updateItemStatus(SyncItem& item)
{
    return updateInCache(item);
}


int CacheSyncSource::addItem(SyncItem& item) {
    
    item.setSyncStatus(insertItem(item));
    return 0;
//	return updateInCache(item);
}


int CacheSyncSource::updateItem(SyncItem& item) {
    
    item.setSyncStatus(modifyItem(item));
	return updateInCache(item);
}

int CacheSyncSource::updateInCache(SyncItem& item) {
	StringBuffer key;
	key.convert(item.getKey());
	
	if (!isErrorCode(item.getSyncStatus())) {
		logSuccess(opName(item.getState()), getConfig().getName(), key.c_str(), item.getSyncStatus());
		if (item.getState() == SYNC_STATE_DELETED) {
			char* t = toMultibyte(item.getKey());
	        KeyValuePair k (t, "");
	        removeFromCache(k);
	        delete [] t;
		} else {
			KeyValuePair k;
	        getKeyAndSignature(item, k);
			if (item.getState() == SYNC_STATE_NEW) {
				insertInCache(k);
			} else {
				updateInCache(k);
			}
		}
	} else {
		logFailure(opName(item.getState()), getConfig().getName(), key.c_str(), item.getSyncStatus());
	}
	return item.getSyncStatus();
}

void CacheSyncSource::logSuccess(const char* operation, const char* name, const char* key, int result) {
	LOG.info("[%s] Successful %s of item with key %s - code %d", getConfig().getName(), operation, key, result);
}

void CacheSyncSource::logFailure(const char* operation, const char* name, const char* key, int result) {
	LOG.error("[%s] Failed %s of item with key %s - code %d", getConfig().getName(), operation, key, result);
}



const char* CacheSyncSource::opName(int syncState) {
	if (syncState == SYNC_STATE_NEW) {
		return "add";
	} else if (syncState == SYNC_STATE_UPDATED) {
		return "update";
	} else {
		return "delete";
	}
}

int CacheSyncSource::deleteItem(SyncItem& item) {
    
    item.setSyncStatus(removeItem(item));
	return updateInCache(item);
} 


StringBuffer CacheSyncSource::readCachePropertyValue(const char* prop)
{
    return cache->readPropertyValue(prop);
}


bool CacheSyncSource::isErrorCode(int code) {
    
    if ((code >= STC_OK && code < STC_MULTIPLE_CHOICES && 
         code != STC_CHUNKED_ITEM_ACCEPTED &&
         code != STC_PARTIAL_CONTENT       && 
         code != STC_RESET_CONTENT         && 
         code != STC_NO_CONTENT            ) ||                 
         code == STC_ALREADY_EXISTS) {
        return false;
    } else {
        return true;
    }
}

void CacheSyncSource::fireClientTotalNumber(int number) {

    fireSyncSourceEvent(getConfig().getURI(), 
                        getConfig().getName(), 
                        getSyncMode(), number, 
                        SYNC_SOURCE_TOTAL_CLIENT_ITEMS);

}

bool CacheSyncSource::isThereLocalModification() {
    
    // all the current items keys list
    Enumeration* items_ori = (Enumeration*)getAllItemList();
    
    if (!items_ori) {
        LOG.error("Error in isThereLocalModification");
        return false;
    }
    
    // all the current items keys list
    ArrayList items; 
    while(items_ori->hasMoreElement()) {
        items.add(*items_ori->getNextElement());
    }
    
    
    // all the action are done on the copy so we can delete
    // the element found. The remained are the deleted by the user.        
    Enumeration& e = cache->getProperties();
    ArrayList cacheCopy;
    while(e.hasMoreElement()) {
        cacheCopy.add(*e.getNextElement());
    }
    
    // 1. Count the number in the addressbook and in the cache. If different 
    // there is new o a delete
    if (items.size() != cacheCopy.size()) {
        return true;
    }
    
    // 2. Check for a modification: at the first found, exit the loop
    bool modificationFound = false;
    
    StringBuffer* key;
    KeyValuePair* kvp;
    
    for (int i = 0; i < items.size(); i++) {
    //if (items.size() > 0) {
    //    while(items.hasMoreElement()) {
        key = (StringBuffer*)items.get(i);
        //key = (StringBuffer*)items.getNextElement();
        int size = cacheCopy.size();
        bool foundnew = true;
        
        for (int i = 0; i < size; i++) {
            kvp = (KeyValuePair*)(cacheCopy[i]);
            if (strcmp(kvp->getKey(), key->c_str()) == 0) {
                foundnew = false;
                // see if it is updated.
                StringBuffer sign = getItemSignature(*key);
                if (sign != kvp->getValue()) {
                    // there is an update. if equal nothing to do...
                    modificationFound = true;
                }
                cacheCopy.removeElementAt(i);
                break;
            }
        }
        if (foundnew || modificationFound == true) {
            modificationFound = true;
            break;
        }
    }
        
    //}
    
    if (modificationFound == true) {
        return true;
    }
    
    for(kvp=(KeyValuePair*)cacheCopy.front();kvp;kvp=(KeyValuePair*)cacheCopy.next()) {
        return true;
    }
    
    return false;
    
}

KeyValueStore* CacheSyncSource::getCache() {
	return cache;
}


END_NAMESPACE
