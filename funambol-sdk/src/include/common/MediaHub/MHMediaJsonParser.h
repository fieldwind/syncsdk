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

#ifndef __MH_MEDIA_JSON_PARSER_H__
#define __MH_MEDIA_JSON_PARSER_H__

#include <vector>

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"
#include "MediaHub/MHSyncItemInfo.h"
#include "MediaHub/MHStore.h"
#include "MediaHub/MHJsonParser.h"
#include "base/util/ArrayList.h"
#include "cJSON.h"
#include "sapi/SapiUserProfile.h"
#include "sapi/SapiSubscription.h"
#include "sapi/SapiPayment.h"
#include "base/util/StringMap.h"
#include "sapi/SapiStatusReport.h"
#include "client/ServerLoginInfo.h"

BEGIN_FUNAMBOL_NAMESPACE


class MHMediaJsonParser
{
    private:
        StringBuffer errorCode;
        StringBuffer errorMessage;
 
    public:
        MHMediaJsonParser();
        virtual ~MHMediaJsonParser();

        bool parseLastSyncTimestampObject(const char* jsonObj,
                                          unsigned long *lastSyncTimestamp,
                                          const char* deviceId,
                                          const char* sourceUri,
                                          ESMPStatus* errCode);
        bool parseItemCountObject(const char* jsonObj, int* count, ESMPStatus* errCode);
        bool parseQuotaInfoObject(const char* quotaInfoJsonObject,int64_t* free,uint64_t* quota, ESMPStatus* errCode);
        

        bool parseLogin(const char* message, StringBuffer& sessionID, StringBuffer& validationKey, time_t* responseTime, ESMPStatus* errCode, unsigned long * expiretime, StringMap* sourcesStringMap, 
            StringMap* propertyStringMap, int* loginStatusCode); // new parseLogin with extra json parsing
    
        bool parseGetLabels(const char* message, ESMPStatus* errCode, std::map<MHLabelInfo*,std::vector<uint32_t> >& labelsMap);

        bool parseRestoreChargeResponse(const char* message, ESMPStatus* errCode); // parses the response from MHRestoreCharge


        bool parseExternalServicesListObject(const char* externalServicesJson, CacheItemsList&  externalServicesList, ESMPStatus* errCode); 
        bool parseExternalServicesAlbumListObject(const char* externalServicesAlbumJson, CacheItemsList&  albumList, const char* serviceName, ESMPStatus* errCode); 
    
        bool formatProfilePropertyKeyJson(const char *propertyKey, char **propertyKeyJson);
        bool formatProfilePropertyJson(const char *propertyKey, const char *propertyValue, char **propertyJson);
    
        bool formatUserProfileJson(SapiUserProfile& userProfile, char **userProfileJson);
        bool formatItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, bool prettyPrint=false);
        bool formatMediaItemMetaData(MHSyncItemInfo* itemInfo, char** itemJsonMetaData, bool metaDataOnlyUpdate, bool prettyPrint=false);
        bool formatJsonExportedItem(std::vector<ExportedItem*>& exportedItem, char** exportedItemJson, bool prettyPrint=false);
        bool formatMediaSetItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, 
                                           const char* itemDescription, const char* sourceName, bool prettyPrint=false);
    
        bool formatSaveSubscriptionObject(const char * itemsID, char **itemsListJsonObject, bool prettyPrint);
    
        bool formatStatusReport(SapiStatusReport* statusReport, char** statusReportJson);
    
        bool formatPushDeviceToken(const char* deviceToken, char** deviceTokenJson);
        
        bool parseMediaSetResponse(const char* responseBody, StringBuffer& mediaSetSuccess, 
                                   long* mediaSetId,         StringBuffer& mediaSetUrl,
                                   time_t* responseTime,     ESMPStatus* errCode);

        bool parseJsonExportedItemResponse(const char* jsonResponse, ESMPStatus* errCode, time_t* responseTime);

        bool parseProfilePropertyObject(const char* jsonResponse, const char* propertyKey, char* propertyValue, ESMPStatus* errCode);
    
        bool parseJsonCurrentSubscription(const char* jsonResponse, SapiSubscription& currentSubscription, ESMPStatus* errCode);
        bool parseJsonSubscriptionList(const char* jsonResponse, std::vector<SapiSubscription*>& subscriptions, ESMPStatus* errCode);
        bool parseJsonSubscriptionPaymentList(const char* jsonResponse, std::vector<SapiPayment*>& payments, ESMPStatus* errCode);
        bool parseJsonUserProfileObject(const char* jsonResponse, SapiUserProfile& userProfile, ESMPStatus* errCode);
        bool parseServerInfoJson(const char* jsonResponse, ServerInfo& serverLoginInfo, ESMPStatus* errCode);

        bool formatDelItemsListObject(const ArrayList& itemsIDs, char **itemsListJsonObject, 
                                      const char* sourceName, bool prettyPrint);
    
        bool checkErrorMessages(const char* response, time_t* lastUpdate = NULL);
        
        StringBuffer& getErrorCode()     { return errorCode;    }
        StringBuffer& getErrorMessage()  { return errorMessage; }
    
    private:
		int jsonObjArray2StringMap(cJSON* objJsonArray, StringMap* stringMap);  //converts from an array "name":"value" to string map
        ESMPStatus checkErrorMessage(cJSON* objRoot, bool* error);
        ESMPStatus parseCurrentSubscription(cJSON* subscriptionJson, SapiSubscription &subscription);
        ESMPStatus parseSubscription(cJSON* subscriptionJson, SapiSubscription& subscription);
        ESMPStatus parsePayment(cJSON* paymentJson, SapiPayment &payment);
        ESMPStatus parseExternalService(cJSON* externalServiceJson, ExternalService& service);
        ESMPStatus formatJsonArrayToStringBuffer(cJSON* itemsArray, StringBuffer &valuesString, const char* valueSeparator=",");  
};

END_FUNAMBOL_NAMESPACE

#endif
