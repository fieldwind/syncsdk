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

#include <string>
#include "MediaHub/MHLabelsStore.h"
#include "MediaHub/MHSyncItemInfo.h"
#include "base/Log.h"
#include "string.h"
#include "errno.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* MHLabelsStore::guid_field_name               = "guid";
const char* MHLabelsStore::name_field_name               = "name";

const char* MHLabelsStore::select_all_ordered_stmt_fmt   = "SELECT * from %s ORDER BY %s %s";
const char* MHLabelsStore::select_entry_id_stmt_fmt      = "SELECT * from %s WHERE id = %lu";
const char* MHLabelsStore::delete_row_id_stmt_fmt        = "DELETE FROM %s WHERE id = %lu";
const char* MHLabelsStore::select_count_with_status      = "SELECT COUNT(*) FROM %s WHERE status = %lu";

MHLabelsStore::MHLabelsStore(const char* storeName, const char* storePath,
                             int funambolSavedVersionNumber,
                             int funambolCurrentVersionNumber,
                             bool init) : MHStore(storeName, storePath) 
{
    if (init) {
        if (store_status == store_status_not_initialized) {
            
            initializeStaticQueries();
            
            // db file was just created: init table
            LOG.debug("%s: initializing table for store %s", __FUNCTION__, store_name.c_str());
            if (!initializeTable()) {
                LOG.debug("%s: initializing store %s for db: %s", __FUNCTION__, store_name.c_str(), sqlite3_errmsg(db));
            }
            // ignoring error in initialize_table (can't catch the 'error already exist')
            store_status = store_status_initialized;
            
            if (funambolSavedVersionNumber != funambolCurrentVersionNumber) {
                //upgrade(funambolSavedVersionNumber, funambolCurrentVersionNumber);
            }
        }
    }
}

MHLabelsStore::~MHLabelsStore() {}

bool MHLabelsStore::initializeTable()
{
    int ret = SQLITE_OK;
    ret = sqlite3_exec(db, createQuery.c_str(), NULL, NULL, NULL);
    return (ret == SQLITE_OK);
}

void MHLabelsStore::initializeStaticQueries() 
{
    // Build the create stmt
    createQuery = "CREATE TABLE ";
    createQuery.append(store_name.c_str()).append(" (ID INTEGER PRIMARY KEY AUTOINCREMENT");
    createQuery.append(",");
    createQuery.append(name_field_name).append(" ").append("TEXT");
    createQuery.append(",");
    createQuery.append(guid_field_name).append(" ").append("INTEGER");
    createQuery.append(")");
    
    // Build the insert row stmt
    insertQuery = "INSERT INTO ";
    insertQuery.append(store_name.c_str());
    insertQuery.append(" (");
    // Name field
    insertQuery.append(name_field_name);
    insertQuery.append(",");
    insertQuery.append(guid_field_name);
    insertQuery.append(") VALUES (");
    insertQuery.append("'%q'");
    insertQuery.append(",");
    insertQuery.append("%u");
    insertQuery.append(")");
    
    // Build the update row stmt
    updateQuery = "UPDATE ";
    updateQuery.append(store_name.c_str());
    updateQuery.append(" SET ");
    updateQuery.append(name_field_name).append("=").append("'%q'");
    updateQuery.append(",");
    updateQuery.append(guid_field_name).append("=").append("\"%d\"");
    updateQuery.append(" WHERE ID=%lu");
}

bool MHLabelsStore::AddEntry(MHStoreEntry* entry)
{
    uint64_t entryId;
    bool ret = MHStore::AddEntry(entry, entryId);
    
    //
    // add the item ID
    //
    if (ret) {
        MHLabelInfo* itemInfo = dynamic_cast<MHLabelInfo*>(entry);
        if (itemInfo == NULL) {
            LOG.error("%s: invalid parameter", __FUNCTION__);
            return false;
        }
        
        itemInfo->setLuid(static_cast<uint32_t>(entryId));
    }
    
    return ret;
}

bool MHLabelsStore::UpdateEntry(MHStoreEntry* entry)
{
    int ret = SQLITE_OK;
   
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    MHSyncItemInfo* itemInfo = (MHSyncItemInfo*)entry;
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }

    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        
        return false;
    }

    StringBuffer sql = formatUpdateItemStmt(itemInfo);
    
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

bool MHLabelsStore::RemoveEntry(MHStoreEntry* entry)
{
    if (entry == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }
    
    MHLabelInfo* itemInfo = dynamic_cast<MHLabelInfo*>(entry);
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return false;
    }

    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    
    StringBuffer sql;
    sql.sprintf(delete_row_id_stmt_fmt, store_name.c_str(), itemInfo->getLuid());
    
    if (sql.empty()) {
        LOG.error("%s: error formatting delete-entry statement", __FUNCTION__);
        return false;
    }
    
    //LOG.debug("%s: > SQL exec: '%s'", __FUNCTION__, sql.c_str());
    pthread_mutex_lock(&store_access_mutex);
    int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    int changes = sqlite3_changes(db);
    pthread_mutex_unlock(&store_access_mutex);
    
    if (ret != SQLITE_OK) {
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        return false;
    } else if (changes == 0) {
        LOG.info("%s: cannot remove item %u", __FUNCTION__, itemInfo->getLuid());
        return false;
    }
    else {
        cache_items_count = getCount();
        cache_items_count--;
        return true;
    }
}


bool MHLabelsStore::getAllEntries(CacheItemsList& entryList)
{
    return getAllEntries(entryList, name_field_name, true);
}

bool MHLabelsStore::getAllEntries(CacheItemsList& entryList, const char* orderedBy,
                                 bool descending)
{
    //LOG.debug("%s: entering", __FUNCTION__);
    
    int ret = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    if (!orderedBy) {
        LOG.error("%s: empty 'ordered by' parameter", __FUNCTION__);
        return false;
    }

    // format the SQL query
    select_all_stmt.sprintf(select_all_ordered_stmt_fmt, store_name.c_str(), orderedBy, descending? "DESC":"ASC");
  
    pthread_mutex_lock(&store_access_mutex);
    
    ret = sqlite3_prepare_v2(db, select_all_stmt.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__,
            sqlite3_errmsg(db));
            
        return false;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW))
    {        
        MHStoreEntry* entry = readEntry(stmt);
        if (entry) {
            entryList.addItem(entry);
        }
    }

    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return true;
}


MHStoreEntry* MHLabelsStore::getEntry(const unsigned long itemID) const
{
    int res = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    MHStoreEntry* entry = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return NULL;
    }
    
    StringBuffer select_entry_stmt;
    select_entry_stmt.sprintf(select_entry_id_stmt_fmt, store_name.c_str(), itemID);
   
    pthread_mutex_lock(&store_access_mutex);
    res = sqlite3_prepare_v2(db, select_entry_stmt.c_str(), -1, &stmt, NULL );
    if (res  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
        return NULL;
    }
    
    while ((sqlite3_step(stmt) == SQLITE_ROW)) 
    {
        entry = readEntry(stmt);
    }
    
    sqlite3_finalize( stmt );
    pthread_mutex_unlock(&store_access_mutex);
    
    return entry;
}


MHStoreEntry* MHLabelsStore::readEntry(sqlite3_stmt *stmt) const
{
    if (!stmt) {
        LOG.error("%s: null sql statement", __FUNCTION__);
        return NULL;
    }
    
    sqlite3_value* oid = sqlite3_column_value(stmt, 0);
    unsigned long luid = static_cast<unsigned long>(sqlite3_value_int64(oid));
    const char* name = (const char*)sqlite3_column_text(stmt, 1);
    uint32_t guid = (uint32_t)sqlite3_column_int(stmt, 2);
    
    MHLabelInfo* itemInfo = new MHLabelInfo(guid, luid, name);
    return itemInfo;
}


StringBuffer MHLabelsStore::formatInsertItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
  
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    MHLabelInfo* itemInfo = dynamic_cast<MHLabelInfo*>(entry);
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return stmt;
    }
    
    const char* n = itemInfo->getName().c_str();
    StringBuffer name = n != NULL ? n : "";
    
    char* queryBuilt = sqlite3_mprintf(insertQuery.c_str(), name.c_str(), itemInfo->getGuid());
    stmt.sprintf("%s", queryBuilt); 
    sqlite3_free(queryBuilt);
    
    return stmt;
}

StringBuffer MHLabelsStore::formatUpdateItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    MHLabelInfo* itemInfo = dynamic_cast<MHLabelInfo*>(entry);
    if (itemInfo == NULL) {
        LOG.error("%s: invalid parameter", __FUNCTION__);
        return stmt;
    }
    
    unsigned long itemId = itemInfo->getLuid();
    const char* n = itemInfo->getName().c_str();
    StringBuffer name = n != NULL ? n : "";
    
    uint32_t guid = itemInfo->getGuid();
    char* queryBuilt = sqlite3_mprintf(updateQuery.c_str(), name.c_str(), guid, itemId);
    stmt.sprintf("%s", queryBuilt); 
    sqlite3_free(queryBuilt);

    return stmt;
}

END_FUNAMBOL_NAMESPACE
