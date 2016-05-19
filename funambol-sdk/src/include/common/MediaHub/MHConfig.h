/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2011 Funambol, Inc.
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



#ifndef INCL_MH_CONFIG
#define INCL_MH_CONFIG
/** @cond API */
/** @addtogroup Client */
/** @{ */

#include "spds/CustomConfig.h"
#include "base/globalsdef.h"
#include "base/util/StringMap.h"
#include "base/Log.h"

//
// MHConfig's properties
//
#define PROPERTY_REQUEST_TIMEOUT          "requestTimeout"
#define PROPERTY_RESPONSE_TIMEOUT         "responseTimeout"
#define PROPERTY_UPLOAD_CHUNK_SIZE        "uploadChunkSize"
#define PROPERTY_DOWNLOAD_CHUNK_SIZE      "downloadChunkSize"
#define PROPERTY_MAX_RETRIES_ON_ERROR     "maxRetriesOnError"
#define PROPERTY_SLEEP_TIME_ON_RETRY      "sleepMsecOnRetry"
#define PROPERTY_RESET_STREAM_ON_RETRY    "resetStreamOnRetry"
#define PROPERTY_MIN_DATA_SIZE_ON_RETRY   "minDataSizeOnRetry"
#define PROPERTY_IS_CARED_SERVER          "isCaredServer"

BEGIN_NAMESPACE

/**
 * This class groups general config properties related to the MH media sync.
 */
class MHConfig : public CustomConfig {
    
private:
    int requestTimeout;
    int responseTimeout;
    int uploadChunkSize;
    int downloadChunkSize;

public:

    /// Constructs a new MHConfig object
    MHConfig();

    /// Destructor
    ~MHConfig();


    /**
     * Sets the timeout for the http requests (upload).
     * @param timeout  the timeout, in seconds
     */
    void setRequestTimeout (const int timeout);

    /**
     * Sets the timeout for the http responses from the server (download).
     * @param timeout  the timeout, in seconds
     */
    void setResponseTimeout(const int timeout);

    /**
     * Sets the upload http chunk size, for http requests.
     * @param size  the chunk size, in bytes
     */
    void setUploadChunkSize(const int size);

    /**
     * Sets the download http chunk size, for http responses.
     * @param size  the chunk size, in bytes
     */
    void setDownloadChunkSize(const int size);

    /**
     * Sets the max number of retries in case of network error.
     * @param retries  the max number of retries
     */
    void setMaxRetriesOnError(const int retries);
    
    /**
     * Sets the sleep time in case of retry when a network error occurs.
     * @param msec  the sleep time, in milliSeconds
     */
    void setSleepTimeOnRetry(const long msec);

    /**
     * Sets flag for resetting data stream on uploads.
     * @param reset the boolean value of the flag
     */
    void setResetStreamOnRetry(bool reset);

    /**
     * Sets the minimum amount of data (in bytes) transferred to retry a connection.
     * If the data transferred is bigger than this size, the retry mechanism is reset.
     * @param size  the size in bytes
     */
    void setMinDataSizeOnRetry(const long size);
    
    /**
     * Sets a boolean to specify if the Server is a cared (supports MH) or not (comed).
     * It should be set after the first MH login, and it can be used as an
     * optimization by clients to avoid calling other MH if the server does not support them.
     */
    void setCaredServer(bool cared);

    int getRequestTimeout();
    int getResponseTimeout();
    int getUploadChunkSize();
    int getDownloadChunkSize();
    int getMaxRetriesOnError();
    long getSleepTimeOnRetry();
    bool getResetStreamOnRetry();
    long getMinDataSizeOnRetry();
    bool isCaredServer();

    /**
     * Initialize this object with the given MHConfig
     * @param sc the MHConfig object
     */
    void assign(const MHConfig& sc);

    /**
     * Assign operator
     */
    MHConfig& operator = (const MHConfig& sc) {
        assign(sc);
        return *this;
    }
    
    void setProperty(const char* propertyName, const char* propertyValue);

};


END_NAMESPACE

/** @} */
/** @endcond */
#endif
