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

#ifndef __MH_MEDIA_REQUEST_MANAGER_H__
#define __MH_MEDIA_REQUEST_MANAGER_H__

#include <vector>
#include <map>

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/globalsdef.h"
#include "base/util/StringBuffer.h"
#include "base/util/StringMap.h"
#include "spds/AbstractSyncConfig.h"
#include "MediaHub/UploadMHSyncItem.h"
#include "MediaHub/DownloadMHSyncItem.h"
#include "MediaHub/MHStore.h"
#include "MediaHub/MHLabelInfo.h"
#include "ioStream/OutputStream.h"
#include "http/HttpConnection.h"
#include "http/AbstractHttpConnection.h"
#include "http/URL.h"
#include <pthread.h>
#include "sapi/SapiUserProfile.h"
#include "base/util/StringMap.h"
#include "MediaHub/MHMediaJsonParser.h"
#include "MediaHub/MHItemJsonParser.h"
#include "sapi/SapiSubscription.h"
#include "sapi/SapiPayment.h"
#include "sapi/SapiStatusReport.h"
#include "client/ServerLoginInfo.h"

BEGIN_FUNAMBOL_NAMESPACE

typedef enum EMHMediaRequestStatus //Enumeration Sapi Media Request Managers
{
    ESMRSuccess = 0,
    ESMRConnectionSetupError,               // Error setting up the connection
    ESMRAccessDenied,                   
    ESMRGenericHttpError,                   // Http error
    ESMRMHInvalidResponse,                  // Bad MH response received
    ESMRMHMessageParseError,                // Error occurred parsing the JSON body received
    ESMRMHMessageFormatError,               // Error occurred formatting the JSON body to send
    ESMRInvalidParam,                       // An invalid parameter is passed
    ESMRNetworkError,                       // Network error 
    ESMRRequestTimeout,                         
    ESMRHTTPFunctionalityNotSupported,      // 501 error from the server (check fields on server not supporting this MH)
    ESMRMHNotSupported,                     // MH incompatible version on server (missing needed parameters in json responses, etc..)
    ESMRMHIllicitContent,                   // MED-1016, illicit media content
    ESMRMHNotValidatedContent,              // MED-1017, not validated media content
    ESMRPaymentRequired,                    // 402 payment required
    ESMRForbidden,                          // 403 forbidden, maybe the url to download image has expired
    
    // MH request failure codes
    ESMRSecurityException,
    ESMRUserIdMissing,
    ESMRSessionAlreadyOpen,
    ESMRInvalidValidationKey,
    ESMRInvalidAuthSchema,
    ESMGeneralSubscriptionsError,

    ESMRUnknownMediaException, 
    ESMRMediaIdWithLimit,
    ESMRErrorRetrievingMediaItem,
    ESMRInvalidContentRange,

    ESMRPictureGenericError,
    ESMRNoPicturesSpecfied,
    ESMRPictureIdUsedWithLimit,
    ESMRUnableToRetrievePicture,
    ESMRQuotaExceeded,
    ESMRFileTooLarge,

    ESMRInvalidDataTypeOrFormat,
    ESMRInvalidFileName,
    ESMRMissingRequiredParameter,
    ESMRInvalidEmptyDataParameter,
    ESMRInvalidLUID,
    ESMRInvalidDeviceId,
    
    // generic error
    ESMRInternalError,                       // internal data consistency error
    ESMROperationCanceled,                   // operation cancelled on user request
    ESMRGenericError,
    
    // Error writing response
    ESMRCannotWriteResponse,

	// Error reading input stream
	ESMRCannotReadInputStream,

    // proxy auth required. Mostly handled in windows client
    ESMRProxyAuthenticationRequired,

    // leave this values for login values
    ESMRErrorCodeLoginFailure750 = 750,
    ESMRErrorCodeLoginFailure751 = 751,
    ESMRErrorCodeLoginFailure752 = 752,
    ESMRErrorCodeLoginFailure753 = 753,
    ESMRErrorCodeLoginFailure754 = 754,
    ESMRErrorCodeLoginFailure755 = 755,
    ESMRErrorCodeLoginFailure756 = 756,
    ESMRErrorCodeLoginFailure757 = 757
} ESMRStatus;

class HttpConnection;
class MHMediaJsonParser;
class HttpAuthentication;
class ExportedItem;

class MHMediaRequestManager
{
    public:
        // sapi query parameters
        static const char* sortByCreationDateToken;
        static const char* sortByModificationDateToken;
 
        // sapi auth error codes
        static const char* MHSecurityException;
        static const char* MHUserIdMissing;
        static const char* MHSessionAlreadyOpen;
        static const char* MHInvalidValidationKey;
        static const char* MHInvalidAuthSchema;

 
    protected:
        StringBuffer serverUrl;             
 
        MHMediaJsonParser* jsonMHMediaObjectParser;
        MHItemJsonParser*  jsonMHItemObjectParser;
        HttpConnection* httpConnection;
        HttpAuthentication* auth;
        StringMap lastHttpResponseHeaders;
        AbstractSyncConfig* config;
        
        const char* mhMediaSourceName;
        const char* itemsArrayKey;
        const char* orderField;
        const char* deviceID;
        const char* credInfo;
    
        
        /* The session ID is unique for all instances, because there is a single session kept open
           with the server
         */
        static pthread_mutex_t sessionIDAccessMutex;
        static bool sessionIDAccessMutexInitialized;
        static StringBuffer sessionID;
        static StringBuffer validationKey;

        // if true, adds the validation key in all SAPI requests URL. 
        // Set to false to disable validation of SAPI calls.
        static bool validateRequest;
       
        
    public:
        MHMediaRequestManager(const char* url, const char* sapiSourceUri, const char* sapiArrayKey,
                              const char* orderFieldValue, MHItemJsonParser* itemParser,
                              AbstractSyncConfig* config);
       
        MHMediaRequestManager(const char* url, AbstractSyncConfig* config); 
       
        virtual ~MHMediaRequestManager();

        virtual EMHMediaRequestStatus getLastSyncTimestamp(const char* remoteUri,
                                                           unsigned long *timestamp);
        virtual EMHMediaRequestStatus getItemsCount(int *count);
        virtual EMHMediaRequestStatus getAllItems(CacheItemsList& MHItemInfoList, 
                                                  CacheLabelsMap& labelsMap,
                                                  time_t* responseTime,
                                                  int limit = 0, int offset = 0);
        
        virtual EMHMediaRequestStatus getItemsFromId(CacheItemsList& items, CacheLabelsMap& labels, const ArrayList& itemsIDs);
        virtual EMHMediaRequestStatus getItemsChanges(ArrayList& newIDs,
                                                      ArrayList& modIDs, 
                                                      ArrayList& delIDs, 
                                                      const StringBuffer& fromDate,
                                                      time_t* requestTimestamp);
    
        virtual EMHMediaRequestStatus getItemsChanges(ArrayList& newIDs,
                                                      ArrayList& modIDs, 
                                                      ArrayList& delIDs,
                                                      const StringBuffer& fromDate,
                                                      time_t* reponseTimestamp,
                                                      const StringBuffer& username,
                                                      const StringBuffer& password);
    
        // @param includingDeletes  if false, do not consider Deletes "D" as pending changes
        virtual EMHMediaRequestStatus getSourcesWithPendingChanges(ArrayList& sourcesToCheck,
                                                                   const StringBuffer& fromDate, 
                                                                   time_t* reponseTimestamp,
                                                                   ArrayList& sourcesWithPendingChanges,
                                                                   bool includingDeletes = true);
    
        virtual EMHMediaRequestStatus downloadItem(DownloadMHSyncItem* item);
        virtual EMHMediaRequestStatus getItemResumeInfo(UploadMHSyncItem* item, int64_t* offset);
        virtual EMHMediaRequestStatus getQuotaInfo(int64_t* free,uint64_t* quota);
        
        // item upload/update methods
        virtual EMHMediaRequestStatus uploadItemMetaData(UploadMHSyncItem* item, bool updateMetaDataOnly = false);
        virtual EMHMediaRequestStatus uploadItemData(UploadMHSyncItem* item, time_t* lastUpdate,
                                                     bool isUpdate = false,
                                                     HttpConnectionUploadObserver* uploadObserver = NULL,
                                                     MHSyncItemInfo* responseItem = NULL);
    
        virtual EMHMediaRequestStatus deleteItem(const char* itemId);

        // MH session set up methods
        virtual EMHMediaRequestStatus registerPayment(const char *receipt, const char *productIdentifier);
        virtual EMHMediaRequestStatus getCurrentSubscription(SapiSubscription& currentSubscription);
        virtual EMHMediaRequestStatus getSubscriptionByName(const char *subscriptionName, SapiSubscription& subscription);
        virtual EMHMediaRequestStatus getSubscriptions(std::vector<SapiSubscription*>& subscriptions, bool useIAP = true);
        virtual EMHMediaRequestStatus purchaseSubscription(const char* subscription);
        virtual EMHMediaRequestStatus cancelCurrentSubscription();
        virtual EMHMediaRequestStatus getSubscriptionPayments(std::vector<SapiPayment *>& payments);
        virtual EMHMediaRequestStatus getUserProfile(SapiUserProfile& userProfile);
        virtual EMHMediaRequestStatus updateUserProfile(SapiUserProfile& userProfile);
    
        virtual EMHMediaRequestStatus getProfileProperty(const char *profilePropertyKey, char *profilePropertyValue);
        virtual EMHMediaRequestStatus updateProfileProperty(const char *profilePropertyKey, const char *profilePropertyValue);
    
        virtual EMHMediaRequestStatus login(const char* device_id, time_t* serverTime = NULL, unsigned long *expiretime = NULL, StringMap* sourcesStringMap = NULL, StringMap* propertyStringMap = NULL);
    
        virtual EMHMediaRequestStatus login(time_t* serverTime = NULL);
    
        virtual EMHMediaRequestStatus logout();
    
        virtual EMHMediaRequestStatus sendStatuReport(SapiStatusReport* statusReport);
    
        // Push notifications
        virtual EMHMediaRequestStatus registerPushDeviceToken(const char* deviceToken);

        // external services methods
        virtual EMHMediaRequestStatus getExternalServices(CacheItemsList& servicesList,
                                                          bool isDesktop);
        virtual EMHMediaRequestStatus getExternalServicesAlbum(CacheItemsList& albumList, const char* serviceName);
        virtual EMHMediaRequestStatus exportItemToExternalService(std::vector<ExportedItem*>& exportedItem,
                                                                  const char* sourceUri,
                                                                  time_t* responseTime);
    
        virtual EMHMediaRequestStatus updateFacebookToken(const char* token, const char* accountName, long expTime);
    
        EMHMediaRequestStatus getItemsMediaSet(URL& mediaSetUrl, const ArrayList& itemsIDs, 
                                               const char* sourceName, const char* description, time_t* responseTime);


        virtual EMHMediaRequestStatus getServerInfo(ServerInfo& serverInfo);
    
        virtual EMHMediaRequestStatus getRemoteLabels(const char* type, std::map<MHLabelInfo*,std::vector<uint32_t> >& labelsMap);
    
        /**
         * Sets the timeout for the http requests (upload) into httpConnection.
         * @param timeout  the timeout, in seconds
         */
        void setRequestTimeout(const int timeout);

        /**
         * Sets the timeout for the http responses from the server (download) into httpConnection.
         * @param timeout  the timeout, in seconds
         */
        void setResponseTimeout(const int timeout);
        /**
         * Sets the upload http chunk size, for http requests into httpConnection.
         * @param size  the chunk size, in bytes
         */
        void setUploadChunkSize(const int size);

        /**
         * Sets the download http chunk size, for http responses into httpConnection.
         * @param size  the chunk size, in bytes
         */
        void setDownloadChunkSize(const int size);
        
        /**
         * Sets SAPI requests session ID (HTTP cookie) 
         */
        void setSessionID(const char* sessionId);
        
        /**
         * Returns the session ID stored during the first login call.
         */
        StringBuffer getSessionID() { return sessionID; }
    
        // Resets the session ID (HTTP cookie)
        static void resetSessionID() { sessionID.reset(); }
        
    protected:

        /**
         * Sets the JSESSIONID header to the http request.
         */
        void setRequestSessionId();

        /**
         * Sets the request authentication header depends on the type of authentication
         * see OAuth2MediaRequestManager / SapiMediaRequestManager)
         */
        virtual void setRequestAuthentication();

        virtual void processResponseHeaders();

        /**
         * Set extra headers that can be added by other MediaRequestManager implementation.
         */
        virtual void setUploadItemExtendedHeaders(UploadMHSyncItem* item);
    
        /**
         * Adds the validation key to the passed URL, appending a param like "&validationkey=5e603b274d3342476d384e262b562f59".
         * If 'validateRequest' is false or 'validationKey' is missing, nothing is done.
         *
         * @param url                 [IN-OUT] the url to be modified adding the validationkey parameter
         * @param logOriginalRequest  if true, logs the original http request (before adding the validation key)
         * @return                    true if the URL returned contains the validation key, false if not
         */
        bool addValidationKey(URL& url, const bool logOriginalRequest = false);

        /// Resets the validation key, usually in case of SAPI error SEC-1003 (invalid validation key).
        void resetValidationKey();

        int performRequest(const URL& url, HttpConnection::RequestMethod method,
                           char* body, OutputStream& response, bool logRequest=true, bool useAuthentication = true);
                           
        int performRequest(const URL& url, HttpConnection::RequestMethod method,
                           InputStream& body, OutputStream& response, bool logRequest=true, bool useAuthentication = true);
    
        int performRequest(const URL& url, HttpConnection::RequestMethod method,
                           char* body, InputStream* bodyStream,
                           OutputStream& response, bool logRequest, bool useAuthentication);
    
        EMHMediaRequestStatus handleHttpError(int httpCode);
        
};

END_FUNAMBOL_NAMESPACE

#endif
