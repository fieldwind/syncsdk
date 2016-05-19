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


#include "spds/SyncSourceReport.h"
#include "spds/SyncReport.h"
#include "spds/ItemReport.h"
#include "base/globalsdef.h"

USE_NAMESPACE

const char* const SyncSourceReport::targets[] = {
    CLIENT,
    SERVER,
    NULL
};

const char* const SyncSourceReport::commands[] = {
    COMMAND_ADD,
    COMMAND_REPLACE,
    COMMAND_DELETE,
    HTTP_UPLOAD, 
    HTTP_DOWNLOAD,
    NULL
};



//--------------------------------------------------- Constructor & Destructor
SyncSourceReport::SyncSourceReport(const char* name) {

    initialize();

    if (name) {
        setSourceName(name);
    }
}

SyncSourceReport::SyncSourceReport(SyncSourceReport& ssr) {
    initialize();
    assign(ssr);
}

SyncSourceReport::~SyncSourceReport() {

    clear();
    delete clientAddItems;
    delete clientModItems;
    delete clientDelItems;
    delete clientDownloadedItems;
    
    delete serverAddItems;
    delete serverModItems;
    delete serverDelItems;
    delete serverUploadedItems;
}

void SyncSourceReport::deleteItems(std::map<std::string,ItemReport*>* itemsMap) {
    std::map<std::string,ItemReport*>::iterator it = itemsMap->begin();
    for(; it != itemsMap->end();++it) {
        std::pair<const std::string,ItemReport*>& itemPair = *it;
        delete itemPair.second;
    }
}



//------------------------------------------------------------- Public Methods


void SyncSourceReport::clear()
{
    deleteItems(clientAddItems);
    clientAddItems->clear();
    
    deleteItems(clientModItems);
    clientModItems->clear();
    
    deleteItems(clientDelItems);
    clientDelItems->clear();

    deleteItems(serverAddItems);
    serverAddItems->clear();
    
    deleteItems(serverModItems);
    serverModItems->clear();
    
    deleteItems(serverDelItems);
    serverDelItems->clear();

    deleteItems(clientDownloadedItems);
    clientDownloadedItems->clear();
    
    deleteItems(serverUploadedItems);
    serverUploadedItems->clear();
    
    lastErrorCode  = ERR_NONE;
    lastErrorMsg   = "";
    lastErrorType  = "";
    state          = SOURCE_INACTIVE;
}
 
int SyncSourceReport::getLastErrorCode() const {
    return lastErrorCode;
}
void SyncSourceReport::setLastErrorCode(const int code) {
    lastErrorCode = code;
}

SourceState SyncSourceReport::getState() const {
    return state;
}
void SyncSourceReport::setState(const SourceState s) {
    state = s;
}

const char* SyncSourceReport::getStateString() {
    switch (state) {
        case SOURCE_ACTIVE:   return "OK";
        case SOURCE_INACTIVE: return "Not synchronized";
        case SOURCE_ERROR:    return "Error";
    }
    return "";
}

const char* SyncSourceReport::getLastErrorMsg() const {
    return lastErrorMsg.c_str();
}
void SyncSourceReport::setLastErrorMsg(const char* msg) {
    lastErrorMsg = msg != NULL ? msg : "";
}

const char* SyncSourceReport::getLastErrorType() const {
    return lastErrorType.c_str();
}
void SyncSourceReport::setLastErrorType(const char* type) {
    lastErrorType = type != NULL ? type : NULL;
}

const char* SyncSourceReport::getSourceName() const {
    return sourceName.c_str();
}
void SyncSourceReport::setSourceName(const char* name) {
    sourceName = name;
}


bool SyncSourceReport::checkState() {
    if (state == SOURCE_ACTIVE) {
        return true;
    }
    return false;
}

ItemReport* SyncSourceReport::find(const char* target, const char* command, const WCHAR* ID) {
    
    if (ID == NULL) {
        return NULL;
    }
    const char* id = toMultibyte(ID);
    std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    std::map<std::string,ItemReport*>::iterator itemIt = itemsMap->find(id);
    delete [] id;
    if (itemIt != itemsMap->end()) {
        const std::pair<std::string,ItemReport*>& itemPair = *itemIt;
        return itemPair.second;
    } else {
        return NULL;
    }
}


void SyncSourceReport::addItem(const char* target, const char* command, const WCHAR* ID,
                               const int status, const WCHAR* statusMessage) {

    // Skip status 213: it's received many times in case of large objects.
    if (status == STC_CHUNKED_ITEM_ACCEPTED) {
        return;
    }

    // safe: ID can't be NULL
    if (ID == NULL) {
        return;
    }
    
    const char* id = toMultibyte(ID);

    // Create the ItemReport element
    std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    std::map<std::string,ItemReport*>::iterator itemIt = itemsMap->find(id);
    if (itemIt != itemsMap->end()) {
        // The item already exists
        const std::pair<std::string,ItemReport*>& itemPair = *itemIt;
        ItemReport* existingItem = itemPair.second;
        existingItem->setStatus(status);
    } else {
        // This is a new item
        ItemReport* element = new ItemReport(ID, status, statusMessage);
        std::pair<std::string,ItemReport*> mapValue;
        mapValue.first = id;
        mapValue.second = element;
        itemsMap->insert(mapValue);
    }

    delete [] id;
}

int SyncSourceReport::getItemReportCount(const char* target, const char* command) {
    std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    return itemsMap->size();
}

int SyncSourceReport::getItemReportSuccessfulCount(const char* target, const char* command) {

    int good = 0;
    const std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    std::map<std::string,ItemReport*>::const_iterator itemsMapIt = itemsMap->begin();
    for(;itemsMapIt != itemsMap->end();++itemsMapIt) {
        const std::pair<std::string,ItemReport*>& itemPair = *itemsMapIt;
        ItemReport* e = itemPair.second;
        if (isSuccessful(e->getStatus()) ) {
            good++;
        }
    }
    return good;
}


int SyncSourceReport::getItemReportFailedCount(const char* target, const char* command) {
    std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    int good = getItemReportSuccessfulCount(target, command);
    int res = itemsMap->size() - good;
    return res >= 0 ? res : 0;
}


int SyncSourceReport::getItemReportAlreadyExistCount(const char* target, const char* command) {
    int found = 0;
    std::map<std::string,ItemReport*>* itemsMap = getMap(target, command);
    std::map<std::string,ItemReport*>::iterator itemsMapIt = itemsMap->begin();
    for(;itemsMapIt != itemsMap->end();++itemsMapIt) {
        const std::pair<std::string,ItemReport*>& itemPair = *itemsMapIt;
        ItemReport* e = itemPair.second;
        if (e->getStatus() == ALREADY_EXISTS) {
            found++;
        }
    }
    return found;
}

int SyncSourceReport::getTotalSuccessfulCount() {

    int ret = getItemReportSuccessfulCount(CLIENT, COMMAND_ADD);
    ret    += getItemReportSuccessfulCount(CLIENT, COMMAND_REPLACE);
    ret    += getItemReportSuccessfulCount(CLIENT, COMMAND_DELETE);
    ret    += getItemReportSuccessfulCount(SERVER, COMMAND_ADD);
    ret    += getItemReportSuccessfulCount(SERVER, COMMAND_REPLACE);
    ret    += getItemReportSuccessfulCount(SERVER, COMMAND_DELETE);

    // upload and download lists are intentionally skipped.

    return ret;
}


std::map<std::string,ItemReport*>* SyncSourceReport::getMap(const char* target, const char* command) const {
    
    std::map<std::string,ItemReport*>* ret = NULL;

    if (!strcmp(target, CLIENT)) {
        if (!strcmp(command, COMMAND_ADD)) {
            ret = clientAddItems;
        }
        else if (!strcmp(command, COMMAND_REPLACE)) {
            ret = clientModItems;
        }
        else if (!strcmp(command, COMMAND_DELETE)) {
            ret = clientDelItems;
        }
        else if (!strcmp(command, HTTP_DOWNLOAD)) {
            ret = clientDownloadedItems;
        }
        else {
            // error
        }
    }
    else if (!strcmp(target, SERVER)) {
        if (!strcmp(command, COMMAND_ADD)) {
            ret = serverAddItems;
        }
        else if (!strcmp(command, COMMAND_REPLACE)) {
            ret = serverModItems;
        }
        else if (!strcmp(command, COMMAND_DELETE)) {
            ret = serverDelItems;
        }
        else if (!strcmp(command, HTTP_UPLOAD)) {
            ret = serverUploadedItems;
        }
        else {
            // error
        }
    }
    else {
        // error
    }

    return ret;
}


//------------------------------------------------------------- Private Methods

bool SyncSourceReport::isSuccessful(const int status) {

    // Media sources: successful status = 0
    // TODO FIXME HORRIBLE
    if (!strcmp(sourceName.c_str(), "picture") ||
        !strcmp(sourceName.c_str(), "video") ||
        !strcmp(sourceName.c_str(), "files") ||
        !strcmp(sourceName.c_str(), "music")) {
            if (status == 0) {
                return true;
            } else {
                return false;
            }
    }

    // Note: code 420 = 'device full' is a failure status!
    // (Server refused the item because quota exceeded)
    if (status >= 200 && status < 500 && status != STC_DEVICE_FULL)
        return true;
    else
        return false;
}

void SyncSourceReport::initialize() {
    lastErrorCode  = ERR_NONE;
    lastErrorMsg   = "";
    lastErrorType  = "";
    sourceName     = "";
    state          = SOURCE_INACTIVE;
    
    clientAddItems = new std::map<std::string,ItemReport*>();
    clientModItems = new std::map<std::string,ItemReport*>();
    clientDelItems = new std::map<std::string,ItemReport*>();
    
    serverAddItems = new std::map<std::string,ItemReport*>();
    serverModItems = new std::map<std::string,ItemReport*>();
    serverDelItems = new std::map<std::string,ItemReport*>();
    
    clientDownloadedItems = new std::map<std::string,ItemReport*>();
    serverUploadedItems   = new std::map<std::string,ItemReport*>();
}

void SyncSourceReport::cloneMap(std::map<std::string,ItemReport*>* srcMap,std::map<std::string,ItemReport*>* tgtMap) {
    std::map<std::string,ItemReport*>::iterator itemsMapIt = srcMap->begin();
    for(;itemsMapIt != srcMap->end();++itemsMapIt) {
        std::pair<std::string,ItemReport*> itemPair = *itemsMapIt;
        ItemReport* e = itemPair.second;
        ItemReport* clonedItem = (ItemReport*)e->clone();
        itemPair.second = clonedItem;
        tgtMap->insert(itemPair);
    }
}

void SyncSourceReport::assign(const SyncSourceReport& ssr) {

    setLastErrorCode(ssr.getLastErrorCode());
    setLastErrorMsg (ssr.getLastErrorMsg ());
    setLastErrorType(ssr.getLastErrorType());
    setSourceName   (ssr.getSourceName   ());
    setState        (ssr.getState        ());
    
    cloneMap(ssr.getMap(CLIENT, COMMAND_ADD), clientAddItems);
    cloneMap(ssr.getMap(CLIENT, COMMAND_REPLACE), clientModItems);
    cloneMap(ssr.getMap(CLIENT, COMMAND_DELETE), clientDelItems);
    
    cloneMap(ssr.getMap(SERVER, COMMAND_ADD), serverAddItems);
    cloneMap(ssr.getMap(SERVER, COMMAND_REPLACE), serverModItems);
    cloneMap(ssr.getMap(SERVER, COMMAND_DELETE), serverDelItems);
    
    cloneMap(ssr.getMap(CLIENT, HTTP_DOWNLOAD), clientDownloadedItems);
    cloneMap(ssr.getMap(SERVER, HTTP_UPLOAD), serverUploadedItems);
}

ArrayElement* SyncSourceReport::clone() {
    return new SyncSourceReport(*this);
}
