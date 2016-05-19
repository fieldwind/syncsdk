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

#include "MediaHub/MHItemJsonParser.h"
#include "MediaHub/MHLabelInfo.h"
#include "MediaHub/ExternalService.h"
#include "MediaHub/ExportedItem.h"
#include "MediaHub/ExternalServiceAlbum.h"
#include "base/Log.h"
#include "base/util/utils.h"

BEGIN_FUNAMBOL_NAMESPACE

MHItemJsonParser::MHItemJsonParser()
{
    errorCode.reset();
    errorMessage.reset();
}   

MHItemJsonParser::~MHItemJsonParser()
{
}


bool MHItemJsonParser::parseItemsListObject(const char* itemsListJsonObject,
                                             const char* sourceName, 
                                             CacheItemsList& itemsInfoList,
                                             CacheLabelsMap& labelsMap,
                                             time_t* responseTime, ESMPStatus* errCode)
{
    cJSON *root = NULL,
          *data = NULL,
          *itemsArray = NULL,
          *responsetime = NULL,
          *serverUrl = NULL;
          
    int arraySize = 0;
    bool error = false;
    char* downloadServerUrl = NULL;
    
    if ((itemsListJsonObject == NULL) || (strlen(itemsListJsonObject) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
      
        return false;
    }
    
    if ((sourceName == NULL) || (strlen(sourceName) == 0)) {
        LOG.error("%s: missing source name parameter", __FUNCTION__);
        
        return false;
    }
    
    if ((root = cJSON_Parse(itemsListJsonObject)) == NULL) {
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
        LOG.error("%s: responsetime parameter missing in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        return false;
    }

    if ((data = cJSON_GetObjectItem(root, "data")) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        return false;
    }

    if ((serverUrl = cJSON_GetObjectItem(data, "mediaserverurl")) != NULL) {
        downloadServerUrl = serverUrl->valuestring;
    }
   
    if ((itemsArray = cJSON_GetObjectItem(data, sourceName)) == NULL) {
        LOG.error("%s: missing data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    arraySize = cJSON_GetArraySize(itemsArray);
    
    if (arraySize) {
        for (int i = 0; i < arraySize; i++) {
            cJSON* sourceItem = cJSON_GetArrayItem(itemsArray, i);
            
            if (sourceItem) {
                MHSyncItemInfo* sourceItemInfo = createSyncItemInfo();
                bool parsingOk = parseSourceItem(sourceItem, labelsMap, downloadServerUrl, sourceItemInfo);
                if (parsingOk) {
                    itemsInfoList.addItem(sourceItemInfo);
                } else {
                    LOG.error("%s: error getting source item info from json object");
                }
            } else {
                LOG.error("%s: error getting source item for json object array", __FUNCTION__); 
            }
        }
    }
    
    cJSON_Delete(root);
    
    return true;
}

MHSyncItemInfo* MHItemJsonParser::createSyncItemInfo() {
    return new MHSyncItemInfo();
}

bool MHItemJsonParser::parseItemsChangesObjectForPendingChanges(
                             const char*itemsChangesJsonObject, 
                             ArrayList& sourcesToCheck,
                             ArrayList& sourcesWithPendingChanges, 
                             time_t* responseTimestamp,
                             ESMPStatus *errCode,
                             bool includingDeletes)
{
    cJSON *root   = NULL,
    *data         = NULL,
    *responsetime = NULL,
    *itemsList    = NULL;
    
    bool error = false;
    const char *newItemsKey     = "N", // see SAPI developer guide @section 3.3.33
               *updatedItemsKey = "U",
               *deletedItemsKey = "D";
    int arraySize = 0;
    
    if ((itemsChangesJsonObject == NULL) || (strlen(itemsChangesJsonObject) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
        
        return false;
    }
    
    if ((root = cJSON_Parse(itemsChangesJsonObject)) == NULL) {
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
    
    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        
        // reponse time from server are in millisecs
        *responseTimestamp = static_cast<time_t>(responsetime->valueint / 1000);
        // to be removed. try to see if it is a string...
        if (*responseTimestamp == 0) {
            *responseTimestamp = (atoll(responsetime->valuestring) / 1000);
        }
    } else {
        LOG.error("%s: missing responsetime field from json message", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(data)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);
        
        return true;
    }
    
    for(int i=0;i < sourcesToCheck.size();++i) {
        StringBuffer* sourceUri = (StringBuffer*)sourcesToCheck.get(i);
        if ((itemsList = cJSON_GetObjectItem(data, sourceUri->c_str())) != NULL) {
            if (cJSON_GetObjectItem(itemsList, newItemsKey) != NULL) {
                LOG.debug("%s: there are NEW remote items for source %s", __FUNCTION__, sourceUri->c_str());
                sourcesWithPendingChanges.add(*sourceUri);
            } 
            else if (cJSON_GetObjectItem(itemsList, updatedItemsKey) != NULL) {
                LOG.debug("%s: there are UPDATED remote items for source %s", __FUNCTION__, sourceUri->c_str());
                sourcesWithPendingChanges.add(*sourceUri);
            } 
            else if (cJSON_GetObjectItem(itemsList, deletedItemsKey) != NULL) {
                if (includingDeletes) {
                    LOG.debug("%s: there are DELETED remote items for source %s", __FUNCTION__, sourceUri->c_str());
                    sourcesWithPendingChanges.add(*sourceUri);
                }
            }
        }
    }
    
    cJSON_Delete(root);
    
    return true;
}

bool MHItemJsonParser::parseItemsChangesObject(const char* itemsChangesJsonObject, const char* sourceName, 
        ArrayList& newIDs, ArrayList& modIDs, ArrayList& delIDs, time_t* responseTimestamp, ESMPStatus* errCode)
{
    cJSON *root = NULL,
          *data = NULL,
          *responsetime = NULL,
          *itemsList         = NULL,
          *newItemsArray     = NULL,
          *updatedItemsArray = NULL,
          *deletedItemsArray = NULL;
          
    bool error = false;
    const char *newItemsKey     = "N", // see MH developer guide @section 2.5.42
               *updatedItemsKey = "U",
               *deletedItemsKey = "D";
    int arraySize = 0;
    
    if ((itemsChangesJsonObject == NULL) || (strlen(itemsChangesJsonObject) == 0)) {
        LOG.error("%s: invalid JSON message", __FUNCTION__);
        *errCode = ESMPInvalidMessage;
      
        return false;
    }

     if ((sourceName == NULL) || (strlen(sourceName) == 0)) {
        LOG.error("%s: missing source name parameter", __FUNCTION__);
        *errCode = ESMPInvalidArgument;
      
        return false;
    }
    
    if ((root = cJSON_Parse(itemsChangesJsonObject)) == NULL) {
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

    if ((responsetime = cJSON_GetObjectItem(root, "responsetime")) != NULL) {
        
        // reponse time from server are in millisecs
        *responseTimestamp = static_cast<time_t>(responsetime->valueint / 1000);
        // to be removed. try to see if it is a string...
        if (*responseTimestamp == 0) {
            *responseTimestamp = (atoll(responsetime->valuestring) / 1000);
        }
    } else {
        LOG.error("%s: missing responsetime field from json message", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }
    
    if ((arraySize = cJSON_GetArraySize(data)) == 0) {
        LOG.debug("%s: json object has no items set in array", __FUNCTION__);        
        cJSON_Delete(root);

        return true;
    }

    if ((itemsList = cJSON_GetObjectItem(data, sourceName)) == NULL) {
        LOG.error("%s: missing source data field in json object", __FUNCTION__);
        cJSON_Delete(root);
        *errCode = ESMPKeyMissing;
        
        return false;
    }

    if ((newItemsArray = cJSON_GetObjectItem(itemsList, newItemsKey)) != NULL) {
        if (parseItemsChangeArray(newItemsArray, newIDs) != ESMPNoError) {
            LOG.error("%s: error parsing item changes json array", __FUNCTION__);
            cJSON_Delete(root);

            return false;
        }
    }
    
    if ((updatedItemsArray = cJSON_GetObjectItem(itemsList, updatedItemsKey)) != NULL) {
        if (parseItemsChangeArray(updatedItemsArray, modIDs) != ESMPNoError) {
            LOG.error("%s: error parsing item changes json array", __FUNCTION__);
            cJSON_Delete(root);

            return false;
        }
    }
    
    if ((deletedItemsArray = cJSON_GetObjectItem(itemsList, deletedItemsKey)) != NULL) {
        if (parseItemsChangeArray(deletedItemsArray, delIDs) != ESMPNoError) {
            LOG.error("%s: error parsing item changes json array", __FUNCTION__);
            cJSON_Delete(root);

            return false;
        }
    }
    
    cJSON_Delete(root);

    return true;
}
    

ESMPStatus MHItemJsonParser::checkErrorMessage(cJSON* root, bool* error_flag)
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


bool MHItemJsonParser::parseSourceItem(cJSON* sourceItem, CacheLabelsMap& labels, 
                                       const char* downloadServerUrl, MHSyncItemInfo* itemInfo)
{
    cJSON *guid = NULL,
    *url = NULL,
    *modification_date = NULL,
    *creation_date = NULL,
    *size = NULL,
    *name = NULL,
    *exif_data = NULL,
    *exif_maker = NULL,
    *exif_model = NULL,
    *exif_image_length = NULL,
    *exif_image_width = NULL,
    *exif_creation_date = NULL,
    *video_metadata = NULL,
    *video_codec    = NULL,
    *video_duration = NULL,
    *video_bitrate  = NULL,
    *video_height   = NULL,
    *video_width    = NULL,
    *exported_services = NULL,
    *thumbnails_array = NULL,
    *thumbnail_data = NULL,
    *thumbnail_url  = NULL,
    *preview_data   = NULL,
    *preview_url    = NULL,
    *shared = NULL;
    
    const char* guid_value = NULL;
    const char* url_value = NULL;
    const char* name_value = NULL;
    const char* remote_thumb_url = NULL;
    const char* remote_preview_url = NULL;
    
    size_t size_value = 0;
    int64_t creation_date_value = 0;
    int64_t  modification_date_value = 0;
    
    MHItemExifData* itemExifData = NULL;
    MHItemVideoMetaData* itemVideoMetadata = NULL;
    
    StringMap itemExportedServices;
    bool itemShared = false;
    bool gotExifCreationDate = false;
    
    int thumbnailsArraySize = 0;
    
    if (sourceItem == NULL) {
        return false;
    }
    
    if ((guid = cJSON_GetObjectItem(sourceItem, "id")) == NULL) {
        LOG.error("%s: missing \"guid\" field in json object", __FUNCTION__);
        return false;
    }
    
    guid_value = guid->valuestring;
    
    if ((url = cJSON_GetObjectItem(sourceItem, "url")) == NULL) {
        LOG.error("%s: missing \"url\" field in json object", __FUNCTION__);
        return false;
    }
    
    url_value = url->valuestring;
    
    if ((name = cJSON_GetObjectItem(sourceItem, "name")) == NULL) {
        LOG.error("%s: missing \"name\" field in json object", __FUNCTION__);
        return false;
    }
    
    name_value = name->valuestring;
    
    if ((size = cJSON_GetObjectItem(sourceItem, "size")) == NULL) {
        LOG.error("%s: missing \"size\" field in json object", __FUNCTION__);
        return false;
    }
    
    size_value = static_cast<size_t>(size->valueint);
    
    if ((modification_date = cJSON_GetObjectItem(sourceItem, "date")) != NULL) {
        modification_date_value = static_cast<int64_t>(modification_date->valueint);
        // Note: modificationdate value is a timestamp: it's an UTC value
    }
    
    if ((creation_date = cJSON_GetObjectItem(sourceItem, "creationdate")) != NULL) {
        creation_date_value = static_cast<int64_t>(creation_date->valueint);
        // Note: creationdate value is a timestamp: it's an UTC value
    }
    
    cJSON* binaryETag;
    const char* binaryETagValue = NULL;
    if ((binaryETag = cJSON_GetObjectItem(sourceItem, "etag")) != NULL) {
        binaryETagValue = binaryETag->valuestring;
    }
    
    
    if ((exif_data = cJSON_GetObjectItem(sourceItem, "exif")) != NULL) {
        StringBuffer maker_value(""), 
        model_value("");
        int64_t exifCreateDate = 0;
        int image_width_value = 0, 
        image_length_value = 0;
        
        if ((exif_maker = cJSON_GetObjectItem(exif_data, "Make")) != NULL) {
            maker_value = exif_maker->valuestring;
            maker_value.trim('\'');
        }
        
        if ((exif_model = cJSON_GetObjectItem(exif_data, "Model")) != NULL) {
            model_value = exif_model->valuestring;
            model_value.trim('\'');
        }
        
        if ((exif_image_width = cJSON_GetObjectItem(exif_data, "Exif Image Width")) != NULL) {
            if (exif_image_width->valuestring) {
                image_width_value = atoi(exif_image_width->valuestring);
            }
        }
        
        if ((exif_image_length = cJSON_GetObjectItem(exif_data, "Exif Image Length")) != NULL) {
            if (exif_image_length->valuestring) {
                image_length_value = atoi(exif_image_length->valuestring);
            }
        }
        
        if ((exif_creation_date = cJSON_GetObjectItem(exif_data, "Create Date")) != NULL) {
            struct tm tp;
            const char* exif_creation_date_value = exif_creation_date->valuestring;
            
            if ((exif_creation_date_value != NULL) && (strlen(exif_creation_date_value) > 0)) {
                memset(&tp, 0, sizeof(struct tm));

#if defined(FUN_MAC) || defined(FUN_IPHONE)
                if (strptime(exif_creation_date_value, "'%Y:%m:%d %H:%M:%S'", &tp)) {                
                    exifCreateDate = timelocal(&tp);    // EXIF dates are expressed in local time!
                    gotExifCreationDate = true;
                }

#else           // conversion on windows
                int year=0, month=0, day=0, hour=0, min=0, sec=0;
                int ret = sscanf(exif_creation_date_value, "'%d:%d:%d %d:%d:%d'", &year, &month, &day, &hour, &min, &sec);
                if (ret == 6) {   // ok (parsed all 6 items)
                    SYSTEMTIME sysTime;
                    sysTime.wYear = year;
                    sysTime.wMonth = month;
                    sysTime.wDay = day;
                    sysTime.wHour = hour;
                    sysTime.wMinute = min;
                    sysTime.wSecond = sec;
                    sysTime.wMilliseconds = 0;                
                    FILETIME UTCfileTime, localFileTime;
                    SystemTimeToFileTime(&sysTime, &UTCfileTime);
                    FileTimeToLocalFileTime(&UTCfileTime, &localFileTime);
                
                    ULARGE_INTEGER ull;
                    ull.LowPart = localFileTime.dwLowDateTime;
                    ull.HighPart = localFileTime.dwHighDateTime;
                    exifCreateDate =  static_cast<int64_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
                    gotExifCreationDate = true;   
                }
#endif

                if (gotExifCreationDate) {
                    LOG.debug("%s: remote item: GUID = %s, name = %s, EXIF date = %s -> %lli (%s)", __FUNCTION__, guid_value, name_value,
                        exif_creation_date_value, exifCreateDate, unixTimeToString(exifCreateDate, true).c_str());
                }
                else {
                    // error parsing the EXIF 'Create Date'
                    LOG.error("%s: Bad format of EXIF 'Create Date' for item '%s': %s, expected: 'YYYY:MM:DD HH:MM:SS' -> do not use it.", 
                        __FUNCTION__, name_value, exif_creation_date_value);
                    exifCreateDate = 0;
                }
            }
        }
        
        itemExifData = new MHItemExifData(maker_value, model_value, image_width_value, 
                                          image_length_value, exifCreateDate);
    }
    
    if ((video_metadata = cJSON_GetObjectItem(sourceItem, "metadata")) != NULL) {
        StringBuffer codec("");
        unsigned int video_duration_value = 0, video_bitrate_value = 0, 
        video_width_value = 0, video_height_value = 0;
        
        if ((video_codec = cJSON_GetObjectItem(video_metadata, "codec")) != NULL) {
            codec  = video_codec->valuestring;
        }
        
        if ((video_duration = cJSON_GetObjectItem(video_metadata, "duration")) != NULL) {
            video_duration_value = video_duration->valueint / 1000;
        }
        
        if ((video_bitrate = cJSON_GetObjectItem(video_metadata, "bitrate")) != NULL) {
            video_bitrate_value = video_bitrate->valueint;
        }
        
        if ((video_height = cJSON_GetObjectItem(video_metadata, "height")) != NULL) {
            video_height_value = video_height->valueint;
        }
        
        if ((video_width = cJSON_GetObjectItem(video_metadata, "width")) != NULL) {
            video_width_value = video_width->valueint;
        }
        
        itemVideoMetadata = new MHItemVideoMetaData(codec, video_duration_value, 
                                                    video_bitrate_value, video_height_value, video_width_value);
        
    }
    
    if ((shared = cJSON_GetObjectItem(sourceItem, "shared")) != NULL) {
        itemShared = shared->valueint ? true : false;
    }
    
    if ((exported_services = cJSON_GetObjectItem(sourceItem, "exported")) != NULL) {
        cJSON *exported_service = NULL;
        cJSON *exported_service_time = NULL;
        StringBuffer exported_service_time_str;
        unsigned long timestamp = 0;
        
        if ((exported_service = cJSON_GetObjectItem(exported_services, "picasa")) != NULL) {
            if ((exported_service_time = cJSON_GetObjectItem(exported_service, "exporttime")) != NULL) {
                timestamp = (exported_service_time->valueint / 1000);
                exported_service_time_str.sprintf("%ld", timestamp);
                itemExportedServices.put("picasa", exported_service_time_str.c_str());
                
                exported_service_time = NULL;
                exported_service_time_str.reset();
            }
        }
        
        exported_service = NULL;
        if ((exported_service = cJSON_GetObjectItem(exported_services, "flickr")) != NULL) {
            if ((exported_service_time = cJSON_GetObjectItem(exported_service, "exporttime")) != NULL) {
                timestamp = (exported_service_time->valueint / 1000);
                exported_service_time_str.sprintf("%lu", timestamp);
                itemExportedServices.put("flickr", exported_service_time_str.c_str());
                
                exported_service_time = NULL;
                exported_service_time_str.reset();
            }
        }
        
        exported_service = NULL;
        if ((exported_service = cJSON_GetObjectItem(exported_services, "facebook")) != NULL) {
            if ((exported_service_time = cJSON_GetObjectItem(exported_service, "exporttime")) != NULL) {
                timestamp = (exported_service_time->valueint / 1000);
                exported_service_time_str.sprintf("%lu", timestamp);
                itemExportedServices.put("facebook", exported_service_time_str.c_str());
                
                exported_service_time = NULL;
                exported_service_time_str.reset();
            }
        }
        
        exported_service = NULL;
        if ((exported_service = cJSON_GetObjectItem(exported_services, "youtube")) != NULL) {
            if ((exported_service_time = cJSON_GetObjectItem(exported_service, "exporttime")) != NULL) {
                timestamp = (exported_service_time->valueint / 1000);
                exported_service_time_str.sprintf("%lu", timestamp);
                itemExportedServices.put("youtube", exported_service_time_str.c_str());
                
                exported_service_time = NULL;
                exported_service_time_str.reset();
            }
        }
        
        exported_service = NULL;
        if ((exported_service = cJSON_GetObjectItem(exported_services, "twitter")) != NULL) {
            if ((exported_service_time = cJSON_GetObjectItem(exported_service, "exporttime")) != NULL) {
                timestamp = (exported_service_time->valueint / 1000);
                exported_service_time_str.sprintf("%lu", timestamp);
                itemExportedServices.put("twitter", exported_service_time_str.c_str());
                
                exported_service_time = NULL;
                exported_service_time_str.reset();
            }
        }
    }
    
    // Note that in some cases the remote thumb and preview url may be missing if the server
    // is not able to generate them. Still we want these items to be included in the digital life
    remote_thumb_url = NULL;
    remote_preview_url = NULL;
    cJSON* thumbETag = NULL;
    cJSON* previewETag = NULL;
    const char* remoteThumbETag = NULL;
    const char* remotePreviewETag = NULL;
    if ((thumbnails_array = cJSON_GetObjectItem(sourceItem, "thumbnails")) != NULL) {
        if ((thumbnailsArraySize = cJSON_GetArraySize(thumbnails_array)) == 2) {
            // Get the thumbnail
            if ((thumbnail_data = cJSON_GetArrayItem(thumbnails_array, 0)) != NULL) {    
                if ((thumbnail_url = cJSON_GetObjectItem(thumbnail_data, "url")) != NULL) {
                    remote_thumb_url = thumbnail_url->valuestring;
                }
                if ((thumbETag = cJSON_GetObjectItem(thumbnail_data, "etag")) != NULL) {
                    remoteThumbETag = thumbETag->valuestring;
                }
            }
            // Get the preview
            if ((preview_data = cJSON_GetArrayItem(thumbnails_array, 1)) != NULL) {
                if ((preview_url = cJSON_GetObjectItem(preview_data, "url")) != NULL) {
                    remote_preview_url = preview_url->valuestring;
                }
                if ((previewETag = cJSON_GetObjectItem(thumbnail_data, "etag")) != NULL) {
                    remotePreviewETag = previewETag->valuestring;
                }
            }
        }
    }
    
    if (remote_thumb_url == NULL) {
        remote_thumb_url = "";
    }
    
    if (remote_preview_url == NULL) {
        remote_preview_url = "";
    }
    
    if (remoteThumbETag == NULL) {
        remoteThumbETag = "";
    }
    
    if (remotePreviewETag == NULL) {
        remotePreviewETag = "";
    }
    
    // use the modification date for incoming server items last update
    time_t server_last_update = modification_date_value / 1000;
    
    // Just make sure EXIF creationDate is used (if existing) as the creation date of this item.
    // Items will be sorted by the 'creation_date_value'.
    if (itemExifData && gotExifCreationDate) {
        
        time_t exifDate = itemExifData->creation_date;
        time_t creationDateSecs = (time_t)(creation_date_value / 1000);
        if (exifDate > 0 && creationDateSecs != exifDate) {
            LOG.debug("%s: creation date mismatch for remote item '%s': use EXIF as creation date (%s)", 
                      __FUNCTION__, name_value, unixTimeToString(exifDate, true).c_str());
            // creation date is in msecs, while the exif one is in secs
            creation_date_value = ((int64_t)exifDate) * 1000;
        }
    }
    
    // Get labels info if present
    cJSON *labelsArray;
    if ((labelsArray = cJSON_GetObjectItem(sourceItem, "labels")) != NULL) {
        for(int i=0;i<cJSON_GetArraySize(labelsArray);++i) {
            // Get the label item
            cJSON* label;
            if ((label = cJSON_GetArrayItem(labelsArray, i)) != NULL) {
                cJSON* labelIdObj, *labelNameObj;
                uint32_t labelId = -1;
                const char* labelName = NULL;
                if ((labelIdObj = cJSON_GetObjectItem(label, "labelid")) != NULL) {
                    labelId = labelIdObj->valueint;
                }
                if ((labelNameObj = cJSON_GetObjectItem(label, "name")) != NULL) {
                    labelName = labelNameObj->valuestring;
                }
                if (labelId != -1 && labelName != NULL) {
                    MHLabelInfo* labelInfo = new MHLabelInfo(labelId, -1, labelName);
                    labels.addItem(itemInfo, labelInfo);
                }
            }
        }
    }
    
    itemInfo->setId(0);
    itemInfo->setGuid(guid_value);
    itemInfo->setLuid(NULL);
    itemInfo->setName(name_value);
    itemInfo->setSize(size_value);
    itemInfo->setServerUrl(downloadServerUrl);
    itemInfo->setCreationDate(creation_date_value);
    itemInfo->setModificationDate(modification_date_value);
    itemInfo->setStatus(EStatusUndefined);
    itemInfo->setServerLastUpdate(server_last_update);
    itemInfo->setRemoteItemUrl(url_value);
    itemInfo->setRemoteThumbUrl(remote_thumb_url);
    itemInfo->setRemotePreviewUrl(remote_preview_url);
    itemInfo->setShared(itemShared);
    itemInfo->setRemoteThumbETag(remoteThumbETag);
    itemInfo->setRemotePreviewETag(remotePreviewETag);
    
    if (binaryETagValue != NULL) {
        itemInfo->setRemoteItemETag(binaryETagValue);
    }
    
    if (itemExifData) {
        itemInfo->setItemExifData(*itemExifData);
        delete itemExifData;
    }
    
    if (itemVideoMetadata) {
        itemInfo->setItemVideoMetadata(*itemVideoMetadata);
        delete itemVideoMetadata;
    }
    
    if (itemExportedServices.empty() == false) {
        itemInfo->setExportedServices(itemExportedServices);
    }
    
    cJSON* validationStatus = cJSON_GetObjectItem(sourceItem, "status");
    if (validationStatus != NULL && validationStatus->valuestring != NULL) {
        char statusCode = validationStatus->valuestring[0];
        itemInfo->setValidationStatus(statusCode);
    } else {
        itemInfo->setValidationStatus(EValidationStatusValid);
    }
    
    return true;
}

ESMPStatus MHItemJsonParser::parseItemsChangeArray(cJSON* itemsArray, ArrayList& idList)
{
    int arraySize = 0;
    
    if (itemsArray == NULL) {
        LOG.error("%s: invalid json array object", __FUNCTION__);
        
        return ESMPInvalidArgument;
    }
    
    arraySize = cJSON_GetArraySize(itemsArray);
    
    if (arraySize) {
        for (int i = 0; i < arraySize; i++) {
            cJSON* itemID = cJSON_GetArrayItem(itemsArray, i);
            
            if (itemID) {                
                StringBuffer s = StringBuffer().sprintf("%d", static_cast<int>(itemID->valueint));
                
                idList.add(s);
            } else {
                LOG.error("%s: error getting source item id in json object array", __FUNCTION__); 
                
                return ESMPInvalidMessage;
            }
        }
    }
    
    return ESMPNoError;
}

bool MHItemJsonParser::parseMediaAddItem(const char* itemMetaDataUploadJson,
                                         const char* sourceName,
                                         StringBuffer& itemId,
                                         time_t* lastUpdate, ESMPStatus* errCode,
                                         MHSyncItemInfo* responseItem)
{
    cJSON *root = NULL;
    cJSON *id = NULL;
    cJSON *date = NULL;
    const char* id_value = NULL;
    bool error = false;
    
    // clear out id buffer
    itemId.reset();
    
    if (itemMetaDataUploadJson == NULL) {
        LOG.error("%s: json formatted object parameter in empty", __FUNCTION__);
        
        return false;
    }
    
    if ((root = cJSON_Parse(itemMetaDataUploadJson)) == NULL) {
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
    
    
    // possibile json reponses:
    // for uploads: {"success":"Item uploaded successfully","id":"410919","lastupdate":1302275919}
    // for updates: {"success":"Item uploaded successfully","lastupdate":1302275919}
    // see section 3.10 "Upload binary files" of funambol MH developer guide
    if ((id = cJSON_GetObjectItem(root, "id")) != NULL) {
        id_value = id->valuestring;
        
        if ((id_value == NULL) || (strlen(id_value) == 0)) {
            LOG.error("%s: empty id field in json object", __FUNCTION__);

        }
        
        itemId.assign(id_value);
    }
    
    // Get the "lastupdate" property = the upload time on the server
    if ((date = cJSON_GetObjectItem(root, "lastupdate")) != NULL) {
        // upload time from server is in millisecs
        *lastUpdate = static_cast<time_t>((date->valueint/1000));
        //*lastUpdate = static_cast<time_t>(date->valueint);
        // to be removed. try to see if it is a string...
        if (*lastUpdate == 0) {
            *lastUpdate = atoll(date->valuestring);
        }
    } else {
        *lastUpdate = 0;
    }
    
    // Parse the status to support content validation
    cJSON* status = cJSON_GetObjectItem(root, "status");
    if (status != NULL) {
        char statusCode = status->valuestring[0];
        responseItem->setValidationStatus(statusCode);
    }
    
    // Now parse the whole returned item
    bool res = true;
    CacheLabelsMap labels;
    cJSON* metadata = cJSON_GetObjectItem(root, "metadata");
    if (metadata != NULL) {
        cJSON* itemsArray;
        cJSON* serverUrlObj = cJSON_GetObjectItem(metadata, "mediaserverurl");
        const char* serverUrl = NULL;
        if (serverUrlObj != NULL) {
            serverUrl = serverUrlObj->valuestring;
        }
        if ((itemsArray = cJSON_GetObjectItem(metadata, sourceName)) != NULL) {
            uint32_t arraySize = cJSON_GetArraySize(itemsArray);
            if (arraySize == 1) {
                cJSON* sourceItem = cJSON_GetArrayItem(itemsArray, 0);
                if (sourceItem) {
                    res = parseSourceItem(sourceItem, labels, serverUrl, responseItem);
                }
            } else {
                LOG.error("%s: Upload sapi returned more than one item, this is not expected",__FUNCTION__);
            }
        }
    } else {
        // If the answer does not contain a valid item, we mark the responseItem as invalid
        responseItem->setId(-1);
    }
    
    cJSON_Delete(root);
    return res;
}

StringBuffer& MHItemJsonParser::getErrorCode() { 
    return errorCode;
}

StringBuffer& MHItemJsonParser::getErrorMessage() {
    return errorMessage;
}


END_FUNAMBOL_NAMESPACE
