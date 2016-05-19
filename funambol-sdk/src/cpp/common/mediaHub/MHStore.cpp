/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2011 Funambol, Inc.
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

#include "MediaHub/MHStore.h"
#include "base/Log.h"
#include "base/util/utils.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* MHStore::select_count_stmt_fmt          = "SELECT COUNT(%s) FROM %s";
const char* MHStore::select_entry_generic_stmt_fmt  = "SELECT * from %s WHERE %s = '%q'";
const char* MHStore::select_all_stmt_fmt            = "SELECT * from %s";
const char* MHStore::delete_all_fmt                 = "DELETE FROM %s";

const char* MHStore::asc_order_keyword              = "ASC";
const char* MHStore::desc_order_keyword             = "DESC";
 
MHStore::MHStore(const char* storeName, const char* storePath) : 
                    store_name(storeName), store_path(storePath), 
                    store_status(store_status_not_initialized), error_status(store_no_error)
{
                     
    error_status = store_no_error;
    pthread_mutex_init(&store_access_mutex, NULL);
                        
    cache_items_count = -1;
    
    if (store_path.empty()) {
        error_status = store_invalid_path;
        LOG.error("%s: invalid parameter for cache path", __FUNCTION__);
        return;
    } 
    
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    int status = 0;
    
    if ((status = statFile(store_path.c_str(), &st)) != 0) {
        if (errno == ENOENT) {
            // no database found - we must create table
            store_status = store_status_not_initialized;
        } else {
            LOG.error("%s: can't open store '%s' with database path %s: %s", __FUNCTION__, 
                      store_name.c_str(), store_path.c_str(), strerror(errno));
            error_status = store_invalid_path;
            store_status = store_status_error;
        }
    }
    
    sqlite3_initialize();
    
    status = sqlite3_open_v2(store_path.c_str(), 
                             &db, 
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, 
                             NULL);
    if (status != SQLITE_OK) {
        LOG.error("%s: Cannot open/create db %s",__FUNCTION__, sqlite3_errmsg(db));
    }
    
    const char *pragmaSql = "PRAGMA cache_size = 50";
    if (sqlite3_exec(db, pragmaSql, NULL, NULL, NULL) != SQLITE_OK) {
        LOG.error("%s: Error: failed to execute pragma statement with message '%s'.", __FUNCTION__, sqlite3_errmsg(db));
    }
    
    if (status != SQLITE_OK) {
        store_status = store_status_error;
        error_status = store_open_error;
        LOG.error("%s: error opening sqlite database %s: %s", __FUNCTION__, store_path.c_str(), sqlite3_errmsg(db));
    }
}

MHStore::~MHStore() {
    
    if (store_status == store_status_initialized) {
        sqlite3_close(db);
    }
    
    sqlite3_shutdown();
    pthread_mutex_destroy(&store_access_mutex);
}


MHStoreEntry* MHStore::getEntry(const char* fieldName, const char* fieldValue) const
{
    int res = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    MHStoreEntry* entry = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return NULL;
    }
    
    if (!fieldName || !fieldValue) {
        LOG.error("%s: invalid parameters", __FUNCTION__);
        return NULL;
    }
    
    // Format the statement
    char* select_entry_stmt = sqlite3_mprintf(select_entry_generic_stmt_fmt, store_name.c_str(), fieldName, fieldValue);
    
    pthread_mutex_lock(&store_access_mutex);
    res = sqlite3_prepare_v2(db, select_entry_stmt, -1, &stmt, NULL );
    if (res != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return NULL;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) 
    {        
        entry = readEntry(stmt);
    }
    
    sqlite3_finalize( stmt );
    sqlite3_free(select_entry_stmt);
    pthread_mutex_unlock(&store_access_mutex);
    
    return entry;
}

bool MHStore::AddEntry(MHStoreEntry* entry) {
    uint64_t entryId;
    return AddEntry(entry, entryId);
}

bool MHStore::addEntries(std::vector<MHStoreEntry*>& entries, std::vector<uint64_t>& entriesId) {
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    bool res = true;
    
    std::vector<MHStoreEntry*>::iterator it = entries.begin();
    pthread_mutex_lock(&store_access_mutex);
    cache_items_count = getCount();
    // We execute everything inside a transaction to improve performance
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    for(;it != entries.end();++it) {
        MHStoreEntry* entry = *it;
        StringBuffer sql = formatInsertItemStmt(entry);
        uint64_t entryId;
        if (!sql.empty()) {
            int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
            if (ret == SQLITE_OK) {
                entryId = (uint64_t)sqlite3_last_insert_rowid(db);
                cache_items_count++;
            } else {
                entryId = (uint64_t)-1;
                LOG.error("%s: error executing SQL statement %s : %s", __FUNCTION__, sql.c_str(), sqlite3_errmsg(db));
                res = false;
            }
        } else {
            LOG.error("%s: cannot insert item with an empty SQL statement",__FUNCTION__);
            entryId = (uint64_t)-1;
            res = false;
        }
        entriesId.push_back(entryId);
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    pthread_mutex_unlock(&store_access_mutex);
    return res;
}

bool MHStore::AddEntry(MHStoreEntry* entry, uint64_t& entryId)
{
    int ret = SQLITE_OK;
    
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        
        return false;
    }
    
    StringBuffer sql = formatInsertItemStmt(entry);
    
    if (sql.empty()) {
        LOG.error("%s: error formatting cache insertion statement", __FUNCTION__);
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    
    if (ret != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error executing SQL statement %s : %s", __FUNCTION__, sql.c_str(), sqlite3_errmsg(db));
        
        return false;
    }
    
    entryId = (uint64_t)sqlite3_last_insert_rowid(db);
    
    cache_items_count = getCount();
    cache_items_count++;
    
    pthread_mutex_unlock(&store_access_mutex);
    
    return true;
}

bool MHStore::UpdateEntry(MHStoreEntry* entry)
{
    int ret = SQLITE_OK;
    
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        
        return false;
    }
    
    StringBuffer sql = formatUpdateItemStmt(entry);
    
    if (sql.empty()) {
        LOG.error("%s: error formatting cache update item statement", __FUNCTION__);
        
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    }
    
    return true;
}

bool MHStore::updateEntries(std::vector<MHStoreEntry*>& entries) {
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    bool res = true;
    
    std::vector<MHStoreEntry*>::iterator it = entries.begin();
    pthread_mutex_lock(&store_access_mutex);
    cache_items_count = getCount();
    // We execute everything inside a transaction to improve performance
    sqlite3_exec(db, "BEGIN", 0, 0, 0);
    for(;it != entries.end();++it) {
        MHStoreEntry* entry = *it;
        StringBuffer sql = formatUpdateItemStmt(entry);
        if (!sql.empty()) {
            int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
            if (ret != SQLITE_OK) {
                LOG.error("%s: error executing SQL statement %s : %s", __FUNCTION__, sql.c_str(), sqlite3_errmsg(db));
                res = false;
            }
        } else {
            LOG.error("%s: cannot insert item with an empty SQL statement",__FUNCTION__);
            res = false;
        }
    }
    sqlite3_exec(db, "COMMIT", 0, 0, 0);
    pthread_mutex_unlock(&store_access_mutex);
    return res;
}

// private
long MHStore::get_count()
{
    long ret = -1;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        
        return ret;
    }
    
    // no mutex
    int sqlret = sqlite3_prepare_v2(db, select_count_stmt.c_str(), -1, &stmt, NULL );
    if (sqlret != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        
        return ret;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) {
        ret = (unsigned long)sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    
    // no mutex
    
    return ret;
}

// public
long MHStore::getCount() 
{
    if (cache_items_count == -1) {
        cache_items_count = get_count();
    }
    
    return cache_items_count;
}


bool MHStore::removeAllEntries() {
    
    int ret = SQLITE_OK;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    StringBuffer sql;
    sql.sprintf(delete_all_fmt, store_name.c_str());
    
    if (sql.empty()) {
        LOG.error("%s: error formatting delete-all statement", __FUNCTION__);
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    pthread_mutex_unlock(&store_access_mutex);
    
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        cache_items_count = -1;
        return false;
    }
    
    cache_items_count = 0;
    
    return true;
}

CacheItemsList::CacheItemsList() : retainObjects(false) {
}

CacheItemsList::~CacheItemsList() {
    if (!retainObjects) {
        clear();
    }
}

void CacheItemsList::retain() {
    retainObjects = true;
}

void CacheItemsList::addItem(MHStoreEntry* item) {
    items.push_back(item);
}

std::vector<MHStoreEntry*>& CacheItemsList::getItems() {
    return items;
}

void CacheItemsList::clear() {
    for(unsigned int i=0;i<items.size();++i) {
        MHStoreEntry* entry = items[i];
        delete entry;
    }
    items.clear();
}

CacheLabelsMap::CacheLabelsMap() {
}

CacheLabelsMap::~CacheLabelsMap() {
    clear();
}

void CacheLabelsMap::addItem(MHStoreEntry* item, MHLabelInfo* label) {
    std::map<MHStoreEntry*,std::vector<MHLabelInfo*>*>::iterator it = labels.find(item);
    std::vector<MHLabelInfo*>* itemLabels;
    if (it == labels.end()) {
        // This is the first label for this item, allocate the vector
        itemLabels = new std::vector<MHLabelInfo*>();
        std::pair<MHStoreEntry*,std::vector<MHLabelInfo*>*> value;
        value.first = item;
        value.second = itemLabels;
        labels.insert(value);
    } else {
        itemLabels = it->second;
    }
    itemLabels->push_back(label);
}

std::vector<MHLabelInfo*>* CacheLabelsMap::getLabels(MHStoreEntry* item) {
    std::map<MHStoreEntry*,std::vector<MHLabelInfo*>*>::iterator it = labels.find(item);
    if (it == labels.end()) {
        return NULL;
    } else {
        return it->second;
    }
}

void CacheLabelsMap::clear() {
    std::map<MHStoreEntry*,std::vector<MHLabelInfo*>*>::iterator it = labels.begin();
    while(it != labels.end()) {
        std::vector<MHLabelInfo*>* itemLabels = it->second;
        std::vector<MHLabelInfo*>::iterator itemLabelsIt = itemLabels->begin();
        for(;itemLabelsIt != itemLabels->end();++itemLabelsIt) {
            MHLabelInfo* labelInfo = *itemLabelsIt;
            delete labelInfo;
        }
        delete itemLabels;
        it++;
    }
    labels.clear();
}


END_FUNAMBOL_NAMESPACE
