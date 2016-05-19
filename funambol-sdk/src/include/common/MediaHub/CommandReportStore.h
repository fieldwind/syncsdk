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

#ifndef __SQLITE_COMMAND_REPORT_STORE_H__
#define __SQLITE_COMMAND_REPORT_STORE_H__

#include "base/util/StringBuffer.h"
#include "base/globalsdef.h"
#include "base/util/ArrayList.h"
#include "spds/constants.h"
#include "MediaHub/MHStore.h"

BEGIN_FUNAMBOL_NAMESPACE


class CommandReport;


class CommandReportStore : public MHStore
{    
private:
    
    static const char* create_table_stmt_fmt;
    static const char* insert_row_stmt_fmt;
    static const char* row_values_fmt;
    static const char* select_limited_reports_fmt;
    static const char* select_all_ordered_stmt_fmt;
    static const char* update_status_stmt_fmt;
    
    
    bool initialize();
    bool initialize_table();
    void initialize_static_queries();
    
    
    virtual StringBuffer formatInsertItemStmt(MHStoreEntry* entry); 
    virtual StringBuffer formatUpdateItemStmt(MHStoreEntry* entry);
    
    /// Returns new allocated MHStoreEntry (a CommandReport*) from a sqlite statement object
    MHStoreEntry* readEntry(sqlite3_stmt *stmt) const;   // pure virtual
    
public:
    
    // store entry name mappings
    static const char* commandName_field_name;
    static const char* commandId_field_name;
    static const char* commandStatus_field_name;
    static const char* commandTimestamp_field_name;
    static const char* shortMessage_field_name;
    static const char* longMessage_field_name;
    
    
    explicit CommandReportStore(const char* storeName, const char* storePath);
    ~CommandReportStore();
    
    
    bool RemoveEntry(MHStoreEntry* entry);
    

    /**
     * Returns all cache entries in the passed arraylist.
     * No sorting is applied.
     * @param entryList  [IN-OUT] the arraylist returned with all entries
     */
    bool getAllEntries(CacheItemsList& entryList);
    
    /**
     * Returns all cache entries in the passed arraylist applying sorting
     * ordered by id
     * @param entryList  [IN-OUT] the arraylist returned with all entries
     */
    bool getAllEntries(CacheItemsList& entryList, store_query_order_t query_order);
    
    /**
     * Returns the max number of entries identified by the maxNumberOfElement parameter.
     * @param entryList  [IN-OUT] the arraylist returned with entries
     * @param maxNmberOfElement - the max number of element sorted descending (suppose the commandID
     *                            is only incremented. By default is 0 so return all like getAllEntries
     */
    bool getLimitedEntries(ArrayList& entryList, int maxNumberOfElement = 0);   
    
    /// Returns a new allocated MHStoreEntry* given a generic field name & value, NULL if not found.
    MHStoreEntry* getEntry(const char* fieldName, const char* fieldValue) const {
        return MHStore::getEntry(fieldName, fieldValue);
    }
    
    bool updateStatus(StringBuffer command_id, int new_status);
};

END_FUNAMBOL_NAMESPACE

#endif
