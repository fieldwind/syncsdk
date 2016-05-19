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

#ifndef __MH_STORE_H__
#define __MH_STORE_H__

#include <vector>
#include <map>

#include "base/globalsdef.h"
#include "base/util/ArrayList.h"
#include "spds/constants.h"
#include "base/util/StringBuffer.h"
#include "MediaHub/MHStoreEntry.h"
#include "MediaHub/MHLabelInfo.h"
#include "sqlite3.h"
#include "pthread.h"

BEGIN_FUNAMBOL_NAMESPACE 

class MHStoreEntry;


// store status 
typedef enum store_status_t {
    store_status_not_initialized = 0,
    store_status_initialized,
    store_status_error
} store_status_t;


// generic error codes related to store status
typedef enum store_error_t {
    store_no_error = 0,
    store_invalid_name,
    store_invalid_path,
    store_open_error,
    store_write_error,
    store_read_error
} store_error_t;

typedef enum {
    order_ascending = 0,
    order_descending
} store_query_order_t;

class CacheItemsList {
private:
    std::vector<MHStoreEntry*> items;
    bool retainObjects;
    
public:
    CacheItemsList();
    virtual ~CacheItemsList();
    void addItem(MHStoreEntry*);
    std::vector<MHStoreEntry*>& getItems();
    void clear();
    
    // This method retains the MHStoreEntries upon item destruction. This means that the
    // ownership of these object is transferred to the caller which is responsible for
    // their deallocation
    void retain();
};

class CacheLabelsMap {
private:
    std::map<MHStoreEntry*,std::vector<MHLabelInfo*>*> labels;
    
public:
    CacheLabelsMap();
    virtual ~CacheLabelsMap();
    void addItem(MHStoreEntry* item, MHLabelInfo* label);
    void clear();
    std::vector<MHLabelInfo*>* getLabels(MHStoreEntry* item);
};

/**
 * MH store interface.
 */
class MHStore
{
    
protected:
    
    sqlite3* db;
    
    StringBuffer store_name;
    StringBuffer store_path;
    store_status_t store_status;
    store_error_t  error_status;
    
    mutable pthread_mutex_t store_access_mutex;
    
    
    /// Optimization: cache the entries count
    long cache_items_count;
    
    
    static const char* select_count_stmt_fmt;
    static const char* select_entry_generic_stmt_fmt;
    static const char* select_all_stmt_fmt;
    static const char* delete_all_fmt;
  
    // order by token for select queries
    static const char* asc_order_keyword;
    static const char* desc_order_keyword;

    StringBuffer select_count_stmt;
    StringBuffer insert_row_stmt;
    StringBuffer select_all_stmt;
    
    /// Queries effectively the db to get the number of entries.
    long get_count();
    

    /**
     * Returns new allocated MHStoreEntry from a a sqlite statement object
     * Derived classes must implement this method creating a new MHStoreEntry of
     * the specific type.
     */
    virtual MHStoreEntry* readEntry(sqlite3_stmt *stmt) const = 0;
    
    /**
     * Returns a formatted string with the query to insert a new entry.
     * To be implemented by derived classed.
     */
    virtual StringBuffer formatInsertItemStmt(MHStoreEntry* entry) = 0;
    virtual StringBuffer formatUpdateItemStmt(MHStoreEntry* entry) = 0;
    
public:
    
    explicit MHStore(const char* storeName, const char* storePath);
    
    
    virtual ~MHStore();
    
    
    virtual bool AddEntry(MHStoreEntry* entry);
    virtual bool addEntries(std::vector<MHStoreEntry*>& entries, std::vector<uint64_t>& entriesId);
    virtual bool UpdateEntry(MHStoreEntry* entry);
    virtual bool updateEntries(std::vector<MHStoreEntry*>& entries);
    virtual bool RemoveEntry(MHStoreEntry* entry)  = 0;
    
    virtual bool removeAllEntries();
    
    virtual bool getAllEntries(CacheItemsList& itemInfoList) = 0; 
    
    virtual MHStoreEntry* getEntry(const char* fieldName, const char* fieldValue) const;
    
    /**
     * Returns the number >0 of entries in the database, -1 in case of error.
     * Uses a buffered value (cache_items_count)
     */
    virtual long getCount();
    
protected:
    virtual bool AddEntry(MHStoreEntry* entry, uint64_t& entryId);
    
};


END_FUNAMBOL_NAMESPACE 

#endif
