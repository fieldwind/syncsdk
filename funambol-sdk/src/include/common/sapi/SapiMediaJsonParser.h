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

#ifndef __SAPI_MEDIA_JSON_PARSER_H__
#define __SAPI_MEDIA_JSON_PARSER_H__

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"
#include "sapi/SapiSyncItemInfo.h"
#include "base/util/ArrayList.h"
#include "cJSON.h"

#include "base/util/StringMap.h"

BEGIN_FUNAMBOL_NAMESPACE

class SapiSyncItemInfo;

typedef enum ESapiMediaParserStatus
{
    ESMPNoError = 0,
    ESMPInvalidArgument,
    ESMPParseError,
    ESMPInvalidMessage,
    ESMPKeyMissing,
    ESMPValueMissing
} ESMPStatus;


class SapiMediaJsonParser
{
    private:
        StringBuffer errorCode;
        StringBuffer errorMessage;
 
    public:
        SapiMediaJsonParser();
        ~SapiMediaJsonParser();

        bool parseItemCountObject(const char* jsonObj, int* count, ESMPStatus* errCode);
        bool parseItemsListObject(const char* itemsListJsonObject, const char* sourceName, ArrayList& sapiItemInfoList, 
                                    time_t* responseTime, ESMPStatus* errCode);
        bool parseItemsChangesObject(const char* itemsChangesJsonObject, const char* sourceName, ArrayList& newIDs, 
                                     ArrayList& modIDs, ArrayList& delIDs, time_t* responseTimestamp, ESMPStatus* errCode);
        bool parseQuotaInfoObject(const char* quotaInfoJsonObject, unsigned long long* free, unsigned long long* quota, ESMPStatus* errCode);
        
		
		//bool parseLogin(const char* message, StringBuffer& sessionID, time_t* responseTime, ESMPStatus* errCode);
		bool parseLogin(const char* message, StringBuffer& sessionID, time_t* responseTime, ESMPStatus* errCode, unsigned long * expiretime, StringMap* sourcesStringMap, StringMap* propertyStringMap); // new parseLogin with extra json parsing

        /**
         * Parses the SAPI for server information: GET /sapi/system/information?action=get
         * Actually extracts and returns only the "sapiversion" string value
         * @return true if no error
         */
        bool parseServerInfo(const char* message, StringMap* responseParams, ESMPStatus* errCode);

		bool parseRestoreChargeResponse(const char* message, ESMPStatus* errCode); // parses the response from SAPIRestoreCharge

        bool parseMediaAddItem(const char* itemMetaDataUploadJson, StringBuffer& itemId, time_t* lastUpdate, ESMPStatus* errCode);
    
        bool formatItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, bool prettyPrint=false);
        bool formatMediaItemMetaData(SapiSyncItemInfo* itemInfo, char** itemJsonMetaData, bool prettyPrint=false);
        bool formatDelItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, const char* source, bool prettyPrint=false);
        bool formatRenameItemsListObject(SapiSyncItemInfo* itemInfo, char **itemsListJsonObject, bool prettyPrint=false);

        bool checkErrorMessages(const char* response, time_t* lastUpdate = NULL);
        StringBuffer& getErrorCode()     { return errorCode;    }
        StringBuffer& getErrorMessage()  { return errorMessage; }
    
    private:
		int jsonObjArray2StringMap(cJSON* objJsonArray, StringMap* stringMap);  //converts from an array "name":"value" to string map
        ESMPStatus checkErrorMessage(cJSON* objRoot, bool* error);
        SapiSyncItemInfo* parseSourceItem(cJSON* sourceItem, const char* downloadServerUrl);
        ESMPStatus parseItemsChangeArray(cJSON* itemsArray, ArrayList& idList);
};

END_FUNAMBOL_NAMESPACE

#endif
