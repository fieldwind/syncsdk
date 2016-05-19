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

#include <string>
#include <map>
#include "MediaHub/MHMediaRequestManager.h"
#include "MediaHub/MHMediaJsonParser.h"
#include "MediaHub/MHItemJsonParser.h"
#include "http/HttpConnection.h"
#include "http/BasicAuthentication.h"
#include "http/URL.h"
#include "ioStream/BufferInputStream.h"
#include "ioStream/StringOutputStream.h"
#include "ioStream/BufferOutputStream.h"
#include "event/FireEvent.h"

BEGIN_FUNAMBOL_NAMESPACE

// public static data
const char* MHMediaRequestManager::sortByCreationDateToken     = "creationdate";
const char* MHMediaRequestManager::sortByModificationDateToken = "date";
 
StringBuffer MHMediaRequestManager::sessionID;
StringBuffer MHMediaRequestManager::validationKey;
bool MHMediaRequestManager::validateRequest = true;     // if true, adds the validation key in all SAPI requests URL
pthread_mutex_t MHMediaRequestManager::sessionIDAccessMutex;
bool MHMediaRequestManager::sessionIDAccessMutexInitialized = false;

// static data for sapi media request URLs
static const char* getLastSyncUriFmt  = "%s/sapi/profile?action=get-last-sync";
static const char* getCountUriFmt     = "%s/sapi/media/%s?action=count&responsetime=true";
static const char* getUriFmt          = "%s/sapi/media/%s?action=get&responsetime=true&exif=none&shared=true";
static const char* getUriWithIdsFmt   = "%s/sapi/media/%s?action=get&id=%s&responsetime=true&exported=true&sortby=%s&sortorder=descending&shared=true";
static const char* getWithLimitUriFmt = "%s/sapi/media/%s?action=get&limit=%d&responsetime=true&exported=true&sortby=%s&sortorder=descending&shared=true";
static const char* getWithOffsetUriFmt = "%s/sapi/media/%s?action=get&offset=%d&responsetime=true&exported=true&sortby=%s&sortorder=descending&shared=true";
static const char* getWithLimitAndOffsetUrlFmt = "%s/sapi/media/%s?action=get&limit=%d&offset=%d&responsetime=true&exported=true&sortby=%s&sortorder=descending&shared=true"; 
static const char* getChangesUrlFmt   = "%s/sapi/profile/changes?action=get&from=%s&type=%s&responsetime=true&sortby=%s&sortorder=descending";
static const char* getQuotaInfoUrlFmt = "%s/sapi/media?action=get-storage-space&responsetime=true";
static const char* getLabelsFmt       = "%s/sapi/label?action=get";
//static const char* MHLoginUrlFmt    = "%s/sapi/login?action=login&responsetime=true&syncdeviceid=%s";
static const char* MHLogoutUrlFmt   = "%s/sapi/login?action=logout&responsetime=true";

// new sapi interface for updates
static const char* saveItemMetaDataFmt       = "%s/sapi/upload/%s?action=save-metadata&responsetime=true";
static const char* saveItemDataFmt           = "%s/sapi/upload/%s?action=save&lastupdate=true";

// subscriptions
static const char* getPlanByName              = "%s/sapi/subscription/plan?action=get&name=%s";
static const char* getCurrentPlan             = "%s/sapi/subscription?action=get";
static const char* registerPaymentUrl         = "%s/sapi/subscription/payment?action=save";
static const char* getSubscriptionList        = "%s/sapi/subscription/plan?action=get&paymenttype=%s";
static const char* getSubscriptionPaymentList = "%s/sapi/subscription/payment?action=get";
static const char* saveSubscription           = "%s/sapi/subscription?action=save";
static const char* cancelSubscription         = "%s/sapi/subscription?action=cancel";

// user profile
static const char* getUserProfileUrl  = "%s/sapi/profile?action=get";
static const char* putUserProfileUrl  = "%s/sapi/profile/generic?action=update";

// MH for url for delete
static const char* deleteItemData      = "%s/sapi/media/%s?action=delete";

// external services
static const char* getExternalServicesFmt = "%s/sapi/externalservice?action=get&devicetype=%s";
static const char* exportItemFmt          = "%s/sapi/media/%s?action=export&responsetime=true";
static const char* getExternalServiceAlbumsFmt  = "%s/sapi/externalservice/album?action=get-albums&servicename=%s";
static const char* getMediaSetFmt          = "%s/sapi/media/set?action=save&responsetime=true";
static const char* saveFacebookToken         = "%s/sapi/externalservice/authorization?action=save";

//-- profile properties
static const char* getProfilePropertyUrl = "%s/sapi/profile/properties?action=get";
static const char* setProfilePropertyUrl = "%s/sapi/profile/properties?action=set";

// send status report
static const char* sendStatusReportFmt          = "%s/sapi/activity?action=save";

// push notifications (Apple Push Notification service)
static const char* registerAPNsDeviceToken      = "%s/sapi/profile/device/token?action=save&deviceid=%s";

// get server information
static const char*  serverInfoUrlFmt          = "%s/sapi/system/information?action=get";

// sapi error codes
const char* MHMediaRequestManager::MHSecurityException         = "SEC-1000";
const char* MHMediaRequestManager::MHUserIdMissing             = "SEC-1001";
const char* MHMediaRequestManager::MHSessionAlreadyOpen        = "SEC-1002";
const char* MHMediaRequestManager::MHInvalidValidationKey      = "SEC-1003";
const char* MHMediaRequestManager::MHInvalidAuthSchema         = "SEC-1004";

static const char* MHUnknownMediaException     = "MED-1000"; 
static const char* MHInvalidContentRange       = "MED-1006";
static const char* MHUnableToRetrieveMedia     = "MED-1005";
static const char* MHUserQuotaReached          = "MED-1007";
static const char* MHIllicitContent            = "MED-1016";
static const char* MHNotValidatedContent       = "MED-1017";

static const char* MHPictureIdUsedWithLimit    = "PIC-1003";
static const char* MHUnableToRetrievePicture   = "PIC-1005";

static const char* MHInvalidDataTypeOrFormat   = "COM-1008";
static const char* MHOperationNotSupported     = "COM-1005";
static const char* MHMissingRequiredParameter  = "COM-1011";
static const char* MHInvalidLUID               = "COM-1014";

static const char* MHGeneralSubscriptionsError = "SUB-1000";

/* 
 * sapi errors code not yet used
 *
static const char* MHFileTooLarge              = "PIC-1007";
static const char* MHMediaIdWithLimit          = "MED-1003";
static const char* MHErrorRetrievingMediaItem  = "MED-1005";
static const char* MHPictureGenericError       = "PIC-1000";
static const char* MHNoPicturesSpecfied        = "PIC-1001";
static const char* MHInvalidFileName           = "COM-1010";
static const char* MHInvalidEmptyDataParameter = "COM-1013";
static const char* MHInvalidDeviceId           = "COM-1015";
*/

MHMediaRequestManager::MHMediaRequestManager(const char* url,
                                             const char* sapiSourceUri,
                                             const char* sapiArrayKey,
                                             const char* orderFieldValue,
                                             MHItemJsonParser* itemParser,
                                             AbstractSyncConfig* config_) :
    serverUrl(url),
    httpConnection(NULL),
    jsonMHMediaObjectParser(new MHMediaJsonParser()),
    jsonMHItemObjectParser(itemParser),
    auth(NULL),
    mhMediaSourceName(sapiSourceUri),
    itemsArrayKey(sapiArrayKey),
    orderField(orderFieldValue),
    config(config_)
{
    if (!sessionIDAccessMutexInitialized) {
        pthread_mutex_init(&sessionIDAccessMutex, NULL);
        sessionIDAccessMutexInitialized = true;
    }
    
    httpConnection = new HttpConnection(config->getUserAgent());
    deviceID = config->getDevID();
    credInfo = config->getCredInfo();

    Proxy  p;
    p.setProxy(NULL, 0, config->getProxyUsername(), config->getProxyPassword()); 
    httpConnection->setProxy(p);
    
    httpConnection->setSSLVerifyServer(config->getSSLVerifyServer());
}


MHMediaRequestManager::MHMediaRequestManager(const char* url, AbstractSyncConfig* config_) :
    serverUrl(url),
    httpConnection(NULL),
    jsonMHMediaObjectParser(new MHMediaJsonParser()),
    jsonMHItemObjectParser(new MHItemJsonParser()),
    auth(NULL),
    mhMediaSourceName(NULL),
    itemsArrayKey(NULL),
    orderField("creationdate"),
    config(config_)
{
    if (!sessionIDAccessMutexInitialized) {
        pthread_mutex_init(&sessionIDAccessMutex, NULL);
        sessionIDAccessMutexInitialized = true;
    }
    
    httpConnection = new HttpConnection(config->getUserAgent());
    
    deviceID = config->getDevID();
    credInfo = config->getCredInfo();

    Proxy  p;
    p.setProxy(NULL, 0, config->getProxyUsername(), config->getProxyPassword()); 
    httpConnection->setProxy(p);

    httpConnection->setSSLVerifyServer(config->getSSLVerifyServer());
}

MHMediaRequestManager::~MHMediaRequestManager()
{
    delete httpConnection;
    delete jsonMHMediaObjectParser;
    delete jsonMHItemObjectParser;
    delete auth;
}

EMHMediaRequestStatus MHMediaRequestManager::getItemsCount(int *itemCount)
{
    int status = 0;
    StringBuffer itemsCountRequestUrl;
    URL requestUrl;
    const char* itemsCountJsonObject = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInvalidParam;
    }
    
    *itemCount = 0;

    itemsCountRequestUrl.sprintf(getCountUriFmt, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(itemsCountRequestUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        httpConnection->close();
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((itemsCountJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("%s: server response = %s", __FUNCTION__, itemsCountJsonObject);

    if (jsonMHMediaObjectParser->parseItemCountObject(itemsCountJsonObject, itemCount, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("sapi error %s: %s", errorCode, errorMsg);

        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getAllItems(CacheItemsList& itemsInfoList,
                                                         CacheLabelsMap& labelsMap,
                                                         time_t* responseTime,
                                                         int limit, int offset)
{
    int status = 0;
    StringBuffer itemsListRequestUrl;
    URL requestUrl;
    const char* itemsListJsonObject = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    if (mhMediaSourceName == NULL) {
        LOG.error("%s: sapi source name unset", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    if ((limit == 0) && (offset == 0)) {
        itemsListRequestUrl.sprintf(getUriFmt, serverUrl.c_str(), mhMediaSourceName);
    } else if (offset == 0) { // paged get request
        itemsListRequestUrl.sprintf(getWithLimitUriFmt, serverUrl.c_str(), mhMediaSourceName, limit, orderField);
    } else if (limit == 0) {  // only offset       
        itemsListRequestUrl.sprintf(getWithOffsetUriFmt, serverUrl.c_str(), mhMediaSourceName, offset, orderField);
    } else { // both parameters speficied
        itemsListRequestUrl.sprintf(getWithLimitAndOffsetUrlFmt, serverUrl.c_str(), mhMediaSourceName, limit, offset, orderField);
    }
    
    requestUrl.setURL(itemsListRequestUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((itemsListJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("response returned = %s", itemsListJsonObject);

    
    if (itemsArrayKey == NULL) {
        LOG.error("%s: can't get valid source name token for json object parse", __FUNCTION__);
        
        return ESMRInternalError; 
    }
    
    if (jsonMHItemObjectParser->parseItemsListObject(itemsListJsonObject, itemsArrayKey, itemsInfoList,
                                                     labelsMap, responseTime, &parserStatus) == false) {
        const char* errorCode = jsonMHItemObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHItemObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getExternalServices(CacheItemsList& externalServicesList,
                                                                 bool isDesktop)
{
    int status = 0;
    StringBuffer externalServicesRequestUrl;
    URL requestUrl;
    const char* externalServicesJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    const char* deviceType = isDesktop ? "desktop" : "mobile";
    externalServicesRequestUrl.sprintf(getExternalServicesFmt, serverUrl.c_str(), deviceType);
 
    requestUrl.setURL(externalServicesRequestUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((externalServicesJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("response returned = %s", externalServicesJson);

    if (jsonMHMediaObjectParser->parseExternalServicesListObject(externalServicesJson,  externalServicesList, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::exportItemToExternalService(std::vector<ExportedItem*>& exportedItems,
                                                                         const char* sourceUri, time_t* responseTime)
{
    int status = 0;
    StringBuffer exportItemRequestUrl;
    URL requestUrl;
    StringOutputStream response;
    ESMPStatus parserStatus;
    const char *sourceName = sourceUri,
               *exportedItemJsonResponse = NULL;
    char* exportedItemJson = NULL;
    
    if (exportedItems.size() == 0) {
        LOG.error("invalid argument for export item request");
        
        return ESMRInvalidParam;
    }
    
    exportItemRequestUrl.sprintf(exportItemFmt, serverUrl.c_str(), sourceName);
    requestUrl.setURL(exportItemRequestUrl);

    if (jsonMHMediaObjectParser->formatJsonExportedItem(exportedItems, &exportedItemJson) == false) {
        LOG.error("error formatting exported item to json message");
        
        return ESMRMHMessageParseError;
    }

    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, exportedItemJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending export item request: %d", __FUNCTION__, status);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((exportedItemJsonResponse = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("%s: response returned = %s", __FUNCTION__, exportedItemJsonResponse);

    if (jsonMHMediaObjectParser->parseJsonExportedItemResponse(exportedItemJsonResponse, 
                                                               &parserStatus, responseTime) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("%s: response error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getLastSyncTimestamp(const char* sourceUri, 
                                                                  unsigned long *timestamp)
{
    int status = 0;
    StringBuffer itemsGetLastSyncRequestUrl;
    URL requestUrl;
    const char* lastSyncJsonObject = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    *timestamp = 0;
    
    itemsGetLastSyncRequestUrl.sprintf(getLastSyncUriFmt, serverUrl.c_str());
    requestUrl.setURL(itemsGetLastSyncRequestUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi get last sync timestamp request", __FUNCTION__);
        httpConnection->close();
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((lastSyncJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: server response = %s", __FUNCTION__, lastSyncJsonObject);
    
    if (jsonMHMediaObjectParser->parseLastSyncTimestampObject(lastSyncJsonObject, timestamp,
                                                              deviceID,  sourceUri,
                                                              &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("sapi error %s: %s", errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}
        
EMHMediaRequestStatus MHMediaRequestManager::getItemsFromId(CacheItemsList& itemsInfoList, CacheLabelsMap& labels,
                                                            const ArrayList& itemsIDs)
{
    int status = 0;
    StringBuffer itemsListRequestUrl;
    URL requestUrl;
    char* itemsIdsListJsonObject;     // formatted JSON object with items ids
    char* itemsIdsListEncoded = NULL; // urlencoded formatted JSON object with items ids
    const char* itemsListJsonObject = NULL;  // sapi reponse with list of items info JSON objects  
    StringOutputStream response;
    time_t requestTime;
    ESMPStatus parserStatus;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInvalidParam;
    }
    
    if (itemsIDs.size() == 0) {
        LOG.error("%s: list of items id is empty", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    if (itemsIDs.size() == 1) {
        StringBuffer* itemGuid = (StringBuffer*)itemsIDs.get(0);
        itemsListRequestUrl.sprintf(getUriWithIdsFmt, serverUrl.c_str(), mhMediaSourceName, itemGuid->c_str(), orderField);
    } else {
        if (jsonMHMediaObjectParser->formatItemsListObject(itemsIDs, &itemsIdsListJsonObject) == false) {
            LOG.error("%s: error formatting json object for items list", __FUNCTION__);
            return ESMRInvalidParam;
        }
    
        if ((itemsIdsListEncoded = URL::urlEncode(itemsIdsListJsonObject)) == NULL) {
            LOG.error("%s: error url encoding formatted json object with items list", __FUNCTION__);
            free(itemsIdsListJsonObject);
            return ESMRInvalidParam;
        }
        
        itemsListRequestUrl.sprintf(getUriWithIdsFmt, serverUrl.c_str(), mhMediaSourceName, itemsIdsListEncoded, orderField);
    
        free(itemsIdsListJsonObject);
        free(itemsIdsListEncoded);
    }

    
    requestUrl.setURL(itemsListRequestUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((itemsListJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("response returned = %s", itemsListJsonObject);

    if (itemsArrayKey == NULL) {
        LOG.error("%s: can't get valid source name token for json object parse", __FUNCTION__);
        
        return ESMRInternalError; 
    }
    
    if (jsonMHItemObjectParser->parseItemsListObject(itemsListJsonObject, itemsArrayKey, 
                                        itemsInfoList, labels, &requestTime, &parserStatus) == false) {
        StringBuffer errorCode = jsonMHItemObjectParser->getErrorCode().c_str();
        StringBuffer errorMsg  = jsonMHItemObjectParser->getErrorMessage().c_str();
        
        if (errorCode.empty() || errorMsg.empty()) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode.c_str(), errorMsg.c_str());
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHIllicitContent)) {
            return ESMRMHIllicitContent;
        } else if (!strcmp(errorCode, MHNotValidatedContent)) {
            return ESMRMHNotValidatedContent;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getSourcesWithPendingChanges(ArrayList& sourcesToCheck,
                                                                          const StringBuffer& fromDate,  
                                                                          time_t* reponseTimestamp,
                                                                          ArrayList& sourcesWithPendingChanges,
                                                                          bool includingDeletes) 
{
    int status = 0;
    StringBuffer itemChangesRequestUrl;
    URL requestUrl;
    const char* itemsChangesJsonObject = NULL; // JSON object from server
    StringOutputStream response;
    StringBuffer date = fromDate;
    ESMPStatus parserStatus;
    
    if (sourcesToCheck.size() == 0) {
        return ESMRInvalidParam;
    }
    
    if (date.empty()) {
        LOG.debug("%s: timestamp empty: getting all changes from server", __FUNCTION__);
        date = "19700101T000000Z";
    }
    
    // Compose the list of sources to query
    StringBuffer sourcesList;
    for(int i=0;i<sourcesToCheck.size();++i) {
        StringBuffer* sourceUri = (StringBuffer*)sourcesToCheck.get(i);
        if (!sourcesList.empty()) {
            sourcesList.append(",");
        }
        sourcesList.append(sourceUri);    
    }
    
    itemChangesRequestUrl.sprintf(getChangesUrlFmt, serverUrl.c_str(), date.c_str(),
                                  sourcesList.c_str(), orderField);
    
    requestUrl.setURL(itemChangesRequestUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((itemsChangesJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", itemsChangesJsonObject);
    
    *reponseTimestamp = 0L;
    
    bool parserRes = jsonMHItemObjectParser->parseItemsChangesObjectForPendingChanges(
                                                                      itemsChangesJsonObject, 
                                                                      sourcesToCheck,
                                                                      sourcesWithPendingChanges, 
                                                                      reponseTimestamp,
                                                                      &parserStatus,
                                                                      includingDeletes);
    if (parserRes == false) {
        LOG.error("%s: error parsing sapi json object", __FUNCTION__);
        
        switch (parserStatus) {
            case ESMPKeyMissing:
                // is some required field of JSON object has not been found
                // consider the sapi on server as not supported 
                return ESMRMHNotSupported;
                
            default:
                return ESMRMHMessageParseError;
        } 
    }
    
    return ESMRSuccess;    
}

EMHMediaRequestStatus MHMediaRequestManager::getItemsChanges(ArrayList& newIDs,
                                                             ArrayList& modIDs, 
                                                             ArrayList& delIDs,
                                                             const StringBuffer& fromDate, 
                                                             time_t* reponseTimestamp) {
    return getItemsChanges(newIDs, modIDs, delIDs,
                           fromDate, reponseTimestamp, "", "");
    
}



EMHMediaRequestStatus MHMediaRequestManager::getItemsChanges(ArrayList& newIDs,
                                                             ArrayList& modIDs, 
                                                             ArrayList& delIDs,
                                                             const StringBuffer& fromDate, 
                                                             time_t* reponseTimestamp,
                                                             const StringBuffer& username,
                                                             const StringBuffer& password) 
{
    int status = 0;
    StringBuffer itemChangesRequestUrl;
    URL requestUrl;
    const char* itemsChangesJsonObject = NULL; // JSON object from server
    const char* sourceTokenName = mhMediaSourceName;
    StringOutputStream response;
    StringBuffer date = fromDate;
    ESMPStatus parserStatus;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInvalidParam;
    }
    
    if (date.empty()) {
        LOG.debug("%s: timestamp empty: getting all changes from server", __FUNCTION__);
        date = "19700101T000000Z";
    }
    
    itemChangesRequestUrl.sprintf(getChangesUrlFmt, serverUrl.c_str(), date.c_str(),
                                  mhMediaSourceName, orderField);
    requestUrl.setURL(itemChangesRequestUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((itemsChangesJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("response returned = %s", itemsChangesJsonObject);

    if (sourceTokenName == NULL) {
        LOG.error("%s: can't get valid source name token for json object parse", __FUNCTION__);
        
        return ESMRInternalError; 
    }
    
    *reponseTimestamp = 0L;
    
    if (jsonMHItemObjectParser->parseItemsChangesObject(itemsChangesJsonObject, 
                sourceTokenName, newIDs, modIDs, delIDs, reponseTimestamp, &parserStatus) == false) {
        LOG.error("%s: error parsing sapi json object", __FUNCTION__);
   
        switch (parserStatus) {
            case ESMPKeyMissing:
                // is some required field of JSON object has not been found
                // consider the sapi on server as not supported 
                return ESMRMHNotSupported;
                
            default:
                return ESMRMHMessageParseError;
        } 
    }
  
    return ESMRSuccess;
}
       

EMHMediaRequestStatus MHMediaRequestManager::uploadItemMetaData(UploadMHSyncItem* item, bool updateMetaDataOnly)
{
    char* itemJsonMetaData;
    const char* itemMetaDataUploadJson = NULL;
    MHSyncItemInfo* itemInfo = NULL;
    StringBuffer itemId;
    StringBuffer itemMetaDataAddRequestUrl;
    URL requestUrl;
    StringOutputStream response;
    int status = 0;
    ESMPStatus parserStatus;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInternalError;
    }
    
    if (item == NULL) {
        LOG.error("%s: invalid upload sapi item", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    if ((itemInfo = item->getMHSyncItemInfo()) == NULL) {
        LOG.error("%s: invalid upload sapi item (no item info set)", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    if ((jsonMHMediaObjectParser->formatMediaItemMetaData(itemInfo, &itemJsonMetaData, updateMetaDataOnly)) == false) {
        LOG.error("%s: error formatting item meta data as json object", __FUNCTION__);
    
        return ESMRMHMessageFormatError;
    }
    
    //LOG.debug("JSON request body to send:\n%s", itemJsonMetaData);
    itemMetaDataAddRequestUrl.sprintf(saveItemMetaDataFmt, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(itemMetaDataAddRequestUrl);

    LOG.debug("%s: Json request body to send: \n%s", __FUNCTION__, itemJsonMetaData);
     
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, itemJsonMetaData, response)) != HTTP_OK) {
        LOG.error("%s: error sending upload request", __FUNCTION__);
        free(itemJsonMetaData);
        
        // Handle special status here if needed
        if (status == HTTP_SERVER_ERROR) {
            return ESMRUnknownMediaException;
        } else {
            EMHMediaRequestStatus res = handleHttpError(status);
            return res;
        }
    }
    
    free(itemJsonMetaData);
    
    if ((itemMetaDataUploadJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid empty response for sapi item metadata upload", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: sapi add item metadata request response = %s", __FUNCTION__, itemMetaDataUploadJson);
    
    time_t lastUpdate = 0;  // unused
    MHSyncItemInfo* clonedItem = itemInfo->clone(); // unused
    bool parserRes = jsonMHItemObjectParser->parseMediaAddItem(itemMetaDataUploadJson, itemsArrayKey, itemId, &lastUpdate,
                                                               &parserStatus, clonedItem);
    delete clonedItem;
    if (parserRes == false) {        
        const char* errorCode = jsonMHItemObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHItemObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }

        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHUnknownMediaException)) {
            return ESMRUnknownMediaException;
        } else if (!strcmp(errorCode, MHInvalidContentRange)) {
            return ESMRInvalidContentRange;
        } else if (!strcmp(errorCode, MHMissingRequiredParameter)) {
            return ESMRMissingRequiredParameter;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHUserQuotaReached)) {
            return ESMRQuotaExceeded;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHUnableToRetrieveMedia)) {
            return ESMRErrorRetrievingMediaItem;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }

        return ESMRMHMessageParseError;
    }
    
    if (itemId.empty() == false) {
        LOG.debug("%s: setting item id '%s'", __FUNCTION__, itemId.c_str());
    
        itemInfo->setGuid(itemId.c_str());
    }
    
    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::updateFacebookToken(const char* token, const char* accountName, long expTime)
{
    char* itemJsonMetaData;
    const char* responseJSON = NULL;
    StringBuffer itemMetaDataAddRequestUrl;
    URL requestUrl;
    StringOutputStream response;
    int status = 0;

    if (strlen(token) == 0) {
        LOG.error("%s: invalid token (empty)", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    cJSON *data = NULL;
    if ((data = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
    }
    
    cJSON_AddStringToObject (data, "token", token);
    cJSON_AddFloatToObject  (data, "expiretime", (double)expTime*1000);
    cJSON_AddStringToObject (data, "servicename", "facebook");
    cJSON_AddStringToObject (data, "accountname", accountName);
    
    cJSON *message = NULL;
    
    if ((message = cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
    }
    
    cJSON_AddItemReferenceToObject(message, "data", data);
    
    //LOG.debug("JSON request body to send:\n%s", cJSON_Print(data));
    itemJsonMetaData = cJSON_Print(message);
    itemMetaDataAddRequestUrl.sprintf(saveFacebookToken, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(itemMetaDataAddRequestUrl);
    
    LOG.debug("%s: Json request body to send: \n%s", __FUNCTION__, itemJsonMetaData);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, itemJsonMetaData, response)) != HTTP_OK) {
        LOG.error("%s: error sending upload request", __FUNCTION__);
        free(itemJsonMetaData);
        
        // Handle special status here if needed
        if (status == HTTP_SERVER_ERROR) {
            return ESMRMHNotSupported;
        } else {
            EMHMediaRequestStatus res = handleHttpError(status);
            return res;
        }
    }
    
    free(itemJsonMetaData);
    
    if ((responseJSON = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid empty response for sapi item metadata upload", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: sapi add item metadata request response = %s", __FUNCTION__, responseJSON);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::uploadItemData(UploadMHSyncItem* item,
                                                 time_t* lastUpdate,
                                                 bool isUpdate,
                                                 HttpConnectionUploadObserver* uploadObserver,
                                                 MHSyncItemInfo* responseItem)
{
    int status = 0;
    MHSyncItemInfo* itemInfo = NULL;
    const char* itemContentType = NULL;
    const char* itemUploadResultJsonObject = NULL;
    InputStream* itemDataStream = NULL;
    StringOutputStream response;
    StringBuffer itemUploadUrl;
    StringBuffer itemGuid;
    URL requestUrl;
    int64_t dataSize = 0, partialSize = 0;
    ESMPStatus parserStatus;
    StringBuffer itemId;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInternalError;
    }
        
    if (item == NULL) {
        LOG.error("%s: invalid sync item", __FUNCTION__);
        return ESMRInvalidParam;
    }

    if ((itemInfo = item->getMHSyncItemInfo()) == NULL) {
        LOG.error("%s: no item info found in sync item", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    itemId = itemInfo->getGuid();
    if ((itemId == NULL) && (strlen(itemId) == 0)) {
        LOG.error("%s: no guid found in item info", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    itemContentType = itemInfo->getContentType();
     
    if ((itemDataStream = item->getStream()) == NULL) {
        LOG.error("%s: invalid output stream in sync item", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    if ((dataSize = itemDataStream->getTotalSize()) == 0) {
        LOG.error("%s: invalid sync item stream (zero size stream attached)", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    partialSize = itemDataStream->getPosition();
    
    
    itemUploadUrl.sprintf(saveItemDataFmt, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(itemUploadUrl);

    fireTransportEvent(itemInfo->getSize(), SEND_DATA_BEGIN);

    // set requst headers
    if (partialSize > 0) {
        StringBuffer contentRange;
        contentRange.sprintf("bytes %lld-%lld/%lld", partialSize, dataSize - 1, dataSize);
        httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_RANGE, contentRange.c_str());
        fireTransportEvent(partialSize, DATA_ALREADY_COMPLETED);
     }

    if ((itemContentType != NULL) && (strlen(itemContentType) > 0)) {
        httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, itemContentType);
    } else {
        httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/octet-stream");
    }
    
    setUploadItemExtendedHeaders(item);


    // set Funambol mandatory custom headers
    httpConnection->setRequestHeader(HTTP_HEADER_X_FUNAMBOL_ID, itemId);
    StringBuffer dsize;
    dsize.sprintf("%lld", dataSize);
    httpConnection->setRequestHeader(HTTP_HEADER_X_FUNAMBOL_FILE_SIZE, dsize.c_str());
    httpConnection->setUploadObserver(uploadObserver);    
    status = performRequest(requestUrl, HttpConnection::MethodPost, *itemDataStream, response);
    httpConnection->setUploadObserver(NULL);
    
    if ((status != HTTP_OK) && (status != HTTP_PARTIAL_CONTENT)) {
        LOG.error("%s: error sending upload request", __FUNCTION__);

        fireTransportEvent(0, SEND_DATA_END);
        if (status == HTTP_SERVER_ERROR) {
            return ESMRUnknownMediaException; 
        } else {
            // Handle special status here if needed
            EMHMediaRequestStatus res = handleHttpError(status);
            return res;
        }
    }

    fireTransportEvent(item->getMHSyncItemInfo()->getSize(), SEND_DATA_END);

    if ((itemUploadResultJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: item add result = %s", __FUNCTION__, response.getString().c_str());
    
    bool parserRes = jsonMHItemObjectParser->parseMediaAddItem(itemUploadResultJsonObject,itemsArrayKey,
                                                               itemGuid, lastUpdate,
                                                               &parserStatus, responseItem);
    if (parserRes == false) {
        const char* errorCode = jsonMHItemObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHItemObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
           
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                default:
                    return ESMRMHMessageParseError;
            } 
        }

        LOG.error("%s: error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHUnknownMediaException)) {
            return ESMRUnknownMediaException;
        } else if (!strcmp(errorCode, MHInvalidContentRange)) {
            return ESMRInvalidContentRange;
        } else if (!strcmp(errorCode, MHMissingRequiredParameter)) {
            return ESMRMissingRequiredParameter;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHUserQuotaReached)) {
            return ESMRQuotaExceeded;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHUnableToRetrieveMedia)) {
            return ESMRErrorRetrievingMediaItem;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }

        return ESMRMHMessageParseError;
    }
    
    if (itemGuid != itemId) {
        LOG.error("%s: add item error: item id mismatch!", __FUNCTION__);
        return ESMRMHInvalidResponse;
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getItemResumeInfo(UploadMHSyncItem* item, int64_t* offset)
{
    int status = 0;
    MHSyncItemInfo* itemInfo = NULL;
    const char* itemId = NULL;
    InputStream* itemDataStream = NULL;
    StringOutputStream response;
    StringBuffer itemResumeInfoUrl;
    StringBuffer contentRange;
    URL requestUrl;
    int64_t dataSize = 0;
    
    *offset = 0;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInternalError;
    }
        
    if (item == NULL) {
        LOG.error("%s: invalid sync item", __FUNCTION__);
         
        return ESMRInvalidParam;
    }

    if ((itemInfo = item->getMHSyncItemInfo()) == NULL) {
        LOG.error("%s: no item info found in sync item", __FUNCTION__);
         
        return ESMRInvalidParam;
    }
    
    if ((itemDataStream = item->getStream()) == NULL) {
        LOG.error("%s: invalid output stream in sync item", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    if ((dataSize = itemDataStream->getTotalSize()) == 0) {
        LOG.error("%s: invalid sync item stream (zero size stream attached)", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    itemId = itemInfo->getGuid();
    if ((itemId == NULL) || (strlen(itemId) == 0)) {
        LOG.error("%s: no guid found in item info", __FUNCTION__);
        return ESMRInvalidParam;
    }

    itemResumeInfoUrl.sprintf(saveItemDataFmt, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(itemResumeInfoUrl);

    // set requst headers
    contentRange.sprintf("bytes */%lld", dataSize);

    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_RANGE, contentRange.c_str());
    // set Funambol mandatory custom headers
    httpConnection->setRequestHeader(HTTP_HEADER_X_FUNAMBOL_ID, itemId);

    StringBuffer dsize;
    dsize.sprintf("%lld", dataSize);
    httpConnection->setRequestHeader(HTTP_HEADER_X_FUNAMBOL_FILE_SIZE, dsize.c_str());

    status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response);
    
    if (status == HTTP_OK) {
        // We may have received a SAPI error or the item is completed
        LOG.debug("%s: Resume info response %s", __FUNCTION__, response.getString().c_str());
        if (jsonMHMediaObjectParser->checkErrorMessages(response.getString().c_str()) == false) {
            
            const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
            const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
            LOG.error("%s: SAPI error %s: %s", __FUNCTION__, errorCode, errorMsg);
            
            if (errorCode != NULL) {
                if (!strcmp(errorCode, MHUnknownMediaException) || !strcmp(errorCode, MHUnableToRetrieveMedia)) {
                    LOG.debug("%s: Resume is not possible due to invalid server item: %i. Start from scratch", __FUNCTION__, errorCode);
                } else {
                    LOG.error("%s: Resume is not possible due to unexpected error: %i. Start from scratch", __FUNCTION__, errorCode);  
                    if (!strcmp(errorCode, MHInvalidValidationKey)) {
                        resetValidationKey();
                    }
                }
            }
            *offset = -1;
            httpConnection->close();
            return ESMRUnknownMediaException;
                       
        } else { // see doc http://docs.funambol.com/server-api-developers-guide-11-0-2?c=3#sect-resumable-upload ex 4.
            LOG.debug("%s: Resume is not needed, the upload was complete", __FUNCTION__);
            *offset = dataSize;
        }
    } else if (status == HTTP_RESUME_INCOMPLETE) {
        StringBuffer rangeHdr = lastHttpResponseHeaders["Range"];
        ArrayList rangeValues;
        
        rangeHdr.split(rangeValues, "-");
        
        if (rangeValues.size() != 2) {
            LOG.error("%s: error parsing HTTP range headers", __FUNCTION__);
            httpConnection->close();
			return ESMRInvalidContentRange;
        } else {
            StringBuffer* rangeOffset = static_cast<StringBuffer *>(rangeValues.get(1));
        
            if ((rangeOffset == NULL) || (rangeOffset->empty())) {
                LOG.error("%s: error parsing HTTP range headers", __FUNCTION__);
                httpConnection->close();
                return ESMRInvalidContentRange;
            }
            
            // *offset = atol(rangeOffset->c_str());            
            *offset = static_cast<int64_t>(atoll(rangeOffset->c_str()));
            LOG.debug("%s: item offset set to: %lld", __FUNCTION__, *offset);
        }
    } else {
        LOG.error("%s: error sending HTTP item resume info request [HTTP code: %d]", __FUNCTION__, status);
        httpConnection->close();
        
       // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    httpConnection->close();

    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::downloadItem(DownloadMHSyncItem* item)
{
    int status = 0;
    MHSyncItemInfo* itemInfo = NULL;
    OutputStream* itemDataStream = NULL;
    const char* itemUrlFmt = "%s%s";
    const char* itemUrl    = NULL;
    StringBuffer itemServerUrl;
    StringBuffer itemRequestUrl;
    URL requestUrl;
    int64_t partialSize = 0;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInternalError;
    }
    
    if (item == NULL) {
        LOG.error("%s: invalid sync item", __FUNCTION__);
         
        return ESMRInvalidParam;
    }

    if ((itemInfo = item->getMHSyncItemInfo()) == NULL) {
        LOG.error("%s: no item info found in sync item", __FUNCTION__);
         
        return ESMRInvalidParam;
    }
    
    // get server name from item info  
    itemServerUrl = itemInfo->getServerUrl();
    itemUrl = itemInfo->getRemoteItemUrl();
    
    if ((itemUrl == NULL) || (strlen(itemUrl) == 0)) {
        LOG.error("%s: no download url found in item info", __FUNCTION__);
         
        return ESMRInvalidParam;
    }
    
    if ((itemDataStream = item->getStream()) == NULL) {
        LOG.error("%s: invalid output stream in sync item", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
   
    // format request URL: if item server url is not set, use default (set from client)
    itemRequestUrl.sprintf(itemUrlFmt, itemServerUrl.empty() ? serverUrl.c_str() : itemServerUrl.c_str(), itemUrl);
    
    LOG.debug("%s: media item url to fetch: %s", __FUNCTION__, itemRequestUrl.c_str());
     
    requestUrl.setURL(itemRequestUrl);
    
    // check if output stream has already data set and we have to make a resume request
    partialSize = itemDataStream->size();
    
    if (partialSize > 0) {
        StringBuffer contentRange;
        int64_t dataSize = 0;
        
        // get total item size from item info
        if ((dataSize = itemInfo->getSize()) == 0) {
            LOG.error("%s: item info has zero size - can't complete download request", __FUNCTION__);
            return ESMRInvalidParam;
        }

        // some checks on the partial data size
        if (partialSize == dataSize) {
            LOG.debug("%s: partial item is already the whole content, no download is done", __FUNCTION__);
            return ESMRSuccess;
        }
        else if (partialSize > dataSize) {
            LOG.error("%s: partial item downloaded is bigger (%lld bytes) than the final item size (%lld bytes)", 
                __FUNCTION__, partialSize, dataSize);
            LOG.info("item '%s' can't be resumed, will be downloaded from scratch next time", itemInfo->getName().c_str());
            return ESMRInvalidContentRange;
        }
        
        contentRange.sprintf("bytes=%lld-%lld", partialSize, dataSize - 1);
        httpConnection->setRequestHeader(HTTP_HEADER_RANGE, contentRange.c_str());
    }
    
    // fire the begin of the download with the size of the data to download
    fireTransportEvent(itemInfo->getSize(), RECEIVE_DATA_BEGIN);
    if (partialSize > 0) {
        fireTransportEvent(partialSize, DATA_ALREADY_COMPLETED);
    }
   
    status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, *itemDataStream, true, false);    
    
    if ((status != HTTP_OK) && (status != HTTP_PARTIAL_CONTENT)) {
        LOG.error("%s: error downloading item (http status = %d)", __FUNCTION__, status);
        
        fireTransportEvent(0, RECEIVE_DATA_END);
       
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    fireTransportEvent(item->getMHSyncItemInfo()->getSize(), RECEIVE_DATA_END);

    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::getQuotaInfo(int64_t* free,uint64_t* quota)
{
    int status = 0;
    StringBuffer quotaInfoUrl;
    URL requestUrl;
    const char* quotaInfoJsonObject = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    quotaInfoUrl.sprintf(getQuotaInfoUrlFmt, serverUrl.c_str(), mhMediaSourceName);
    requestUrl.setURL(quotaInfoUrl);

    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }

    if ((quotaInfoJsonObject = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);

        return ESMRMHInvalidResponse;
    }

    LOG.debug("response returned = %s", quotaInfoJsonObject);

    if (jsonMHMediaObjectParser->parseQuotaInfoObject(quotaInfoJsonObject, free, quota, &parserStatus) == false) {
        
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi json object", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
  
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getCurrentSubscription(SapiSubscription& currentSubscription)
{
    int status = 0;
    StringBuffer getCurrentSubscriptionUrl;
    URL requestUrl;
    const char* getCurrentSubscriptionJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    getCurrentSubscriptionUrl.sprintf(getCurrentPlan, serverUrl.c_str());
    requestUrl.setURL(getCurrentSubscriptionUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getCurrentSubscriptionJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", getCurrentSubscriptionJson);
    
    if (jsonMHMediaObjectParser->parseJsonCurrentSubscription(getCurrentSubscriptionJson, currentSubscription, &parserStatus) == false) 
    {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) 
        {
            switch (parserStatus) 
            {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHGeneralSubscriptionsError)) {               
            return ESMGeneralSubscriptionsError;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getSubscriptionByName(const char *subscriptionName, SapiSubscription& subscription)
{
    int status = 0;
    StringBuffer getSubscriptionByNameUrl;
    URL requestUrl;
    const char* getSubscriptionByNameJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    getSubscriptionByNameUrl.sprintf(getPlanByName, serverUrl.c_str(), subscriptionName);
    requestUrl.setURL(getSubscriptionByNameUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getSubscriptionByNameJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", getSubscriptionByNameJson);
    
    std::vector<SapiSubscription *> subscriptions;
    if (jsonMHMediaObjectParser->parseJsonSubscriptionList(getSubscriptionByNameJson, subscriptions, &parserStatus) == false) // CurrentSubscription(getSubscriptionByNameJson, subscription, &parserStatus) == false) 
    {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) 
        {
            switch (parserStatus) 
            {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHGeneralSubscriptionsError)) {               
            return ESMGeneralSubscriptionsError;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    if (!subscriptions.empty()) {
        subscription = *subscriptions[0];
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getSubscriptions(std::vector<SapiSubscription*>& subscriptions, bool useIAP)
{
    int status = 0;
    StringBuffer getSubscriptionListUrl;
    URL requestUrl;
    const char* getSubscriptionListJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    getSubscriptionListUrl.sprintf(getSubscriptionList, serverUrl.c_str(), (useIAP ? "apple" : "default"));
    requestUrl.setURL(getSubscriptionListUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getSubscriptionListJson = response.getString().c_str()) == NULL) {
        // If there are no valid subscriptions, then we may receive an empty response (status 200)
        // This is not really correct, as the returned status should not be 200 in this case, or a
        // JSON with an empty list should be returned, but we must cope with the current server
        // implementation and handle this case.
        LOG.error("%s: empty subscription response, means no subscriptions available", __FUNCTION__);
        return ESMRSuccess;
    }
    
    LOG.debug("response returned = %s", getSubscriptionListJson);
    
    if (strlen(getSubscriptionListJson) > 0) {
        if (jsonMHMediaObjectParser->parseJsonSubscriptionList(getSubscriptionListJson, subscriptions, &parserStatus) == false) 
        {
            const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
            const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
            
            if (!errorCode || !errorMsg) 
            {
                switch (parserStatus) 
                {
                    case ESMPKeyMissing:
                        // is some required field of JSON object has not been found
                        // consider the sapi on server as not supported 
                        return ESMRMHNotSupported;
                        
                    default:
                        return ESMRMHMessageParseError;
                }
            }
            
            LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
            
            if (!strcmp(errorCode, MHGeneralSubscriptionsError)) {               
                return ESMGeneralSubscriptionsError;
            } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
                resetValidationKey();
                return ESMRInvalidValidationKey;
            }
            
            return ESMRMHMessageParseError; 
        }
    }
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::purchaseSubscription(const char* subscription)
{
    int status = 0;
    StringBuffer purchaseSubscriptionUrl;
    URL requestUrl;
    char* requestBody = NULL;                   // formatted JSON object with items ids
    const char* purchaseSubscriptionJson = NULL; // JSON object from server
    StringOutputStream response;
    //ESMPStatus parserStatus;
    
    purchaseSubscriptionUrl.sprintf(saveSubscription, serverUrl.c_str());
    requestUrl.setURL(purchaseSubscriptionUrl);
    
    //
    // format the json body
    //
    if (jsonMHMediaObjectParser->formatSaveSubscriptionObject(subscription, &requestBody, false) == false) {
        LOG.error("%s: error formatting json object", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, requestBody, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        
        free(requestBody);
        return res;
    }
    free(requestBody);
    
    if ((purchaseSubscriptionJson = response.getString().c_str()) == NULL) {
        // If there are no valid subscriptions, then we may receive an empty response (status 200)
        // This is not really correct, as the returned status should not be 200 in this case, or a
        // JSON with an empty list should be returned, but we must cope with the current server
        // implementation and handle this case.
        LOG.error("%s: empty subscription response, means no subscriptions available", __FUNCTION__);
        return ESMRSuccess;
    }
    
    LOG.debug("response returned = %s", purchaseSubscriptionJson);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::cancelCurrentSubscription()
{
    int status = 0;
    StringBuffer cancelSubscriptionUrl;
    URL requestUrl;
    const char* cancelSubscriptionJson = NULL; // JSON object from server
    StringOutputStream response;
    //ESMPStatus parserStatus;
    
    cancelSubscriptionUrl.sprintf(cancelSubscription, serverUrl.c_str());
    requestUrl.setURL(cancelSubscriptionUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        
        return res;
    }
    
    LOG.debug("response returned = %s", cancelSubscriptionJson);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getSubscriptionPayments(std::vector<SapiPayment *>& payments)
{
    int status = 0;
    StringBuffer getSubscriptionPaymentsUrl;
    URL requestUrl;
    const char* getSubscriptionPaymentsJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    getSubscriptionPaymentsUrl.sprintf(getSubscriptionPaymentList, serverUrl.c_str());
    requestUrl.setURL(getSubscriptionPaymentsUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getSubscriptionPaymentsJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", getSubscriptionPaymentsJson);
    
    if (jsonMHMediaObjectParser->parseJsonSubscriptionPaymentList(getSubscriptionPaymentsJson, payments, &parserStatus) == false) 
    {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) 
        {
            switch (parserStatus) 
            {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHGeneralSubscriptionsError)) {               
            return ESMGeneralSubscriptionsError;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

//
// sapi user profile
//
EMHMediaRequestStatus MHMediaRequestManager::getUserProfile(SapiUserProfile &userProfile)
{
    int status = 0;
    StringBuffer getUserProfileRequestUrl;
    URL requestUrl;
    const char* getUserProfileJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    getUserProfileRequestUrl.sprintf(getUserProfileUrl, serverUrl.c_str());
    requestUrl.setURL(getUserProfileRequestUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getUserProfileJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", getUserProfileJson);
    
    if (jsonMHMediaObjectParser->parseJsonUserProfileObject(getUserProfileJson, userProfile, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::getProfileProperty(const char *profilePropertyKey, char *profilePropertyValue)
{
    int status = 0;
    StringBuffer getProfilePropertyRequestUrl;
    URL requestUrl;
    char* profilePropertyKeyJson = NULL;           // JSON object to POST
    const char* getProfilePropertyJson = NULL;  // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    // Format json
    if (jsonMHMediaObjectParser->formatProfilePropertyKeyJson(profilePropertyKey, &profilePropertyKeyJson) == false) {
        LOG.error("%s: error formatting json object for profile profile key", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    getProfilePropertyRequestUrl.sprintf(getProfilePropertyUrl, serverUrl.c_str());
    requestUrl.setURL(getProfilePropertyRequestUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, profilePropertyKeyJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((getProfilePropertyJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("response returned = %s", getProfilePropertyJson);
    
    if (jsonMHMediaObjectParser->parseProfilePropertyObject(getProfilePropertyJson, profilePropertyKey, profilePropertyValue, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        //--
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError;
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::updateProfileProperty(const char *profilePropertyKey, const char *profilePropertyValue)
{
    int status = 0;
    StringBuffer updateProfilePropertyRequestUrl;
    URL requestUrl;
    char* profilePropertyJson = NULL; // JSON object to send
    StringOutputStream response;
    //ESMPStatus parserStatus;
    
    // Format json
    if (jsonMHMediaObjectParser->formatProfilePropertyJson(profilePropertyKey, profilePropertyValue, &profilePropertyJson) == false) {
        LOG.error("%s: error formatting json object for property", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    updateProfilePropertyRequestUrl.sprintf(setProfilePropertyUrl, serverUrl.c_str());
    requestUrl.setURL(updateProfilePropertyRequestUrl);
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, profilePropertyJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    free(profilePropertyJson);
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::registerPayment(const char *receipt, const char *productIdentifier)
{
    int status = 0;
    StringBuffer registerPaymentRequestUrl;
    URL requestUrl;
    StringOutputStream response;
    //ESMPStatus parserStatus;
    
    StringBuffer encoded;
    b64_encode(encoded, receipt, strlen(receipt));
    
    registerPaymentRequestUrl.sprintf(registerPaymentUrl, serverUrl.c_str());
    requestUrl.setURL(registerPaymentRequestUrl);
    
    StringBuffer body ("transactionid=");
    body.append(encoded);
    body.append("&name=");
    body.append(productIdentifier);
    
    char *bodyc = const_cast<char*> ( body.c_str() );
    
    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, bodyc, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    const char *string = response.getString().c_str();
    
    if (string != NULL && strlen(string) > 0)
    {
        return ESMGeneralSubscriptionsError;
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::updateUserProfile(SapiUserProfile &userProfile)
{
    int status = 0;
    StringBuffer getUserProfileRequestUrl;
    URL requestUrl;
    char* userProfileJson = NULL; // JSON object from server
    StringOutputStream response;
    //ESMPStatus parserStatus;
    
    // Format json
    if (jsonMHMediaObjectParser->formatUserProfileJson(userProfile, &userProfileJson) == false) {
        LOG.error("%s: error formatting json object for user profile", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    getUserProfileRequestUrl.sprintf(putUserProfileUrl, serverUrl.c_str());
    requestUrl.setURL(getUserProfileRequestUrl);
    
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, userProfileJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    free(userProfileJson);
    
    return ESMRSuccess;
}

//
// authentication methods
//
EMHMediaRequestStatus MHMediaRequestManager::login(const char* device_id, time_t* serverTime, unsigned long * expiretime, StringMap* sourcesStringMap, StringMap* propertyStringMap)
{
    return ESMRAccessDenied;
}

EMHMediaRequestStatus MHMediaRequestManager::login(time_t* serverTime)
{
    return ESMRAccessDenied;
}

EMHMediaRequestStatus MHMediaRequestManager::sendStatuReport(SapiStatusReport* statusReport){
    int status = 0;
    StringBuffer statusReportUrl;
    char* statusReportJson;     // formatted JSON object with Status Report    
    URL requestUrl;
    StringOutputStream response;
    
    
    if (jsonMHMediaObjectParser->formatStatusReport(statusReport, &statusReportJson) == false) {
        LOG.error("%s: error formatting json object for items list", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    statusReportUrl.sprintf(sendStatusReportFmt, serverUrl.c_str());
    
    requestUrl.setURL(statusReportUrl);
    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, statusReportJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending upload request", __FUNCTION__);
        free(statusReportJson);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    free(statusReportJson);
    
    const char* result = NULL;
    
    if ((result = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid empty response for sapi item metadata upload", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: sapi delete item metadata request response = %s", __FUNCTION__, result);
    
    
    if (jsonMHMediaObjectParser->checkErrorMessages(result) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();

        if (errorCode != NULL && !strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
        }
        
        LOG.error("%s: SAPI error %s: %s", __FUNCTION__, errorCode, errorMsg);
        /*
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHUnknownMediaException)) {
            return ESMRUnknownMediaException;
        } else if (!strcmp(errorCode, MHInvalidContentRange)) {
            return ESMRInvalidContentRange;
        } else if (!strcmp(errorCode, MHMissingRequiredParameter)) {
            return ESMRMissingRequiredParameter;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHUserQuotaReached)) {
            return ESMRQuotaExceeded;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        }
        
        return ESMRMHMessageParseError;*/
    }
    
    
    return ESMRSuccess;

}

//Push notifications

EMHMediaRequestStatus MHMediaRequestManager::registerPushDeviceToken(const char* deviceToken)
{
    int status = 0;
    StringBuffer registerPushDeviceTokenUrl;
    char* deviceTokenJson;     // formatted JSON object with Device Token
    URL requestUrl;
    StringOutputStream response;
    
    if (jsonMHMediaObjectParser->formatPushDeviceToken(deviceToken, &deviceTokenJson) == false) {
        LOG.error("%s: error formatting json object for device token", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    // Urlencode the deviceId parameter (may contain unacceptable chars)
    const char* deviceIdEncoded = URL::urlEncode(deviceID);
    registerPushDeviceTokenUrl.sprintf(registerAPNsDeviceToken, serverUrl.c_str(), deviceIdEncoded);
    requestUrl.setURL(registerPushDeviceTokenUrl);
    free((void *)deviceIdEncoded);
    
    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, deviceTokenJson, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi request", __FUNCTION__);
        
        free(deviceTokenJson);
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    free(deviceTokenJson);
    
    const char* result = NULL;
    
    if ((result = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid empty response for sapi item metadata upload", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: sapi register push device token response = %s", __FUNCTION__, result);
    
    if (jsonMHMediaObjectParser->checkErrorMessages(result) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        LOG.error("%s: SAPI error %s: %s", __FUNCTION__, errorCode, errorMsg);

        if (errorCode != NULL && !strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
        }
    }
    
    return ESMRSuccess;
}

EMHMediaRequestStatus MHMediaRequestManager::logout()
{
    int status = 0;
    StringOutputStream response;            // sapi response buffer
    StringBuffer logoutUrl;
    URL requestUrl;
    const char* MHLogoutResponse = NULL;
    
    logoutUrl.sprintf(MHLogoutUrlFmt, serverUrl.c_str());
    
    requestUrl.setURL(logoutUrl.c_str());
    
    httpConnection->setKeepAlive(false);
    httpConnection->setRequestHeader(HTTP_HEADER_ACCEPT,      "*/*");
    
    setRequestSessionId();
    setRequestAuthentication();
    
    if ((status = httpConnection->open(requestUrl, HttpConnection::MethodGet)) != 0) {
        LOG.error("%s: error opening connection", __FUNCTION__);
        
        return ESMRConnectionSetupError; // malformed URI, etc
    }
    
    if ((status = httpConnection->request(NULL, response)) != HTTP_OK) {
        httpConnection->close();
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    httpConnection->close();
        
    if ((MHLogoutResponse = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi logout response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    config->setSessionId("");
    config->setSessionIdSetTime(0);
  
    LOG.debug("response returned = %s", MHLogoutResponse);
    
    return ESMRSuccess;
}


void MHMediaRequestManager::setRequestSessionId()
{
    if (httpConnection) {
        if (!sessionID.empty()) {
            // If another thread is performing the login, we shall wait for it to complete
            pthread_mutex_lock(&sessionIDAccessMutex);

            StringBuffer sessionIdCookie;
            sessionIdCookie.sprintf("JSESSIONID=%s", sessionID.c_str());
            httpConnection->setRequestHeader(HTTP_HEADER_COOKIE, sessionIdCookie.c_str());

            pthread_mutex_unlock(&sessionIDAccessMutex);
        }
    }
}

void MHMediaRequestManager::setRequestAuthentication()
{
    // request auth header depends on the type of authentication
    // see OAuth2MediaRequestManager / SapiMediaRequestManager
}

bool MHMediaRequestManager::addValidationKey(URL& url, const bool logOriginalRequest)
{
    if (!validateRequest) {
        return false;
    }
    if (validationKey.empty()) {
        LOG.debug("[%s] not using validation key: empty key", __FUNCTION__);
        return false;
    }

    std::string fullUrl = url.fullURL;
    if (fullUrl.empty()) {
        return false;
    }

    if (fullUrl.find("&validationkey") == std::string::npos) {
        if (logOriginalRequest) {
            LOG.debug("[%s] append validation key to request: %s", __FUNCTION__, fullUrl.c_str());
        }
        fullUrl.append("&validationkey=").append(validationKey.c_str());
    }

    url.setURL(fullUrl.c_str());
    return true;
}

void MHMediaRequestManager::resetValidationKey()
{
    LOG.debug("[%s] resetting validation key", __FUNCTION__);

    validationKey = "";
    config->setValidationKey("");
}

void MHMediaRequestManager::setRequestTimeout(const int timeout) {
    httpConnection->setRequestTimeout(timeout);
}

void MHMediaRequestManager::setResponseTimeout(const int timeout) {
    httpConnection->setResponseTimeout(timeout);
}

void MHMediaRequestManager::setUploadChunkSize(const int size) {
    httpConnection->setRequestChunkSize(size);
}

void MHMediaRequestManager::setDownloadChunkSize(const int size) {
    httpConnection->setResponseChunkSize(size);
}

void MHMediaRequestManager::setSessionID(const char* sessionId)
{
    if ((sessionId != NULL) && (strlen(sessionId) > 0)) {
        sessionID = sessionId;
    }
}

EMHMediaRequestStatus MHMediaRequestManager::getExternalServicesAlbum(CacheItemsList& albumList, const char* serviceName)
{
    int status = 0;
    StringBuffer externalServicesAlbumRequestUrl;
    URL requestUrl;
    const char* externalServicesAlbumJson = NULL; // JSON object from server
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    if ((serviceName == NULL) || (strcmp(serviceName, "") == true)) {
        LOG.error("%s: service name cannot be null", __FUNCTION__);
        return ESMRGenericError;
    
    }
    
    externalServicesAlbumRequestUrl.sprintf(getExternalServiceAlbumsFmt, serverUrl.c_str(), serviceName);
    
    requestUrl.setURL(externalServicesAlbumRequestUrl);

    httpConnection->setRequestHeader(HTTP_HEADER_ACCEPT, "*/*");
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response)) != HTTP_OK) {
        LOG.error("%s: error sending sapi media count request", __FUNCTION__);
        httpConnection->close();
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    httpConnection->close();
    
    if ((externalServicesAlbumJson = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid sapi media count response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s, response returned = %s", __FUNCTION__, externalServicesAlbumJson);
    
    if (jsonMHMediaObjectParser->parseExternalServicesAlbumListObject(externalServicesAlbumJson,  albumList, serviceName, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::deleteItem(const char* itemId)
{
    int status = 0;
    StringBuffer itemsListRequestUrl;
    char* itemsIdsListJsonObject;     // formatted JSON object with items ids for delete    
    URL requestUrl;
    StringOutputStream response;
    
    if (mhMediaSourceName == NULL) {
        return ESMRInternalError;
    }
    
    if ((itemId == NULL) && (strlen(itemId) == 0)) {
        LOG.error("%s: no guid found in item info", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
        
    if (itemsArrayKey == NULL) {
        LOG.error("%s: source name not found", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    StringBuffer s(itemId);
    ArrayList itemsIDs; 
    itemsIDs.add(s);
    
    if (jsonMHMediaObjectParser->formatDelItemsListObject(itemsIDs, &itemsIdsListJsonObject, itemsArrayKey, true) == false) {
        LOG.error("%s: error formatting json object for items list", __FUNCTION__);
        
        return ESMRInvalidParam;
    }
    
    itemsListRequestUrl.sprintf(deleteItemData, serverUrl.c_str(), mhMediaSourceName);
    
    requestUrl.setURL(itemsListRequestUrl);
    httpConnection->setRequestHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
    
    if ((status = performRequest(requestUrl, HttpConnection::MethodPost, itemsIdsListJsonObject, response)) != HTTP_OK) {
        LOG.error("%s: error sending upload request", __FUNCTION__);
        free(itemsIdsListJsonObject);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    free(itemsIdsListJsonObject);
    
    const char* result = NULL;
    
    if ((result = response.getString().c_str()) == NULL) {
        LOG.error("%s: invalid empty response for sapi item metadata upload", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    LOG.debug("%s: sapi delete item metadata request response = %s", __FUNCTION__, result);
    
    
    if (jsonMHMediaObjectParser->checkErrorMessages(result) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        LOG.error("%s: SAPI error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHUnknownMediaException)) {
            return ESMRUnknownMediaException;
        } else if (!strcmp(errorCode, MHInvalidContentRange)) {
            return ESMRInvalidContentRange;
        } else if (!strcmp(errorCode, MHMissingRequiredParameter)) {
            return ESMRMissingRequiredParameter;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHInvalidLUID)) {
            return ESMRInvalidLUID;
        } else if (!strcmp(errorCode, MHUserQuotaReached)) {
            return ESMRQuotaExceeded;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
                
        return ESMRMHMessageParseError;
    }
    
    
    return ESMRSuccess;
    
}


EMHMediaRequestStatus MHMediaRequestManager::getItemsMediaSet(URL& mediaSetUrl, 
                                                              const ArrayList& itemsIDs, 
                                                              const char* sourceName,
                                                              const char* description,
                                                              time_t* responseTime)
{
    int status = 0;
    StringBuffer itemsListRequestUrl;
    URL requestUrl;
    char* requestBody = NULL;                   // formatted JSON object with items ids
    const char* responseBody = NULL;            // sapi reponse with the url of the media set  
    StringOutputStream response;
    ESMPStatus parserStatus;
    mediaSetUrl.setURL("");
    
    if (itemsIDs.size() == 0) {
        LOG.error("%s: list of items id is empty", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    if (!sourceName || !strlen(sourceName)) {
        LOG.error("%s: missing source name param", __FUNCTION__);
        return ESMRInvalidParam;
    }
    
    //
    // format the json body
    //
    if (jsonMHMediaObjectParser->formatMediaSetItemsListObject(itemsIDs, &requestBody, 
                                                               description, sourceName) == false) {
        LOG.error("%s: error formatting json object", __FUNCTION__);
        return ESMRInvalidParam;
    }

    //
    // Send the http request
    //
    itemsListRequestUrl.sprintf(getMediaSetFmt, serverUrl.c_str());
    requestUrl.setURL(itemsListRequestUrl);
    
    status = performRequest(requestUrl, HttpConnection::MethodPost, requestBody, response);
    free(requestBody);
    
    if (status != HTTP_OK) {
        LOG.error("%s: error sending sapi create-media-set request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);

        return res;
    }
    
    if ((responseBody = response.getString().c_str()) == NULL) {
        LOG.error("%s: NULL sapi response", __FUNCTION__);
        return ESMRMHInvalidResponse;
    }
    
    //
    // parse the response
    //
    LOG.debug("response returned = %s", responseBody);
    
    StringBuffer successMessage, url;   // unused
    long mediaSetId = 0;                // unused
    
    if (jsonMHMediaObjectParser->parseMediaSetResponse(responseBody, successMessage, &mediaSetId, 
                                                       url, responseTime, &parserStatus) == false)
    {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
    
    mediaSetUrl.setURL(url.c_str());
    
    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::getRemoteLabels(const char*type, std::map<MHLabelInfo*,std::vector<uint32_t> >& labelsMap) {
    int status = 0;
    StringBuffer getLabelsRequestUrl;
    URL requestUrl;
    const char* responseBody = NULL;            // sapi reponse with the url of the media set
    StringOutputStream response;
    ESMPStatus parserStatus;
    
    cJSON *body= NULL;
    if ((body= cJSON_CreateObject()) == NULL) {
        LOG.error("error creating JSON object");
    }

    cJSON* emptyData = cJSON_CreateObject();
    cJSON_AddItemToObject(body, "data", emptyData);
    
    char* requestBody = cJSON_Print(body);
    
    //
    // Send the http request
    //
    getLabelsRequestUrl.sprintf(getLabelsFmt, serverUrl.c_str());
    if (type != NULL) {
        getLabelsRequestUrl.append("&type=").append(type);
    }
    requestUrl.setURL(getLabelsRequestUrl);
    
    status = performRequest(requestUrl, HttpConnection::MethodPost, requestBody, response);
    free(requestBody);
    cJSON_Delete(body);
    
    if (status != HTTP_OK) {
        LOG.error("%s: error sending get labels request", __FUNCTION__);
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);
        return res;
    }
    
    if ((responseBody = response.getString().c_str()) == NULL) {
        LOG.error("%s: NULL sapi response", __FUNCTION__);
        return ESMRMHInvalidResponse;
    }
    
    //
    // parse the response
    //
    LOG.debug("response returned = %s", responseBody);
    
    StringBuffer successMessage, url;   // unused
    long mediaSetId = 0;                // unused

    if (jsonMHMediaObjectParser->parseGetLabels(responseBody, &parserStatus, labelsMap) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError;
    }
    
    return ESMRSuccess;
}


EMHMediaRequestStatus MHMediaRequestManager::getServerInfo(ServerInfo& serverInfo)
{
    int status = 0;
    StringBuffer serverInfoRequestUrl;
    URL requestUrl;
    char* requestBody = NULL;                   // formatted JSON object with items ids
    const char* responseBody = NULL;            // sapi reponse with the url of the media set  
    StringOutputStream response;
    ESMPStatus parserStatus = ESMPNoError;
    
    //
    // format and send the http request
    //
    serverInfoRequestUrl.sprintf(serverInfoUrlFmt, serverUrl.c_str());
    requestUrl.setURL(serverInfoRequestUrl);
    
    status = performRequest(requestUrl, HttpConnection::MethodGet, NULL, response);
    
    if (status != HTTP_OK) {
        LOG.error("%s: error sending server info request", __FUNCTION__);
        
        // Handle special status here if needed
        EMHMediaRequestStatus res = handleHttpError(status);

        return res;
    }
    
    if ((responseBody = response.getString().c_str()) == NULL) {
        LOG.error("%s: NULL sapi response", __FUNCTION__);
        
        return ESMRMHInvalidResponse;
    }
    
    //
    // parse the response
    //
    LOG.debug("%s: response returned = %s", __FUNCTION__, responseBody);
    
    if (jsonMHMediaObjectParser->parseServerInfoJson(responseBody, serverInfo, &parserStatus) == false) {
        const char* errorCode = jsonMHMediaObjectParser->getErrorCode().c_str();
        const char* errorMsg  = jsonMHMediaObjectParser->getErrorMessage().c_str();
        
        if (!errorCode || !errorMsg) {
            LOG.error("%s: error parsing sapi return message", __FUNCTION__);
            switch (parserStatus) {
                case ESMPKeyMissing:
                    // is some required field of JSON object has not been found
                    // consider the sapi on server as not supported 
                    return ESMRMHNotSupported;
                    
                default:
                    return ESMRMHMessageParseError;
            }
        }
        
        LOG.error("%s: sapi error %s: %s", __FUNCTION__, errorCode, errorMsg);
        
        if (!strcmp(errorCode, MHSecurityException)) {               
            return ESMRSecurityException;
        } else if (!strcmp(errorCode, MHUserIdMissing)) {
            return ESMRUserIdMissing;
        } else if (!strcmp(errorCode, MHSessionAlreadyOpen)) {             
            return ESMRSessionAlreadyOpen;
        } else if (!strcmp(errorCode, MHInvalidAuthSchema)) {
            return ESMRInvalidAuthSchema;
        } else if (!strcmp(errorCode, MHPictureIdUsedWithLimit)) {
            return ESMRPictureIdUsedWithLimit;
        } else if (!strcmp(errorCode, MHUnableToRetrievePicture)) {
            return ESMRUnableToRetrievePicture;
        } else if (!strcmp(errorCode, MHInvalidDataTypeOrFormat)) {
            return ESMRInvalidDataTypeOrFormat;
        } else if (!strcmp(errorCode, MHOperationNotSupported)) {
            return ESMRMHNotSupported;
        } else if (!strcmp(errorCode, MHInvalidValidationKey)) {
            resetValidationKey();
            return ESMRInvalidValidationKey;
        }
        
        return ESMRMHMessageParseError; 
    }
        
    return ESMRSuccess;
}



int MHMediaRequestManager::performRequest(const URL& url, HttpConnection::RequestMethod method,
                                          char* body, OutputStream& response, bool logRequest, bool useAuthentication) {
    return performRequest(url, method, body, NULL, response, logRequest, useAuthentication);
}

int MHMediaRequestManager::performRequest(const URL& url, HttpConnection::RequestMethod method,
                                          char* body, InputStream* bodyStream,
                                          OutputStream& response, bool logRequest, bool useAuthentication) {
    int httpStatus;
    
    lastHttpResponseHeaders.clear();
    
    pthread_mutex_lock(&sessionIDAccessMutex);
    StringBuffer configSessionId = config->getSessionId();
    validationKey = config->getValidationKey();
    
    // reset SessionID if expirationTime elapsed (will cause a 401 and relogin)
    if (!configSessionId.empty()) {
        time_t now = time(NULL);
        time_t sessionIdSavedTime = config->getSessionIdSetTime();
        time_t sessionIdExpirationTime = config->getSessionIdExpirationTime();
        
        if ((sessionIdSavedTime + sessionIdExpirationTime) > now) {
            sessionID = configSessionId.c_str();
        } else {
            LOG.info("%d hours elapsed: resetting SessionID & validation key", (int)(sessionIdExpirationTime/3600));
            sessionID.reset();
            validationKey.reset();
        }
    }

    // reset SessionID if validation required and validation key is missing (will cause a 401 and relogin)
    if (validateRequest && validationKey.empty()) {
        LOG.info("Validation required but validation key is empty: resetting SessionID");
        sessionID.reset();
    }

    
    // Save the sessionID being used for this request
    StringBuffer requestSessionId(sessionID.c_str());
    pthread_mutex_unlock(&sessionIDAccessMutex);
    
    setRequestSessionId();
    if (useAuthentication) {
        // not all requests require authentication (e.g. download item which is not a SAPI)
        setRequestAuthentication();
    }
    httpConnection->setKeepAlive(false);
    httpConnection->setRequestHeader(HTTP_HEADER_ACCEPT, "*/*");
    httpConnection->setRequestHeader(HTTP_HEADER_X_DEVICE_ID, deviceID);

    const char* fullUrl = url.fullURL;
    URL requestUrl(fullUrl);

    if (useAuthentication && addValidationKey(requestUrl, logRequest)) {
        logRequest = false;     // it contains the validation key from now on: don't want to log it
    }

    if ((httpStatus = httpConnection->open(requestUrl, method, logRequest)) != 0) {
        LOG.error("%s: error opening connection", __FUNCTION__);
        httpConnection->close();
        return HTTP_ERROR; // malformed URI, etc
    }

	// backup info in case this request fails. For the 2nd try (after relogin)
	StringMap backupRequestHeaders = httpConnection->getRequestHeaders();
	int64_t backupStreamPosition = bodyStream ? bodyStream->getPosition() : 0;
    
    if (bodyStream != NULL) {
        httpStatus = httpConnection->request(*bodyStream, response, logRequest);
    } else {
        httpStatus = httpConnection->request(body, response, logRequest);
    }

    // always parse response headers
    lastHttpResponseHeaders = httpConnection->getResponseHeaders();
    processResponseHeaders();
    
    int status = -1;
    if (httpStatus == HTTP_UNAUTHORIZED) {
        // The session may have expired or the user not logged in. Perform a login at this
        // point and perform the request again
        httpConnection->close();
   
        // Perform the login in a mutex section
        pthread_mutex_lock(&sessionIDAccessMutex);
        
        LOG.debug("sessionID=%s",sessionID.c_str());
        LOG.debug("requestSessionID=%s",requestSessionId.c_str());
        
        // We must make sure someone else has not already performed a login
        if (sessionID.empty() || requestSessionId.icmp(sessionID)) {
            time_t serverTime;
            unsigned long expireTime;
            status = login(deviceID, &serverTime, &expireTime, NULL, NULL);
            if (status == ESMRSuccess) {
                httpConnection->close();
                //delete httpConnection;
                //httpConnection = new HttpConnection(config->getUserAgent());
            } else {
                sessionID.reset();
            }
        } else {
            status = ESMRSuccess;
        }
        pthread_mutex_unlock(&sessionIDAccessMutex);
        
        if (status == ESMRSuccess) {
            // If login is successfull, retry the request (restore backed-up headers)
			httpConnection->setRequestHeaders(backupRequestHeaders);
            setRequestSessionId();
            if (useAuthentication) {
                // not all requests require authentication (e.g. download item which is not a SAPI)
                setRequestAuthentication();
            }
            httpConnection->setKeepAlive(false);
            requestUrl = fullUrl;
            if (useAuthentication && addValidationKey(requestUrl, logRequest)) {
                logRequest = false;     // it contains the validation key from now on: don't want to log it
            }
                
            if ((status = httpConnection->open(requestUrl, method, logRequest)) != 0) {
                LOG.error("%s: error opening connection", __FUNCTION__);
                httpConnection->close();
                return ESMRConnectionSetupError; // malformed URI, etc
            }
            response.reset();
            if (bodyStream != NULL) {
				bodyStream->setPosition(backupStreamPosition);
                httpStatus = httpConnection->request(*bodyStream, response, logRequest);
            } else {
                httpStatus = httpConnection->request(body, response, logRequest);
            }
        } else {
            if (status == ESMRPaymentRequired) {                
                httpStatus = HTTP_PAYMENT_REQUIRED;
            } else {
                switch (status) {
                    case ESMRNetworkError:
                    case ESMRErrorCodeLoginFailure750: // custom login error codes
                    case ESMRErrorCodeLoginFailure751:
                    case ESMRErrorCodeLoginFailure752:
                    case ESMRErrorCodeLoginFailure753:
                    case ESMRErrorCodeLoginFailure754:
                    case ESMRErrorCodeLoginFailure755:
                    case ESMRErrorCodeLoginFailure756:
                    case ESMRErrorCodeLoginFailure757:
                        httpStatus = status;
                        break;
                 
                    default:
                        httpStatus = HTTP_UNAUTHORIZED;
                        break;
                }
            }
        }
    }
    
    httpConnection->close();
    return httpStatus;
}

int MHMediaRequestManager::performRequest(const URL& url, HttpConnection::RequestMethod method,
                                          InputStream& body, OutputStream& response, bool logRequest, bool useValidationKey) {
    
    return performRequest(url, method, NULL, &body, response, logRequest, useValidationKey);
}

EMHMediaRequestStatus MHMediaRequestManager::handleHttpError(int httpCode) {
    switch (httpCode) {
        case HTTP_UNAUTHORIZED:
            return ESMRAccessDenied;
            
        case HTTP_FORBIDDEN:
            return ESMRForbidden;
            
        case HTTP_PAYMENT_REQUIRED:
            return ESMRPaymentRequired;
            
        case HTTP_FUNCTIONALITY_NOT_SUPPORTED:
        case HTTP_NOT_FOUND:
            return ESMRHTTPFunctionalityNotSupported;
        case HTTP_PROXY_UNAUTHORIZED:
            return ESMRProxyAuthenticationRequired;

        case HttpConnection::StatusNetworkError:
        case HttpConnection::StatusReadingError:
        case HttpConnection::StatusWritingError:
            return ESMRNetworkError;
            
        case HttpConnection::StatusTimeoutError:
            return ESMRRequestTimeout;
      
        case HttpConnection::StatusCancelledByUser:
                return ESMROperationCanceled;
            
        case HttpConnection::StatusStreamWritingError:
            return ESMRCannotWriteResponse;

		case HttpConnection::StatusStreamReadingError:
			return ESMRCannotReadInputStream;
       
        case ESMRErrorCodeLoginFailure750: // custom login error codes
        case ESMRErrorCodeLoginFailure751:
        case ESMRErrorCodeLoginFailure752:
        case ESMRErrorCodeLoginFailure753:
        case ESMRErrorCodeLoginFailure754:
        case ESMRErrorCodeLoginFailure755:
        case ESMRErrorCodeLoginFailure756:
        case ESMRErrorCodeLoginFailure757:
            return static_cast<EMHMediaRequestStatus>(httpCode);
            
        default:
            break;
    }
    return ESMRGenericHttpError;
}


void MHMediaRequestManager::processResponseHeaders()
{
}

void MHMediaRequestManager::setUploadItemExtendedHeaders(UploadMHSyncItem* item) {
}
END_FUNAMBOL_NAMESPACE
