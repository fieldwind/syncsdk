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

#ifndef __MH_LABELS_STORE_H__
#define __MH_LABELS_STORE_H__

#include <algorithm>
#include <string>

#include "base/util/StringBuffer.h"
#include "MediaHub/MHStore.h"
#include "MediaHub/MHLabelInfo.h"
#include "MediaHub/MHStoreEntry.h"

BEGIN_FUNAMBOL_NAMESPACE

class MHLabelInfo;

class MHLabelsStore : public MHStore
{
public:
    static const char* guid_field_name;
    static const char* name_field_name;
    
protected:
    bool addLocalItemPathOnUpgrade();
    bool migrateItemStatus();

protected: 
    
    static const char* select_all_ordered_stmt_fmt;
    static const char* select_entry_id_stmt_fmt;
    static const char* delete_row_id_stmt_fmt;
    static const char* select_count_with_status;
    
    std::string insertQuery;
    std::string createQuery;
    std::string updateQuery;
    
    virtual bool initializeTable();
    virtual void initializeStaticQueries(); 
    
    virtual StringBuffer formatInsertItemStmt(MHStoreEntry* entry);
    virtual StringBuffer formatUpdateItemStmt(MHStoreEntry* itemInfo);

    
    /// Returns new allocated MHStoreEntry (a MHSyncItemInfo*) from a a sqlite statement object
    virtual MHStoreEntry* readEntry(sqlite3_stmt *stmt) const;
    
public:
    
    explicit MHLabelsStore(const char* cacheName, const char* cachePath,
                           int funambolSavedVersionNumber,
                           int funambolCurrentVersionNumber,
                           bool init=true);
    virtual ~MHLabelsStore();
    
    
    bool AddEntry(MHStoreEntry* entry);
    bool UpdateEntry(MHStoreEntry* entry);
    bool RemoveEntry(MHStoreEntry* entry);
    
    /**
     * Returns all cache entries in the given list, ordered by a given parameter.
     * The entryList contains pointers to MHItemStoreEntry which are
     * automatically deallocated once the entryList gets deallocated.
     *
     * @param MHItemInfoList  [IN-OUT] the list returned with all entries
     * @param orderedBy         the field name to order the returned array
     * @param descending        if true, the items are sorted descending
     */
    bool getAllEntries(CacheItemsList& entryList, const char* orderedBy, bool descending);
    
    /**
     * Returns all cache entries in the given list.
     * The entryList contains pointers to MHItemStoreEntry which are
     * automatically deallocated once the entryList gets deallocated.
     *
     * @param MHItemInfoList  [IN-OUT] the list returned with all entries
     */
    bool getAllEntries(CacheItemsList& entryList);
    
    /// Returns a new allocated MHStoreEntry* (a MHSyncItemInfo*) given its ID, NULL if not found.
    MHStoreEntry* getEntry(const unsigned long itemID) const;
    
    /// Returns a new allocated MHStoreEntry* (a MHSyncItemInfo*) given a custom field, NULL if not found.
    virtual MHStoreEntry* getEntry(const char* fieldName, const char* fieldValue) const {
        return MHStore::getEntry(fieldName, fieldValue);
    }
};

END_FUNAMBOL_NAMESPACE

#endif
