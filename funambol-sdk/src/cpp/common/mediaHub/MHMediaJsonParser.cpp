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

#include "MediaHub/MHMediaJsonParser.h"
#include "MediaHub/ExternalService.h"
#include "MediaHub/ExportedItem.h"
#include "MediaHub/ExternalServiceAlbum.h"
#include "base/Log.h"
#include "base/util/utils.h"
#include "sapi/SapiPayment.h"
#include <string.h>

BEGIN_FUNAMBOL_NAMESPACE

MHMediaJsonParser::MHMediaJsonParser()
{
    errorCode.reset();
    errorMessage.reset();
}   

MHMediaJsonParser::~MHMediaJsonParser()
{
}

bool MHMediaJsonParser::parseItemCountObject(const char* jsonObj, int* count, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *itemsCount = NULL;
    bool error = false;

    *count = 0;

    if ((jsonObj == NULL) || (strlen(jsonObj) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonObj)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((itemsCount = cJSON_GetObjectItem(data, "count")) == NULL) {
        LOG.error("%s: missing \"count\" field in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    LOG.debug("%s: item count from json object: %d", __FUNCTION__, itemsCount->valueint);

    *count = (int)itemsCount->valueint;

    cJSON_Delete(root);
    
    return true;
}

bool MHMediaJsonParser::parseLastSyncTimestampObject(const char* jsonObj,
                                                     unsigned long *lastSyncTimestamp,
                                                     const char* deviceId,
                                                     const char* sourceUri,
                                                     ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *lastsArray;
    bool error = false;
    
    *lastSyncTimestamp = 0;
    
    if ((jsonObj == NULL) || (strlen(jsonObj) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonObj)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((lastsArray = cJSON_GetObjectItem(data, "lastsynchronizations")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    int arraySize = cJSON_GetArraySize(lastsArray);
    
    if (arraySize) {
        for (int i = 0; i < arraySize; i++) {
            cJSON* lastSyncItem = cJSON_GetArrayItem(lastsArray, i);
            cJSON* thisDeviceIdObj = cJSON_GetObjectItem(lastSyncItem, "deviceid");
            cJSON* thisSourceUriObj = cJSON_GetObjectItem(lastSyncItem, "syncsource");
            const char* thisDeviceId = thisDeviceIdObj->valuestring;
            const char* thisSourceUri = thisSourceUriObj->valuestring;
            
            if (!strcmp(deviceId, thisDeviceId) && !strcmp(sourceUri, thisSourceUri)) {
                // Found it!
                cJSON* endSyncObj = cJSON_GetObjectItem(lastSyncItem, "endsync");
                // Server response are in milliseconds
                *lastSyncTimestamp = (endSyncObj->valueint / 1000);
                break;
            }
        }
    }
            
    LOG.debug("%s: last sync timestamp for device %s and source uri %s: %lld", __FUNCTION__,
              deviceId, sourceUri, *lastSyncTimestamp);
    
    cJSON_Delete(root);
    
    return true;
    
}

bool MHMediaJsonParser::parseQuotaInfoObject(const char* quotaInfoJsonObject, int64_t* free,uint64_t* quota, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *freeNode = NULL;
    cJSON *quotaNode = NULL;
    bool error = false;

    *free = 0;
    *quota = 0;

    if ((quotaInfoJsonObject == NULL) || (strlen(quotaInfoJsonObject) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(quotaInfoJsonObject)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((freeNode = cJSON_GetObjectItem(data, "free")) == NULL) {
        LOG.error("%s: missing \"free\" field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((quotaNode = cJSON_GetObjectItem(data, "quota")) == NULL) {
        LOG.error("%s: missing \"free\" field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    *free  = (long long)freeNode->valueint;
    *quota = (unsigned long long)quotaNode->valueint;
    
    cJSON_Delete(root);

    return true;
}

bool MHMediaJsonParser::parseJsonExportedItemResponse(const char* jsonResponse, ESMPStatus* errCode, time_t* responseTime)
{
    cJSON *root = NULL, *success = NULL;
    cJSON *responsetime = NULL;
    bool error = false;

    *errCode = ESMPNoError;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: json formatted object parameter in empty", __FUNCTION__);
        *errCode = ESMPInvalidArgument;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        
        if (responseTime != NULL) {
            // reponse time from server are in millisecs
            *responseTime = static_cast<time_t>(responsetime->valueint / 1000);
        }
    } else {
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        
        return false;
    } 
 
    if ((success = cJSON_GetObjectItem(root, "success")) == NULL) {
        LOG.error("%s: can't find 'success' node in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    LOG.debug("%s: json message status description: %s", __FUNCTION__,
        success->valuestring);
    
    cJSON_Delete(root);
    
    return true;
}
        

bool MHMediaJsonParser::parseExternalServicesListObject(const char* externalServicesJson,
                                                        CacheItemsList&  externalServicesList,
                                                        ESMPStatus* errCode)
{
   cJSON *root          = NULL,
          *data         = NULL,
          *servicesList = NULL;
          
    bool error = false;
    int arraySize = 0;
    
    if ((externalServicesJson == NULL) || (strlen(externalServicesJson) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
      
        return false;
    }

    if ((root = cJSON_Parse(externalServicesJson)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
      
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
      
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
      
        return false;
    } 

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
      
        return false;
    }

    if ((servicesList = cJSON_GetObjectItem(data, "services")) == NULL) {
        LOG.error("%s: missing \"services\" field from json message", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(servicesList)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);
        
        return true;
    }

   for (int i = 0; i < arraySize; i++) {
        cJSON* service = cJSON_GetArrayItem(servicesList, i);
        
        if (service) {
            ExternalService* externalService = new ExternalService();
            
            if (parseExternalService(service, *externalService) != ESMPNoError) {
                LOG.error("%s: error parsing item changes json array", __FUNCTION__);
                continue;
            }
            
            externalServicesList.addItem(externalService);
        } else {
            LOG.error("%s: error getting external service item in json object array", __FUNCTION__); 
            
            continue;
        }
    }

    cJSON_Delete(root);

    return true;
}
        
bool MHMediaJsonParser::formatItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, bool prettyPrint)
{
    cJSON *root = NULL, *ids = NULL; 
    int itemsCount = 0;
    
    if ((itemsCount = itemsIDs.size()) == 0) {
        return false;
    }
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
   
    if ((ids = cJSON_CreateArray()) == NULL) {
        LOG.error("error creating JSON object");
        cJSON_Delete(root);
        
        return false;
    }
     
    for (int i = 0; i < itemsCount; i++) {
        StringBuffer* itemIdStr = static_cast<StringBuffer* >(itemsIDs.get(i));
        
        if ((itemIdStr) && (itemIdStr->empty() == false)) {
            int itemId = atoi(itemIdStr->c_str());
            cJSON* jsonItemId = cJSON_CreateInt((long)itemId);
            
            cJSON_AddItemToArray(ids, jsonItemId);
        }
    }
    
    cJSON_AddItemToObject(root, "ids", ids);
    
    if (prettyPrint) {
        *itemsListJsonObject = cJSON_Print(root);
    } else {
        *itemsListJsonObject = cJSON_PrintUnformatted(root);
    }
    
    cJSON_Delete(root);
   
    if (*itemsListJsonObject == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatMediaItemMetaData(MHSyncItemInfo* itemInfo, char** itemJsonMetaData, bool metaDataOnlyUpdate, bool prettyPrint)
{
    cJSON *root = NULL, *data = NULL;
    const char* itemGuid = NULL;
    const char* itemName = NULL;
    const char* itemContentType = NULL;
    int64_t itemSize = 0;
    time_t itemCreationDate = 0, itemModificationDate = 0;
    StringBuffer utcDate;
    
    if (itemInfo == NULL) {
        return false;
    }
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
 
     if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        cJSON_Delete(root);
        
        return false;
    }

    itemGuid = itemInfo->getGuid();
    
    if ((itemGuid != NULL) && (strlen(itemGuid) > 0)) {
        LOG.debug("%s: formatting item GUID '%s' into json message",
            __FUNCTION__, itemGuid);
            
        cJSON_AddStringToObject(data, "id", itemGuid);
    }
    
    itemName = itemInfo->getName();
    if ((itemName == NULL) || (strlen(itemName) == 0)) {
        LOG.error("%s: missing name in item info", __FUNCTION__);
        cJSON_Delete(root);
        
        return false;
    }
        
    cJSON_AddStringToObject(data, "name", itemName);
    
    itemContentType = itemInfo->getContentType();
    if (itemContentType) {
        cJSON_AddStringToObject(data, "contenttype", itemContentType);
    }
   
    // if client requests an update only for meta data
    // don't send item size
    if (metaDataOnlyUpdate == false) {
        if ((itemSize = itemInfo->getSize()) == 0) {
            LOG.error("%s: invalid size parameter in item info", __FUNCTION__);
            cJSON_Delete(root);
            
            return false;
        }
        
        cJSON_AddIntToObject(data, "size", itemSize);
    }
    
    itemCreationDate = itemInfo->getCreationDateSecs();
    utcDate = unixTimeToString((unsigned long)itemCreationDate, true);
    if (utcDate.empty()) {
        LOG.error("%s: error convering item creation date in UTC string", __FUNCTION__);
        
        cJSON_Delete(root);
        
        return false;
    }
    cJSON_AddStringToObject(data, "creationdate", utcDate.c_str());
    //LOG.debug("%s: upload: creationdate = %li -> %s", __FUNCTION__, itemCreationDate, utcDate.c_str());
    
    
    itemModificationDate = itemInfo->getModificationDateSecs();
    utcDate = unixTimeToString((unsigned long)itemModificationDate, true);
    if (utcDate.empty()) {
        LOG.error("%s: error converting item modification date in UTC string", __FUNCTION__);
        
        cJSON_Delete(root);
        
        return false;
    }
    cJSON_AddStringToObject(data, "modificationdate", utcDate.c_str());
    //LOG.debug("%s: upload: modificationdate = %li -> %s", __FUNCTION__, itemModificationDate, utcDate.c_str());
    
    
    cJSON_AddItemToObject(root, "data", data);
    
    if (prettyPrint) {
        *itemJsonMetaData = cJSON_Print(root);
    } else {
        *itemJsonMetaData = cJSON_PrintUnformatted(root);
    }
   
    cJSON_Delete(root);
   
    if ((*itemJsonMetaData == NULL) || (strlen(*itemJsonMetaData) == 0)) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
    
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatJsonExportedItem(std::vector<ExportedItem*>& exportedItems,
                                               char** exportedItemJson, bool prettyPrint)
{
    cJSON *root = NULL, 
          *data = NULL,
          *itemIdArray = NULL,
          *itemAttributes = NULL,
          *itemPrivacyVal = NULL,
          *itemNameVal = NULL,
          *itemDescriptionVal = NULL,
          *itemTitleVal = NULL,
          *itemTagsArray = NULL,
          *recipientsArray = NULL,
          *itemNameObj = NULL,
          *itemDescriptionObj = NULL,
          *itemTitleObj = NULL,
          *itemTagsArrayObj = NULL,
          *itemIdObject = NULL;
          
    const char *serviceName = NULL,
               *albumId     = NULL,
               *itemId       = NULL,
               *itemPrivacy = NULL,
               *itemName    = NULL,
               *itemDescription = NULL,
               *itemTitle = NULL;
               
    int itemIdNum = 0, itemTagsNum = 0;
    ArrayList itemTags, recipients;
   
    *exportedItemJson = NULL;
    
    if (exportedItems.size() == 0) {
        LOG.error("%s: invalid exported item argument", __FUNCTION__);
        return ESMPInvalidArgument;
    }
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
 
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        cJSON_Delete(root);
        
        return false;
    }

    serviceName = exportedItems[0]->getServiceName();
    
    if ((serviceName == NULL) || (strlen(serviceName)== 0)) {
        LOG.error("%s: no service name set in exported item", __FUNCTION__);
        cJSON_Delete(root);
            
        return false;
    }
    
    cJSON_AddStringToObject(data, "servicename", serviceName);
    
    albumId = exportedItems[0]->getAlbumId();
    if ((albumId != NULL) || (strlen(albumId) > 0)) {
        cJSON_AddStringToObject(data, "albumid", albumId);
    }
    
    // Create array with the GUIDs
    itemIdArray = cJSON_CreateArray();
    for(unsigned int i=0;i<exportedItems.size();++i) {
        ExportedItem* exportedItem = exportedItems[i];
        itemId = exportedItem->getItemGuid();
        if ((itemId == NULL) || (strlen(itemId) == 0))  {
            LOG.error("%s: missing item id in exported item", __FUNCTION__);
            cJSON_Delete(root);
            
            return false;
        }
    
        itemIdNum = atoi(itemId);
        LOG.debug("%s: exporting item with GUID=%d", __FUNCTION__, itemIdNum);
    
        itemIdObject = cJSON_CreateInt(itemIdNum);
        if (!itemIdArray || !itemIdObject) {
            LOG.error("%s: error creating item id array", __FUNCTION__);
            cJSON_Delete(root);
            
            return false;
        }
        cJSON_AddItemToArray(itemIdArray, itemIdObject);
    }
    
    cJSON_AddItemToObject(data, "items", itemIdArray);
    
    itemPrivacy = exportedItems[0]->getItemPrivacy();
    itemPrivacyVal = cJSON_CreateString(itemPrivacy ? itemPrivacy : "");
    if (itemPrivacyVal == NULL) {
        LOG.error("%s: error creating item name json string object", 
            __FUNCTION__);
        cJSON_Delete(root);
        cJSON_Delete(itemAttributes);
        
        return false;
    }
 
    cJSON_AddItemToObject(data, "itemprivacy", itemPrivacyVal);
    
    // item attributes array
    itemAttributes = cJSON_CreateArray();
    
    // recipients array
    recipients = exportedItems[0]->getRecipients();
    int recipientsNum = recipients.size();
    if (recipientsNum > 0) {
        recipientsArray = cJSON_CreateArray();
        for (int i = 0; i < recipientsNum; i++) {
            StringBuffer* recipient = (StringBuffer*)recipients.get(i);
            cJSON* recipientVal = cJSON_CreateString(recipient ? recipient->c_str() : "");
            cJSON_AddItemToArray(recipientsArray, recipientVal);
            
            cJSON* recipientObj = cJSON_CreateObject();
            recipientVal = cJSON_CreateString(recipient ? recipient->c_str() : ""); 
            cJSON_AddItemToObject(recipientObj, "recipient", recipientVal);        
            cJSON_AddItemToArray(itemAttributes, recipientObj);
        }
        
        cJSON_AddItemToObject(data, "recipients", recipientsArray);    
    }
    
    itemName = exportedItems[0]->getItemName();
    itemNameVal = cJSON_CreateString(itemName ? itemName : "");
    if (itemNameVal == NULL) {
        LOG.error("%s: error creating item name json string object", 
            __FUNCTION__);
        cJSON_Delete(root);
        cJSON_Delete(itemAttributes);
        
        return false;
    }
    
    itemNameObj = cJSON_CreateObject();
    cJSON_AddItemToObject(itemNameObj, "name", itemNameVal);
    cJSON_AddItemToArray(itemAttributes, itemNameObj);
    
    //
    // item title: only if NOT empty
    //
    itemTitle = exportedItems[0]->getItemTitle();
    if (itemTitle && strlen(itemTitle) > 0) {
        itemTitleVal = cJSON_CreateString(itemTitle ? itemTitle : "");
        
        if (itemTitleVal == NULL) {
            LOG.error("%s: error creating item title json string object", 
                __FUNCTION__);
            cJSON_Delete(root);
            cJSON_Delete(itemAttributes);
            
            return false;
        }
        
        itemTitleObj = cJSON_CreateObject();
        
        cJSON_AddItemToObject(itemTitleObj, "title", itemTitleVal);
        cJSON_AddItemToArray(itemAttributes, itemTitleObj);
    }
    
    //
    // item description: only if NOT empty
    //
    itemDescription = exportedItems[0]->getItemDescription();
    if (itemDescription && strlen(itemDescription) > 0 ) {
        itemDescriptionVal = cJSON_CreateString(itemDescription ? itemDescription : "");
        
        if (itemDescriptionVal == NULL) {
            LOG.error("%s: error creating item description json string object", 
                      __FUNCTION__);
            cJSON_Delete(root);
            cJSON_Delete(itemAttributes);
            
            return false;
        }
        
        itemDescriptionObj = cJSON_CreateObject();
        
        const char *descriptionAttributeKey = (strcmp(serviceName, "mms") == 0) ? "message" : "description";
        cJSON_AddItemToObject(itemDescriptionObj, descriptionAttributeKey, itemDescriptionVal);
        cJSON_AddItemToArray(itemAttributes, itemDescriptionObj);
    }
    
    itemTags = exportedItems[0]->getItemTags();
    itemTagsNum = itemTags.size();
    
    itemTagsArray = cJSON_CreateArray();
    for (int i = 0; i < itemTagsNum; i++) {
        StringBuffer* itemTag = (StringBuffer*)itemTags.get(i);
        cJSON* itemTagVal = cJSON_CreateString(itemTag ? itemTag->c_str() : "");
 
        cJSON_AddItemToArray(itemTagsArray, itemTagVal);
    }
   
    itemTagsArrayObj = cJSON_CreateObject();
    
    cJSON_AddItemToObject(itemTagsArrayObj, "tags", itemTagsArray);
    cJSON_AddItemToArray(itemAttributes, itemTagsArrayObj);
    
    cJSON_AddItemToObject(data, "itemattributes", itemAttributes);
    cJSON_AddItemToObject(root, "data", data);
    
    if (prettyPrint) {
        *exportedItemJson = cJSON_Print(root);
    } else {
        *exportedItemJson = cJSON_PrintUnformatted(root);
    }
   
    cJSON_Delete(root);
   
    if ((*exportedItemJson == NULL) || (strlen(*exportedItemJson) == 0)) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    LOG.debug("%s: formatted JSON object for export request: %s", __FUNCTION__,
        *exportedItemJson);
        
    return true;
}

ESMPStatus MHMediaJsonParser::parseCurrentSubscription(cJSON* subscriptionJson, SapiSubscription &subscription)
{
    //"subscription":{
    //    "plan":"standard",
    //    "activated":"20120430T084622",
    //    "nextrenewal":"20120430T093122",
    //    "laststatuschange":"20120430T084622",
    //    "status":"active"
    //}
    
    cJSON *plan = NULL;
    cJSON *status = NULL;
    cJSON *nextRenewal = NULL;
    
    if (subscriptionJson == NULL) {
        LOG.error("%s: invalid JSON object", __FUNCTION__);
        
        return ESMPInvalidMessage;
    }
    
    if ((plan = cJSON_GetObjectItem(subscriptionJson, "plan")) == NULL) {
        LOG.error("%s: missing 'plan' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((nextRenewal = cJSON_GetObjectItem(subscriptionJson, "nextrenewal")) == NULL) {
        LOG.error("%s: missing 'nextRenewal' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((status = cJSON_GetObjectItem(subscriptionJson, "status")) == NULL) {
        LOG.error("%s: missing 'status' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    subscription.setName(plan->valuestring);
    subscription.setStatus(status->valuestring);
    subscription.setDisplayname(plan->valuestring);
    subscription.setNextRenewal(nextRenewal->valuestring);
    
    return ESMPNoError;
}


ESMPStatus MHMediaJsonParser::parseSubscription(cJSON* subscriptionJson, SapiSubscription &subscription)
{
    cJSON *name = NULL;
    cJSON *price;
    cJSON *currency;
    cJSON *quota;
    cJSON *period;
    cJSON *displayname;
    cJSON *description;
    cJSON *activateable;
    cJSON *isDefault;
    cJSON *message;
    
    if (subscriptionJson == NULL) {
        LOG.error("%s: invalid JSON object", __FUNCTION__);
        
        return ESMPInvalidMessage;
    }
    
    if ((name = cJSON_GetObjectItem(subscriptionJson, "name")) == NULL) {
        LOG.error("%s: missing 'name' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    
    if ((price = cJSON_GetObjectItem(subscriptionJson, "price")) == NULL) {
        LOG.error("%s: missing 'price' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((currency = cJSON_GetObjectItem(subscriptionJson, "currency")) == NULL) {
        LOG.error("%s: missing 'currency' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((quota = cJSON_GetObjectItem(subscriptionJson, "quota")) == NULL) {
        LOG.error("%s: missing 'quota' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((period = cJSON_GetObjectItem(subscriptionJson, "period")) == NULL) {
        LOG.error("%s: missing 'period' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((displayname = cJSON_GetObjectItem(subscriptionJson, "displayname")) == NULL) {
        LOG.error("%s: missing 'displayname' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((description = cJSON_GetObjectItem(subscriptionJson, "description")) == NULL) {
        LOG.error("%s: missing 'description' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((activateable = cJSON_GetObjectItem(subscriptionJson, "activateable")) == NULL) {
        LOG.error("%s: missing 'activateable' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((message = cJSON_GetObjectItem(subscriptionJson, "message")) == NULL) {
        LOG.error("%s: missing 'message' field in json object", __FUNCTION__);
    }
    
    if ((isDefault = cJSON_GetObjectItem(subscriptionJson, "default")) == NULL) {
        LOG.error("%s: missing 'default' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    StringBuffer priceStr = StringBuffer().sprintf("%g", price->valuedouble);
    // char priceStr[10];
    // snprintf(priceStr, 32, "%g", price->valuedouble);
    
    subscription.setName(name->valuestring);
    subscription.setPrice(priceStr.c_str());
    subscription.setCurrency(currency->valuestring);
    subscription.setQuota(quota->valuestring);
    subscription.setPeriod(period->valuestring);
    subscription.setCurrency(currency->valuestring);
    subscription.setDisplayname(displayname->valuestring);
    subscription.setDescription(description->valuestring);
    subscription.setActivateable(activateable->valueint);
    subscription.setIsDefault(isDefault->valueint);
    if(message) subscription.setMessage(message->valuestring);
    
    return ESMPNoError;
}

bool MHMediaJsonParser::parseJsonCurrentSubscription(const char* jsonResponse, SapiSubscription& currentSubscription, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *plan = NULL;
    
    bool error = false;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((plan = cJSON_GetObjectItem(data, "subscription")) == NULL) {
        LOG.error("%s: no user field in data JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if (plan) 
    {
        if (parseCurrentSubscription(plan, currentSubscription) != ESMPNoError) 
        {
            LOG.error("%s: error parsing item changes json array", __FUNCTION__);
        }
    } 
    else 
    {
        LOG.error("%s: error getting external service item in json object array", __FUNCTION__); 
    }
    
    cJSON_Delete(root);
    
    return true;
}

bool MHMediaJsonParser::parseJsonSubscriptionList(const char* jsonResponse, std::vector<SapiSubscription*>& subscriptions, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *plans = NULL;
    
    int arraySize = 0;
    bool error = false;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((plans = cJSON_GetObjectItem(data, "plans")) == NULL) {
        LOG.error("%s: no user field in data JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(plans)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);
        
        return true;
    }
    
    for (int i = 0; i < arraySize; i++) 
    {
        cJSON* plan = cJSON_GetArrayItem(plans, i);
        
        if (plan) 
        {
            SapiSubscription *subscription = new SapiSubscription();
            
            if (parseSubscription(plan, *subscription) != ESMPNoError) 
            {
                LOG.error("%s: error parsing item changes json array", __FUNCTION__);
                continue;
            }
            
            subscriptions.push_back(subscription);
        } 
        else 
        {
            LOG.error("%s: error getting external service item in json object array", __FUNCTION__); 
            continue;
        }
    }
    
    cJSON_Delete(root);
    
    return true;
}

ESMPStatus MHMediaJsonParser::parsePayment(cJSON* paymentJson, SapiPayment &payment)
{
    /*
     {
     "id":0,
     "plan":"standard",
     "status":"verified",
     "transactionid":"xxx"
     }
     */
    cJSON *id = NULL;
    cJSON *plan;
    cJSON *status;
    cJSON *transactionid;
    
    if (paymentJson == NULL) {
        LOG.error("%s: invalid JSON object", __FUNCTION__);
        
        return ESMPInvalidMessage;
    }
    
    if ((id = cJSON_GetObjectItem(paymentJson, "id")) == NULL) {
        LOG.error("%s: missing 'name' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    
    if ((plan = cJSON_GetObjectItem(paymentJson, "plan")) == NULL) {
        LOG.error("%s: missing 'price' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((status = cJSON_GetObjectItem(paymentJson, "status")) == NULL) {
        LOG.error("%s: missing 'currency' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    } 
    
    if ((transactionid = cJSON_GetObjectItem(paymentJson, "transactionid")) == NULL) {
        LOG.error("%s: missing 'quota' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }    
    
    payment.setId(id->valueint); 
    payment.setPlan(plan->valuestring);
    payment.setStatus(status->valuestring);
    payment.setTransactionId(transactionid->valuestring);
    
    return ESMPNoError;
}
  
bool MHMediaJsonParser::parseJsonSubscriptionPaymentList(const char* jsonResponse, std::vector<SapiPayment*>& payments, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *paymentArray = NULL;
    
    int arraySize = 0;
    bool error = false;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((paymentArray = cJSON_GetObjectItem(data, "payments")) == NULL) {
        LOG.error("%s: no user field in data JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(paymentArray)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);
        
        return true;
    }
    
    for (int i = 0; i < arraySize; i++) 
    {
        cJSON* paymentJson = cJSON_GetArrayItem(paymentArray, i);
        
        if (paymentJson) 
        {
            SapiPayment *payment = new SapiPayment();
            
            if (parsePayment(paymentJson, *payment) != ESMPNoError) 
            {
                LOG.error("%s: error parsing item changes json array", __FUNCTION__);
                continue;
            }
            
            payments.push_back(payment);
        } 
        else 
        {
            LOG.error("%s: error getting external service item in json object array", __FUNCTION__); 
            continue;
        }
    }
    
    cJSON_Delete(root);
    
    return true;
}

bool MHMediaJsonParser::parseProfilePropertyObject(const char* jsonResponse, const char* propertyKey, char* propertyValue, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *properties = NULL;
    cJSON *property = NULL;
    cJSON *propertyValueJSON = NULL;
    
    bool error = false;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    }
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((properties = cJSON_GetObjectItem(data, "properties")) == NULL) {
        LOG.error("%s: no properties field in data JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    //-- property
    if ((property = cJSON_GetArrayItem(properties, 0)) == NULL) {
        cJSON_Delete(root);
        strcpy(propertyValue, "");
        return true;
    }
    
    if ((propertyValueJSON = cJSON_GetObjectItem(property, "value")) == NULL) {
        cJSON_Delete(root);
        strcpy(propertyValue, "");
        return true;
    }
    
    if (propertyValueJSON->valuestring == NULL) {
        LOG.error("%s: invalid property value in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    // user profile
    strcpy(propertyValue, propertyValueJSON->valuestring);
    
    cJSON_Delete(root);
    return true;
}

bool MHMediaJsonParser::parseJsonUserProfileObject(const char* jsonResponse, SapiUserProfile& userProfile, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *user = NULL;
    cJSON *generic = NULL;
    cJSON *userid = NULL;
    cJSON *firstname = NULL;
    cJSON *lastname = NULL;
    cJSON *useremail = NULL;
    cJSON *phones = NULL;
    cJSON *phone = NULL;
    cJSON *phonenumber = NULL;
    
    bool error = false;
    
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((user = cJSON_GetObjectItem(data, "user")) == NULL) {
        LOG.error("%s: no user field in data JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((generic = cJSON_GetObjectItem(user, "generic")) == NULL) {
        LOG.error("%s: no generic field in user JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    // userid
    if ((userid = cJSON_GetObjectItem(generic, "userid")) == NULL) {
        LOG.error("%s: no firstname field in generic JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if (userid->valuestring == NULL) {
        LOG.error("%s: invalid user id in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    // firstname
    if ((firstname = cJSON_GetObjectItem(generic, "firstname")) == NULL) {
        LOG.error("%s: no firstname field in generic JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if (firstname->valuestring == NULL) {
        LOG.error("%s: invalid firstname in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    
    // lastname
    if ((lastname = cJSON_GetObjectItem(generic, "lastname")) == NULL) {
        LOG.error("%s: no firstname field in generic JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if (lastname->valuestring == NULL) {
        LOG.error("%s: invalid lastname in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    
    // useremail
    if ((useremail = cJSON_GetObjectItem(generic, "useremail")) == NULL) {
        LOG.error("%s: no useremail field in generic JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if (useremail->valuestring == NULL) {
        LOG.error("%s: invalid useremail in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    //-- phones (can't be mandatory)
    bool phoneNotFound = false;
    if ((phones = cJSON_GetObjectItem(user, "phones")) == NULL) {
        LOG.error("%s: no phones array in user JSON object", __FUNCTION__);
        phoneNotFound = true;
    }
    
    if (!phoneNotFound && (phone = cJSON_GetArrayItem(phones, 0)) == NULL) {
        LOG.error("%s: no phone object in phones JSON array", __FUNCTION__);
        phoneNotFound = true;
    }
    
    if (!phoneNotFound && (phonenumber = cJSON_GetObjectItem(phone, "phonenumber")) == NULL) {
        LOG.error("%s: no phonenumber object in phone JSON object", __FUNCTION__);
        phoneNotFound = true;
    }
    
    // user profile
    userProfile.setUserid(userid->valuestring);
    userProfile.setFirstname(firstname->valuestring);
    userProfile.setLastname(lastname->valuestring);
    userProfile.setUseremail(useremail->valuestring);
    userProfile.setPhoneNumber(!phoneNotFound ? phonenumber->valuestring : "");
    
    cJSON_Delete(root);
    return true;
}

bool MHMediaJsonParser::parseGetLabels(const char* message, ESMPStatus* errCode, std::map<MHLabelInfo*,std::vector<uint32_t> >&labelsMap) {
    
    cJSON *root = NULL;
    
    if ((message == NULL) || (strlen(message) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        return false;
    }
    
    if ((root = cJSON_Parse(message)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        return false;
    }
    
    bool error = false;
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        return false;
    }

    cJSON* data = cJSON_GetObjectItem(root, "data");
    if (data == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        return false;
    }
    cJSON* labels = cJSON_GetObjectItem(data, "labels");
    if (labels != NULL) {
        for(int i=0;i<cJSON_GetArraySize(labels);++i) {
            cJSON* label = cJSON_GetArrayItem(labels, i);
            uint32_t labelId = cJSON_GetObjectItem(label, "labelid")->valueint;
            const char* labelName = cJSON_GetObjectItem(label, "name")->valuestring;
            cJSON* items = cJSON_GetObjectItem(label, "items");
            if (items != NULL) {
                MHLabelInfo* labelInfo = new MHLabelInfo();
                labelInfo->setGuid(labelId);
                labelInfo->setName(labelName);
                std::vector<uint32_t> itemsList;
                
                for(int j=0;j<cJSON_GetArraySize(items);++j) {
                    cJSON* item = cJSON_GetArrayItem(items, j);
                    uint32_t itemId = item->valueint;
                    itemsList.push_back(itemId);
                }
                if (itemsList.size() > 0) {
                    std::pair<MHLabelInfo*,std::vector<uint32_t> > value;
                    value.first = labelInfo;
                    value.second = itemsList;
                    labelsMap.insert(value);
                }
            }
        }
    }
    cJSON_Delete(root);
    return true;
}

bool MHMediaJsonParser::parseLogin(const char* message, StringBuffer& sessionID, StringBuffer& validationKey, time_t* responseTime, ESMPStatus* errCode, unsigned long * expiretime, StringMap* sourcesStringMap, StringMap* propertyStringMap, int* loginStatusCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON* authStatusCode = NULL;
    cJSON* authStatusMsg  = NULL;
    cJSON *sessionId = NULL;
    cJSON *validationkey = NULL;
    cJSON *responsetime = NULL;

    bool error = false;
    
    sessionID.reset();
    *loginStatusCode = 0;
    
    if ((message == NULL) || (strlen(message) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(message)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    // get auth_status_code
    if ((authStatusCode = cJSON_GetObjectItem(data, "auth_status_code")) != NULL) {
        unsigned authErrorCode = static_cast<unsigned>(authStatusCode->valueint);
        
        *loginStatusCode = authErrorCode;
        
        cJSON_Delete(root);
        *errCode = ESMPNoError;
    
        return false;
    }

    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        if (responseTime != NULL) {
            // reponse time from server are in millisecs
            *responseTime = static_cast<time_t>(responsetime->valueint / 1000);
        }
    } else {
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    
    // get sessionID
    if ((sessionId = cJSON_GetObjectItem(data, "jsessionid")) == NULL) {
        LOG.error("%s: no session id in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    if (sessionId->valuestring == NULL) {
        LOG.error("%s: invalid session id in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    sessionID = sessionId->valuestring;


    // get validation key
    if ((validationkey = cJSON_GetObjectItem(data, "validationkey")) == NULL) {
        LOG.info("Missing validation key in Login response!");
        // not fatal, continue
        validationKey = "";
    }
    else {
        if (validationkey->valuestring == NULL) {
            LOG.info("Empty validation key in Login response!");
            // not fatal, continue
            validationKey = "";
        }
        validationKey = validationkey->valuestring;
    }

    
	//
    // getting from json sources string map (enabled/disabled)
	//
	if (sourcesStringMap  != NULL && 
	    propertyStringMap != NULL) {
	    
		cJSON *details	      = NULL;
		cJSON *sourcesJson    = NULL;
		cJSON *expiretimeJson = NULL;
		cJSON *properties     = NULL;

		sourcesStringMap->clear();
		propertyStringMap->clear();

		if ((details = cJSON_GetObjectItem(data, "details")) != NULL) {

			// getting  "expiretime"
			if ((expiretimeJson = cJSON_GetObjectItem(details, "expiretime"))) {
				if ( expiretime != NULL ) {
				    *expiretime = (unsigned long)(expiretimeJson->valueint / 1000);
				}
			}

			// getting "properties" array
			if ((properties = cJSON_GetObjectItem(details, "properties"))) {
				jsonObjArray2StringMap(properties, propertyStringMap);
			}

			// getting the "sources"
			if ((sourcesJson = cJSON_GetObjectItem(details, "sources"))) {
				// sources parsing
				jsonObjArray2StringMap(sourcesJson, sourcesStringMap);
			}

		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////

    cJSON_Delete(root);

    return true;
}


bool MHMediaJsonParser::parseRestoreChargeResponse(const char* message, ESMPStatus* errCode)
{
    cJSON *root = NULL;
    cJSON *data = NULL;
   
    bool error = false;
    
    
    if ((message == NULL) || (strlen(message) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(message)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
   
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPParseError;
        
        return false;
    }
  
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 

	/*
    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        
        if (responseTime != NULL) {
            // reponse time from server are in millisecs
            *responseTime = static_cast<time_t>(responsetime->valueint / 1000);
        }
    } else {
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
	*/

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

	/*
    if ((sessionId = cJSON_GetObjectItem(data, "jsessionid")) == NULL) {
        LOG.error("%s: no session id in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    if (sessionId->valuestring == NULL) {
        LOG.error("%s: invalid session id in JSON object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    sessionID = sessionId->valuestring;
    */

	

    cJSON_Delete(root);
	*errCode = ESMPNoError;
    return true;
}

ESMPStatus MHMediaJsonParser::parseExternalService(cJSON* externalServiceJson, ExternalService& externalService)
{
    cJSON *service_name = NULL,
          *display_name = NULL,
          *auth_url     = NULL,
          *authorized   = NULL,
          *icon         = NULL,
          *has_albums   = NULL,
          *album_privacy_values = NULL,
          *item_attributes_values = NULL,
          *item_privacy_values = NULL,
          *account_name = NULL,
          *lastused_album = NULL,
          *source_values = NULL,
          *apikey        = NULL,
          *exportmultiple = NULL,
          *lastuseditemprivacy = NULL;
   
    StringBuffer albumPrivacyValues(""), itemPrivacyValues(""), 
                itemAttributesValues(""), sourceValues("");
    
    if (externalServiceJson == NULL) {
        LOG.error("%s: invalid JSON object", __FUNCTION__);
        
        return ESMPInvalidMessage;
    }
    
    if ((service_name = cJSON_GetObjectItem(externalServiceJson, "servicename")) == NULL) {
        LOG.error("%s: missing 'servicename' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }

    if ((display_name = cJSON_GetObjectItem(externalServiceJson, "displayname")) == NULL) {
        LOG.error("%s: missing 'displayname' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((auth_url = cJSON_GetObjectItem(externalServiceJson, "authurl")) == NULL) {
        LOG.error("%s: missing 'auth_url' field in json object", __FUNCTION__);
        
//        return ESMPKeyMissing;
    }
    
    if ((authorized = cJSON_GetObjectItem(externalServiceJson, "authorized")) == NULL) {
        LOG.error("%s: missing 'authorized' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    if ((icon = cJSON_GetObjectItem(externalServiceJson, "icon")) == NULL) {
        LOG.error("%s: missing 'icon' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
    
    has_albums = cJSON_GetObjectItem(externalServiceJson, "hasalbums");
    
    if ((album_privacy_values = cJSON_GetObjectItem(externalServiceJson, "albumprivacy")) != NULL) {
        if (formatJsonArrayToStringBuffer(album_privacy_values, albumPrivacyValues) != ESMPNoError) {
            LOG.error("%s: error parsing 'albumprivacy' field array in json object", __FUNCTION__);
            
            return ESMPParseError;
        }
    }
    
    if ((item_privacy_values = cJSON_GetObjectItem(externalServiceJson, "itemprivacy")) != NULL) {
        if (formatJsonArrayToStringBuffer(item_privacy_values, itemPrivacyValues) != ESMPNoError) {
            LOG.error("%s: error parsing 'itemattributes' field array in json object", __FUNCTION__);
            
            return ESMPParseError;
        }
    }
    
    if ((item_attributes_values = cJSON_GetObjectItem(externalServiceJson, "itemattributes")) != NULL) {
        if (formatJsonArrayToStringBuffer(item_attributes_values, itemAttributesValues) != ESMPNoError) {
            LOG.error("%s: error parsing 'itemattributes' field array in json object", __FUNCTION__);
            
            return ESMPParseError;
        }
    }
    
    account_name = cJSON_GetObjectItem(externalServiceJson, "accountname");
    lastused_album = cJSON_GetObjectItem(externalServiceJson, "lastusedalbum");
    apikey = cJSON_GetObjectItem(externalServiceJson, "apikey");
    exportmultiple = cJSON_GetObjectItem(externalServiceJson, "exportmultiple");
    lastuseditemprivacy = cJSON_GetObjectItem(externalServiceJson, "lastuseditemprivacy");
    
    if ((source_values = cJSON_GetObjectItem(externalServiceJson, "sources")) == NULL) {
        LOG.error("%s: missing 'sources' field in json object", __FUNCTION__);
        
        return ESMPKeyMissing;
    }
   
    if (formatJsonArrayToStringBuffer(source_values, sourceValues) != ESMPNoError) {
        LOG.error("%s: error parsing 'source' field array in json object", __FUNCTION__);
        
        return ESMPParseError;
    }
 
    const char* serviceName = service_name->valuestring;
    
    if ((serviceName == NULL) || (strlen(serviceName) == 0)) {
        LOG.error("%s: invalid 'servicename' value in json object", __FUNCTION__);
        
        return ESMPInvalidArgument;
    }
    
    externalService.setServiceName(serviceName); 
  
    externalService.setDisplayName(display_name->valuestring);
    externalService.setAuthUrl(auth_url ? auth_url->valuestring : NULL);
    externalService.setIconUrl(icon->valuestring);
    
    if (account_name) { 
        externalService.setAccountName(account_name->valuestring);
    }
    
    externalService.setSources(sourceValues);
    externalService.setItemAttributes(itemAttributesValues);
    externalService.setItemPrivacy(itemPrivacyValues);
    
    if (apikey) {
        externalService.setApiKey(apikey->valuestring);
    }
    
    if (exportmultiple) {
        externalService.setExportMultiple(exportmultiple->valueint);
    }
    
    if (lastuseditemprivacy) {
        externalService.setLastUsedItemPrivacy(lastuseditemprivacy->valuestring);
    }
    
    if (has_albums) {
        externalService.setHasAlbums(has_albums->valueint);
    
        if (albumPrivacyValues.empty() == false) {
            externalService.setAlbumPrivacy(albumPrivacyValues);
        }
        
        if (lastused_album) {
            externalService.setLastUsedAlbum(lastused_album->valuestring);
        }
    }
    
    externalService.setAuthorized(authorized->valueint);
    
    return ESMPNoError;
}

bool MHMediaJsonParser::parseServerInfoJson(const char* jsonResponse, ServerInfo& serverInfo, ESMPStatus* errCode)
{
    cJSON *root = NULL, *portalUrl = NULL, *mobileUrl = NULL,
        *sapiVersion = NULL, *man = NULL, *mod = NULL, *hwv = NULL,
        *fwv = NULL, *oem = NULL, *devId = NULL, *devType = NULL,
        *verDtd = NULL, *utc = NULL, *supportLargeObjs = NULL,
        *supportNumberOfChanges = NULL, *exts = NULL;
  
    if ((jsonResponse == NULL) || (strlen(jsonResponse) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(jsonResponse)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
   
    if ((portalUrl = cJSON_GetObjectItem(root, "portalurl")) != NULL) {
        serverInfo.setPortalUrl(portalUrl->valuestring);
    }

    if ((mobileUrl = cJSON_GetObjectItem(root, "mobileurl")) != NULL) {
        serverInfo.setMobileUrl(mobileUrl->valuestring);
    }

    if ((sapiVersion = cJSON_GetObjectItem(root, "sapiversion")) != NULL) {
        serverInfo.setSapiVersion(sapiVersion->valuestring);
    }

    if ((man = cJSON_GetObjectItem(root, "man")) != NULL) {
        serverInfo.setManifacturer(man->valuestring);
    }

    if ((mod = cJSON_GetObjectItem(root, "mod")) != NULL) {
        serverInfo.setMod(mod->valuestring);
    }

    if ((hwv = cJSON_GetObjectItem(root, "hwv")) != NULL) {
        serverInfo.setHwv(hwv->valuestring);
    }
    
    if ((fwv = cJSON_GetObjectItem(root, "fwv")) != NULL) {
        serverInfo.setFwv(fwv->valuestring);
    }
    
    if ((oem = cJSON_GetObjectItem(root, "oem")) != NULL) {
        serverInfo.setOem(oem->valuestring);
    }
    
    if ((devId = cJSON_GetObjectItem(root, "devid")) != NULL) {
        serverInfo.setDevId(devId->valuestring);
    }
    
    if ((devType = cJSON_GetObjectItem(root, "devtyp")) != NULL) {
        serverInfo.setDevType(devType->valuestring);
    }
  
    if ((verDtd = cJSON_GetObjectItem(root, "verdtd")) != NULL) {
        serverInfo.setVerDtd(verDtd->valuestring);
    }
  
    if ((utc = cJSON_GetObjectItem(root, "utc")) != NULL) {
        serverInfo.setUtc(static_cast<bool>(utc->valueint));
    }
  
    if ((supportLargeObjs = cJSON_GetObjectItem(root, "supportlargeobjs")) != NULL) {
        serverInfo.setSupportLargeObjs(static_cast<bool>(supportLargeObjs->valueint));
    }
    
    if ((supportNumberOfChanges = cJSON_GetObjectItem(root, "supportnumberofchanges")) != NULL) {
        serverInfo.setSupportNumberOfChanges(static_cast<bool>(supportNumberOfChanges->valueint));
    }
    
    if ((exts = cJSON_GetObjectItem(root, "exts")) != NULL) {
        serverInfo.setExts(exts->valuestring);
    }
    
    cJSON_Delete(root);
     
    return true;
}

//
// Parse a json array to extract parameters "name":"value"
// Stores the parsed values into StringMap
//
// returns the number of "name":"value" parsed from json object.
//
int MHMediaJsonParser::jsonObjArray2StringMap(cJSON* objJsonArray, StringMap* stringMap) {
    
	if (!stringMap || !objJsonArray) {
		return 0;
	}
	stringMap->clear();
	int arraySize = cJSON_GetArraySize(objJsonArray);
	if (arraySize) {
		for (int i = 0; i < arraySize; i++) {
			cJSON* sourceItem = cJSON_GetArrayItem(objJsonArray, i);
			if (sourceItem) {
				cJSON *srcName = NULL;
				cJSON *srcValue = NULL;
				
				if ((srcName = cJSON_GetObjectItem(sourceItem, "name")) == NULL) {
					LOG.error("%s: missing \"name\" field in json object", __FUNCTION__);
				}

				if ((srcValue = cJSON_GetObjectItem(sourceItem, "value")) == NULL) {
					LOG.error("%s: missing \"value\" field in json object", __FUNCTION__);
				}
				if ( stringMap ) {
					stringMap->put(srcName->valuestring, srcValue->valuestring);
				}
			} else {
				// LOG.error("%s: error getting source item for json object array", __FUNCTION__); 
			}
		}
	}
	return arraySize;
}

ESMPStatus MHMediaJsonParser::checkErrorMessage(cJSON* root, bool* error_flag)
{
    cJSON *error = NULL,
          *code  = NULL,
          *message  = NULL;
    const char *errorMessageVal = NULL,
               *errorCodeVal    = NULL;
 
    // reset instance variables
    errorCode.reset();
    errorMessage.reset();
    
    *error_flag = false;
    
    if (root == NULL) {
        return ESMPInvalidMessage;
    }
    
    // check if cJSON object is an error message
    if ((error = cJSON_GetObjectItem(root, "error")) == NULL) {
        return ESMPNoError;
    }
   
    if ((code = cJSON_GetObjectItem(error, "code")) == NULL) {
        LOG.error("%s: error parsing JSON message: no \"code\" field", __FUNCTION__);
    
        return ESMPKeyMissing;
    }
    
    if ((message = cJSON_GetObjectItem(error, "message")) == NULL) {
        LOG.error("%s: error parsing JSON message: no \"message\" field", __FUNCTION__);
    
        return ESMPKeyMissing;
    }
    
    if ((errorCodeVal = code->valuestring) != NULL) {
        errorCode = errorCodeVal;
    }
    
    if ((errorMessageVal = message->valuestring) != NULL) {
        errorMessage = errorMessageVal;
    }
   
    *error_flag = true;
 
    return ESMPNoError;
}

ESMPStatus MHMediaJsonParser::formatJsonArrayToStringBuffer(cJSON* array, StringBuffer &valuesString, const char* valueSeparator)
{
    int array_values_num = 0;
    
    if (array == NULL) {
        return ESMPInvalidArgument;
    }
    
    array_values_num = cJSON_GetArraySize(array);
	
    if (array_values_num) {
        bool firstValueParsed = false;
        
        for (int i = 0; i < array_values_num; i++) {
			cJSON* array_item = cJSON_GetArrayItem(array, i);
			
            if (array_item) {
                if (firstValueParsed == false) {
                    valuesString = array_item->valuestring;
                    firstValueParsed = true;
                } else {
                    StringBuffer arrayItemStringValue;
                    arrayItemStringValue.sprintf("%s%s", valueSeparator, array_item->valuestring);
                    valuesString.append(arrayItemStringValue);
                }
            }
        }
    }

    return ESMPNoError;
}

bool MHMediaJsonParser::parseExternalServicesAlbumListObject(const char* externalServicesAlbumJson, CacheItemsList&  albumList,
    const char* serviceName, ESMPStatus* errCode) 
{
    cJSON *root          = NULL,
    *data         = NULL,
    *servicesList = NULL;
    
    bool error = false;
    int arraySize = 0;
    
    if ((externalServicesAlbumJson == NULL) || (strlen(externalServicesAlbumJson) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((serviceName == NULL) || (strlen(serviceName) == 0)) {
        LOG.error("%s: invalid serviceName", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    
    if ((root = cJSON_Parse(externalServicesAlbumJson)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        *errCode = ESMPParseError;
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        *errCode = ESMPNoError;
        
        return false;
    } 
    
    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((servicesList = cJSON_GetObjectItem(data, "albums")) == NULL) {
        LOG.error("%s: missing \"services\" field from json message", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(servicesList)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);
        
        return true;
    }
    
    for (int i = 0; i < arraySize; i++) {
        cJSON* service = cJSON_GetArrayItem(servicesList, i);
        cJSON *album_id      = NULL,
              *album_name    = NULL,
              *album_privacy = NULL;

        if (service) {
            
            
            if ((album_id = cJSON_GetObjectItem(service, "albumid")) == NULL) {
                LOG.error("%s: missing 'servicename' field in json object", __FUNCTION__);
                *errCode = ESMPKeyMissing;
                return false;
            }
            
            if ((album_name = cJSON_GetObjectItem(service, "name")) == NULL) {
                LOG.error("%s: missing 'displayname' field in json object", __FUNCTION__);
                *errCode = ESMPKeyMissing;
                return false;
            }
            
            if ((album_privacy = cJSON_GetObjectItem(service, "privacy")) == NULL) {
                LOG.error("%s: missing 'auth_url' field in json object", __FUNCTION__);
                *errCode = ESMPKeyMissing;
                return false;
            }

            ExternalServiceAlbum *album = new ExternalServiceAlbum(serviceName, album_id->valuestring);
            album->setAlbumName(album_name->valuestring);
            album->setPrivacy(album_privacy->valuestring);
            
            albumList.addItem(album);
        } else {
            LOG.error("%s: error getting external service item in json object array", __FUNCTION__); 
            
            continue;
        }
    }
    
    cJSON_Delete(root);
    
    return true;
}

bool MHMediaJsonParser::parseMediaSetResponse(const char* responseBody, StringBuffer& mediaSetSuccess, 
                                              long* mediaSetId,         StringBuffer& mediaSetUrl,
                                              time_t* responseTime,     ESMPStatus* errCode)
{
    cJSON *root = NULL,
    *mId = NULL,
    *mUrl = NULL,
    *responsetime = NULL,
    *mSuccess = NULL;
    
    bool error = false;
    
    if (!responseBody|| !strlen(responseBody)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        return false;
    }
    
    if ((root = cJSON_Parse(responseBody)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    
    if (error) {
        cJSON_Delete(root);
        return false;
    } 
    
    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        
        // reponse time from server are in millisecs
        *responseTime = static_cast<time_t>(responsetime->valueint / 1000);
        
        // to be removed. try to see if it is a string...
        if (*responseTime == 0) {
            *responseTime = (atoll(responsetime->valuestring) / 1000);
        }
    } else {
        // not an error
        LOG.debug("%s: responsetime parameter missing in json object", __FUNCTION__);
    }
    
    
    if ((mId = cJSON_GetObjectItem(root, "id")) == NULL) {
        LOG.debug("%s: missing 'id' field in json object", __FUNCTION__);
        // not an error (not used by client)
    }
    *mediaSetId = mId->valueint;
    
    if ((mSuccess = cJSON_GetObjectItem(root, "success")) == NULL) {
        LOG.error("%s: missing 'success' field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        return false;
    }
    mediaSetSuccess = mSuccess->valuestring;
    
    if ((mUrl = cJSON_GetObjectItem(root, "url")) == NULL) {
        LOG.error("%s: missing 'url' field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        return false;
    }
    mediaSetUrl = mUrl->valuestring;
    
    cJSON_Delete(root);
    
    return true;
}


bool MHMediaJsonParser::formatStatusReport(SapiStatusReport* statusReport, char** statusReportJson){
    /*
    { 
        "data": { 
            "deviceid": "fac-123456789", 
            "starttime": 1234567890, 
            "endtime": 1334567890, 
            "status": "200", 
            
            "activities": [ 
                           { 
                           "activitytype": "add", 
                           "source": "picture", 
                           "sent": 3 ,
                           "received": 2 
                           },{ 
                           "activitytype": "add", 
                           "source": "video", 
                           "sent": 3 ,
                           "received": 2 
                           },{ 
                           "activitytype": "delete", 
                           "source": "video", 
                           "sent": 3 ,
                           "received": 2 
                           },{ 
                           "activitytype": "update", 
                           "source": "video", 
                           "sent": 3 ,
                           "received": 2 
                           } 
                           ] 
            
        } 
    }
    */
    std::vector<const SapiStatusReportActivity*> activities = statusReport->getActivities();
    int itemsCount = activities.size();
    
    cJSON *root = NULL, *data = NULL, *act = NULL; 

    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object", __FUNCTION__);
        return false;
    }
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'data'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    if ((act = cJSON_CreateArray()) == NULL) {
        LOG.error("%s: error creating JSON object 'items'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    cJSON_AddStringToObject(data, "deviceid", statusReport->getDeviceID());
    cJSON_AddIntToObject(data, "starttime", statusReport->getStartTime());
    cJSON_AddIntToObject(data, "endtime", statusReport->getEndTime());
    cJSON_AddIntToObject(data, "status", statusReport->getStatus());

    
    for (int i = 0; i < itemsCount; i++) {
        cJSON* item;
        
        if ((item = cJSON_CreateObject()) == NULL) {
            LOG.error("%s: error creating JSON object 'data'", __FUNCTION__);
            cJSON_Delete(root);
            return false;
        }
        
        const SapiStatusReportActivity* activity = activities.at(i);
        
        cJSON_AddStringToObject(item, "activitytype", activity->getActivityType());
        cJSON_AddStringToObject(item, "source", activity->getSource());
        cJSON_AddIntToObject(item, "sent", activity->getSent());
        cJSON_AddIntToObject(item, "received", activity->getReceived());
        cJSON_AddItemToArray(act, item);
    }
    
    cJSON_AddItemToObject(data, "activities", act);
    cJSON_AddItemToObject(root, "data", data);
    
    *statusReportJson = cJSON_Print(root);
    
    return true;
}

bool MHMediaJsonParser::formatPushDeviceToken(const char* deviceToken, char** deviceTokenJson) {
    /*
     {
        "data" : 
            {
                "token" : "af8d4430355965da1db893d634268535e97a7ffef95182eef4723e6c5f2d1807"
            }
     }
     */
    cJSON *root = NULL, *data = NULL;
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object", __FUNCTION__);
        return false;
    }
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'data'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    cJSON_AddStringToObject(data, "token", deviceToken);
    cJSON_AddItemToObject(root, "data", data);
    
    *deviceTokenJson = cJSON_Print(root);
    
    return true;
}

bool MHMediaJsonParser::formatMediaSetItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, 
                                                      const char* itemDescription, const char* sourceName, bool prettyPrint)
{
    cJSON *root = NULL, *items = NULL, *data = NULL, *set = NULL; 
    int itemsCount = 0;
    
    if ((itemsCount = itemsIDs.size()) == 0) {
        LOG.error("%s: no item IDs", __FUNCTION__);
        return false;
    }
    if (!sourceName || !strlen(sourceName)) {
        LOG.error("%s: empty source name", __FUNCTION__);
        return false;        
    }
    
    // Example body:
    // -------------
    //{
    //    "data":{
    //        "set":{
    //            "description":"Visit to Vienna",
    //            "type":"picture",
    //            "items":[1,2]
    //        }
    //    }
    //}
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object", __FUNCTION__);
        return false;
    }
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'data'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    if ((items = cJSON_CreateArray()) == NULL) {
        LOG.error("%s: error creating JSON object 'items'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    if ((set = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'set'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    
    //
    // format the "set" object
    //
    cJSON_AddStringToObject(set, "description", itemDescription);
    cJSON_AddStringToObject(set, "type", sourceName);

    for (int i = 0; i < itemsCount; i++) {
        StringBuffer* itemIdStr = static_cast<StringBuffer* >(itemsIDs.get(i));
        
        if ((itemIdStr) && (itemIdStr->empty() == false)) {
            int itemId = atoi(itemIdStr->c_str());
            cJSON* jsonItemId = cJSON_CreateInt((long)itemId);
            
            cJSON_AddItemToArray(items, jsonItemId);
        }
    }
    cJSON_AddItemToObject(set, "items", items);

    
    cJSON_AddItemToObject(data, "set", set);
    
    cJSON_AddItemToObject(root, "data", data);
    
    if (prettyPrint) {
        *itemsListJsonObject = cJSON_Print(root);
    } else {
        *itemsListJsonObject = cJSON_PrintUnformatted(root);
    }
    
    cJSON_Delete(root);
    
    if (*itemsListJsonObject == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatSaveSubscriptionObject(const char * itemsID, char **itemsListJsonObject, bool prettyPrint)
{
    cJSON *root = NULL, *data = NULL, *subscription = NULL;
    
    if (strlen(itemsID) == 0) {
        LOG.error("%s: no item ID", __FUNCTION__);
        return false;
    }
    
    // Example body:
    // -------------
    //{
    //    "data":{
    //        "subscription":{
    //            "plan":"premium"
    //        }
    //    }
    //}
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object", __FUNCTION__);
        return false;
    }
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'data'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    if ((subscription = cJSON_CreateObject()) == NULL) {
        LOG.error("%s: error creating JSON object 'subscription'", __FUNCTION__);
        cJSON_Delete(root);
        return false;
    }
    
    cJSON_AddStringToObject(subscription, "plan", itemsID);
    cJSON_AddItemToObject(data, "subscription", subscription);
    cJSON_AddItemToObject(root, "data", data);
    
    if (prettyPrint) {
        *itemsListJsonObject = cJSON_Print(root);
    } else {
        *itemsListJsonObject = cJSON_PrintUnformatted(root);
    }
    
    cJSON_Delete(root);
    
    if (*itemsListJsonObject == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatProfilePropertyKeyJson(const char *propertyKey, char **propertyKeyJson)
{
    cJSON *root = NULL, *data = NULL, *properties = NULL;
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((properties = cJSON_CreateArray()) == NULL) {
        LOG.error("error creating Array object");
        
        return false;
    }
    
    if (propertyKey)
    {
        cJSON* pKey = cJSON_CreateString(propertyKey);
        cJSON_AddItemToArray(properties, pKey);
    }
    
    cJSON_AddItemToObject(data, "properties", properties);
    cJSON_AddItemToObject(root, "data", data);
    
    *propertyKeyJson = cJSON_PrintUnformatted(root);
    
    cJSON_Delete(root);
    
    if (*propertyKeyJson == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatProfilePropertyJson(const char *propertyKey, const char *propertyValue, char **propertyJson)
{
    cJSON *root = NULL, *data = NULL, *properties = NULL, *prop = NULL;
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((properties = cJSON_CreateArray()) == NULL) {
        LOG.error("error creating Array object");
        
        return false;
    }
    
    if ((prop = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if (propertyKey)
    {
        cJSON* pKey = cJSON_CreateString(propertyKey);
        cJSON_AddItemToObject(prop, "name", pKey);
    }
    
    if (propertyValue)
    {
        cJSON* pValue = cJSON_CreateString(propertyValue);
        cJSON_AddItemToObject(prop, "value", pValue);
    }
    
    
    cJSON_AddItemToArray(properties, prop);
    cJSON_AddItemToObject(data, "properties", properties);
    cJSON_AddItemToObject(root, "data", data);
    
    *propertyJson = cJSON_PrintUnformatted(root);
    
    cJSON_Delete(root);
    
    if (*propertyJson == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatUserProfileJson(SapiUserProfile &userProfile, char **userProfileJson)
{
    cJSON *root = NULL, *data = NULL, *user = NULL, *generic = NULL;
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((user = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((generic = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if (userProfile.getFirstname())
    {
        cJSON* firstname = cJSON_CreateString(userProfile.getFirstname());
        cJSON_AddItemToObject(generic, "firstname", firstname);
    }
    
    if (userProfile.getLastname())
    {
        cJSON* lastname = cJSON_CreateString(userProfile.getLastname());
        cJSON_AddItemToObject(generic, "lastname", lastname);
    }
    
    if (userProfile.getUseremail())
    {
        cJSON* useremail = cJSON_CreateString(userProfile.getUseremail().c_str());
        cJSON_AddItemToObject(generic, "useremail", useremail);
    }
    
    cJSON_AddItemToObject(user, "generic", generic);
    cJSON_AddItemToObject(data, "user", user);
    cJSON_AddItemToObject(root, "data", data);
    
    *userProfileJson = cJSON_PrintUnformatted(root);
    
    cJSON_Delete(root);
    
    if (*userProfileJson == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::formatDelItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, 
                                                   const char* sourceName, bool prettyPrint)
{
    cJSON *root = NULL, *ids = NULL, *data = NULL; 
    int itemsCount = 0;
    
    if ((itemsCount = itemsIDs.size()) == 0) {
        return false;
    }
    
    if ((root = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
        
        return false;
    }
    
    if ((ids = cJSON_CreateArray()) == NULL) {
        LOG.error("error creating JSON object");
        cJSON_Delete(root);
        
        return false;
    }
    
    for (int i = 0; i < itemsCount; i++) {
        StringBuffer* itemIdStr = static_cast<StringBuffer* >(itemsIDs.get(i));
        
        if ((itemIdStr) && (itemIdStr->empty() == false)) {
            int itemId = atoi(itemIdStr->c_str());
            cJSON* jsonItemId = cJSON_CreateInt((long)itemId);
            
            cJSON_AddItemToArray(ids, jsonItemId);
        }
    }
    
    cJSON_AddItemToObject(data, sourceName, ids);
    
    cJSON_AddItemToObject(root, "data", data);
    
    if (prettyPrint) {
        *itemsListJsonObject = cJSON_Print(root);
    } else {
        *itemsListJsonObject = cJSON_PrintUnformatted(root);
    }
    
    cJSON_Delete(root);
    
    if (*itemsListJsonObject == NULL) {
        LOG.error("%s: error formatting JSON object", __FUNCTION__);
        return false;
    }
    
    return true;
}

bool MHMediaJsonParser::checkErrorMessages(const char* response, time_t* lastUpdate)
{
    cJSON *root = NULL;
    cJSON *date = NULL;
    bool error = false;
    
    if (response == NULL) {
        LOG.error("%s: json formatted object parameter in empty", __FUNCTION__);
        
        return false;
    }
    
    if ((root = cJSON_Parse(response)) == NULL) {
        LOG.error("%s: error parsing JSON message", __FUNCTION__);
        
        return false;
    }
    
    if (checkErrorMessage(root, &error) != ESMPNoError) {
        LOG.error("%s: error parsing json object", __FUNCTION__);
        cJSON_Delete(root);
        
        return false;
    }
    
    if (lastUpdate != NULL) {
        // Get the "lastupdate" property = the upload time on the server
        if ((date = cJSON_GetObjectItem(root, "lastupdate")) != NULL) {
            // upload time from server is in millisecs
            *lastUpdate = static_cast<time_t>((date->valueint/1000));
            //*lastUpdate = static_cast<time_t>(date->valueint);
            // to be removed. try to see if it is a string...
            if (*lastUpdate == 0) {
                *lastUpdate = atoll(date->valuestring);
            }
        } 
        else {
            //LOG.error("%s: missing 'lastupdate' field from json message", __FUNCTION__);
            *lastUpdate = 0;        
        }
    }
    
    if (error) {
        cJSON_Delete(root);
        
        return false;
    } 
    
    cJSON_Delete(root);
    
    return true;
}




END_FUNAMBOL_NAMESPACE
