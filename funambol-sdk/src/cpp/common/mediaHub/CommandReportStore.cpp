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

#include "MediaHub/CommandReportStore.h"
#include "MediaHub/CommandReport.h"
#include "base/Log.h"
#include "string.h"
#include "errno.h"

BEGIN_FUNAMBOL_NAMESPACE

const char* CommandReportStore::commandId_field_name           = "command_id";
const char* CommandReportStore::commandName_field_name         = "command_name";
const char* CommandReportStore::commandStatus_field_name       = "command_status";
const char* CommandReportStore::commandTimestamp_field_name    = "command_timestamp";
const char* CommandReportStore::shortMessage_field_name        = "short_name";
const char* CommandReportStore::longMessage_field_name         = "long_name";

const char* CommandReportStore::create_table_stmt_fmt = "CREATE TABLE %s (id INTEGER PRIMARY KEY, %s TEXT, %s TEXT, "  \
                                                        " %s INTEGER, %s TEXT, %s TEXT, %s TEXT)";

const char* CommandReportStore::insert_row_stmt_fmt   = "INSERT INTO %s (%s, %s, %s, %s, %s, %s)";

const char* CommandReportStore::row_values_fmt        = "VALUES (\"%q\", \"%q\", %d, \"%q\", \"%q\", \"%q\")";

const char* CommandReportStore::select_limited_reports_fmt    = "SELECT * from %s where command_id = \"%s\"";

const char* CommandReportStore::select_all_ordered_stmt_fmt = "SELECT * from %s ORDER BY id %s"; 

const char* CommandReportStore::update_status_stmt_fmt = "UPDATE %s set command_status = %d where command_id = \"%s\"";


CommandReportStore::CommandReportStore(const char* storeName, const char* storePath) : MHStore(storeName, storePath) 
{
    if (store_status == store_status_not_initialized) {
        // db file was just created: init table
        LOG.debug("%s: initializing table for store %s", __FUNCTION__, store_name.c_str());
        if (!initialize_table()) {
            LOG.debug("%s: error initializing table for db: %s", __FUNCTION__, sqlite3_errmsg(db));
        }
        // ignoring error in initialize_table (can't catch the 'error already exist')
        store_status = store_status_initialized;
    }
    
    if (store_status == store_status_initialized) {
        initialize_static_queries();
    }
}

CommandReportStore::~CommandReportStore() {}

bool CommandReportStore::initialize_table()
{
    int ret = SQLITE_OK;
    StringBuffer sql;
  
    sql.sprintf(create_table_stmt_fmt,          store_name.c_str(),             commandId_field_name, 
                commandName_field_name,         commandStatus_field_name,       commandTimestamp_field_name,
                shortMessage_field_name,        longMessage_field_name);
 
    ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    
    return (ret == SQLITE_OK);
}

void CommandReportStore::initialize_static_queries() 
{
    insert_row_stmt.sprintf(insert_row_stmt_fmt,            store_name.c_str(),             commandId_field_name, 
                            commandName_field_name,         commandStatus_field_name,       commandTimestamp_field_name,
                            shortMessage_field_name,        longMessage_field_name);    
    
    select_count_stmt.sprintf(select_count_stmt_fmt, commandId_field_name, store_name.c_str());
    select_all_stmt.sprintf(select_all_stmt_fmt, store_name.c_str());
}


StringBuffer CommandReportStore::formatUpdateItemStmt(MHStoreEntry* entry)
{
    LOG.error("%s: not implemented!", __FUNCTION__);
    return "";
}

bool CommandReportStore::RemoveEntry(MHStoreEntry* entry)
{
    LOG.error("%s: not implemented!", __FUNCTION__);
    return false;
}


bool CommandReportStore::getAllEntries(CacheItemsList& entryList)
{
    int ret = SQLITE_OK;
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    pthread_mutex_lock(&store_access_mutex);
    
    ret = sqlite3_prepare_v2(db, select_all_stmt.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
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

bool CommandReportStore::getAllEntries(CacheItemsList& entryList,
                                       store_query_order_t query_order)
{
    int ret = SQLITE_OK;
    StringBuffer ordered_select_query("");
    sqlite3_stmt *stmt = NULL;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't get entries: cache is not initialized", __FUNCTION__);
        return false;
    }
    
    switch (query_order) {
        case order_ascending:
                ordered_select_query.sprintf(select_all_ordered_stmt_fmt, store_name.c_str(), MHStore::asc_order_keyword);
                break;
        case order_descending:
                ordered_select_query.sprintf(select_all_ordered_stmt_fmt, store_name.c_str(), MHStore::desc_order_keyword);
                break;
        default: {
            LOG.error("%s: unrecognized query order specifier: %d", __FUNCTION__, query_order);
            
            return false;
        }
    }
    
    pthread_mutex_lock(&store_access_mutex);
    
    ret = sqlite3_prepare_v2(db, ordered_select_query.c_str(), -1, &stmt, NULL );
    if (ret  != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error preparing SQL query: %s", __FUNCTION__, sqlite3_errmsg(db));
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


MHStoreEntry* CommandReportStore::readEntry(sqlite3_stmt *stmt) const
{
    if (!stmt) {
        LOG.error("%s: null sql statement", __FUNCTION__);
        return NULL;
    }
    
    const char* commandId          = (const char*)sqlite3_column_text(stmt, 1);
    const char* commandName        = (const char*)sqlite3_column_text(stmt, 2);
    int         commandStatus      = (int)sqlite3_column_int(stmt, 3);
    const char* commandTimestamp   = (const char*)sqlite3_column_text(stmt, 4);
    const char* shortMessage       = (const char*)sqlite3_column_text(stmt, 5);
    const char* longMessage        = (const char*)sqlite3_column_text(stmt, 6);
    
    CommandReport* report = new CommandReport();
    
    report->setCommandId(commandId);
    report->setCommandName(commandName);
    report->setCommandStatus(commandStatus);
    report->setCommandTimestamp(commandTimestamp);
    report->setShortMessage(shortMessage);
    report->setLongMessage(longMessage);
    
    return report;
}


StringBuffer CommandReportStore::formatInsertItemStmt(MHStoreEntry* entry)
{
    StringBuffer stmt("");
    
    if (entry == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    CommandReport* report = (CommandReport*)entry;
    if (report == NULL) {
        LOG.error("%s: invalid argument", __FUNCTION__);
        return stmt;
    }
    
    char* queryBuilt = sqlite3_mprintf(row_values_fmt, 
                   report->getCommandId()         ? report->getCommandId()            : "",
                   report->getCommandName()       ? report->getCommandName()          : "",
                   report->getCommandStatus(),
                   report->getCommandTimestamp()  ? report->getCommandTimestamp()     : "",
                   report->getShortMessage()      ? report->getShortMessage()         : "",
                   report->getLongMessage()       ? report->getLongMessage()          : "");
    
    stmt.sprintf("%s %s", insert_row_stmt.c_str(), queryBuilt); 
    sqlite3_free(queryBuilt);

    
    return stmt;
}

bool CommandReportStore::updateStatus(StringBuffer command_id, int new_status) {
    
    StringBuffer values;
    values.sprintf(update_status_stmt_fmt, store_name.c_str(), new_status, command_id.c_str());
    
    int ret = SQLITE_OK;
    
    if (store_status != store_status_initialized) {
        LOG.error("%s: can't add entry: cache is not initialized", __FUNCTION__);
        
        return false;
    }
    
    pthread_mutex_lock(&store_access_mutex);
    ret = sqlite3_exec(db, values.c_str(), NULL, NULL, NULL);
    
    if (ret != SQLITE_OK) {
        pthread_mutex_unlock(&store_access_mutex);
        LOG.error("%s: error executing SQL statement: %s", __FUNCTION__, sqlite3_errmsg(db));
        
        return false;
    }
        
    pthread_mutex_unlock(&store_access_mutex);
    
    return true;

    
    
}

END_FUNAMBOL_NAMESPACE
