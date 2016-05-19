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

#ifndef __MH_ITEM_JSON_PARSER_H__
#define __MH_ITEM_JSON_PARSER_H__

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"
#include "MediaHub/MHJsonParser.h"
#include "MediaHub/MHSyncItemInfo.h"
#include "MediaHub/MHStore.h"
#include "base/util/ArrayList.h"
#include "cJSON.h"
#include "sapi/SapiUserProfile.h"
#include "base/util/StringMap.h"


BEGIN_FUNAMBOL_NAMESPACE

class MHSyncItemInfo;
class ExternalService;
class ExportedItem;

class MHItemJsonParser {
    
protected:
    StringBuffer errorCode;
    StringBuffer errorMessage;

public:

    MHItemJsonParser();
    virtual ~MHItemJsonParser();

    virtual bool parseItemsListObject(const char* itemsListJsonObject, const char* sourceName,
                                      CacheItemsList& MHItemInfoList, CacheLabelsMap& labelsMap,
                                      time_t* responseTime, ESMPStatus* errCode);
    virtual bool parseItemsChangesObject(const char* itemsChangesJsonObject, const char* sourceName, ArrayList& newIDs, 
                                         ArrayList& modIDs, ArrayList& delIDs, time_t* responseTimestamp, ESMPStatus*
                                         errCode);
            
    virtual bool parseItemsChangesObjectForPendingChanges(const char*itemsChangesJsonObject, 
                                                          ArrayList& sourcesToCheck,
                                                          ArrayList& sourcesWithPendingChanges, 
                                                          time_t* responseTimestamp,
                                                          ESMPStatus *errCode,
                                                          bool includingDeletes);
    
    bool parseMediaAddItem(const char* itemMetaDataUploadJson, const char* sourceName, StringBuffer& itemId,
                           time_t* lastUpdate,
                           ESMPStatus* errCode, MHSyncItemInfo* responseItem);
    
    StringBuffer& getErrorCode();
    StringBuffer& getErrorMessage();
    
protected:
    virtual ESMPStatus checkErrorMessage(cJSON* root, bool* error_flag);
    virtual ESMPStatus parseItemsChangeArray(cJSON* itemsArray, ArrayList& idList);
    
    virtual MHSyncItemInfo* createSyncItemInfo();
    virtual bool parseSourceItem(cJSON* sourceItem, CacheLabelsMap& labels, 
                                 const char* downloadServerUrl, MHSyncItemInfo* itemInfo);

};

END_NAMESPACE

#endif