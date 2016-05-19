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



#include "MediaHub/MHConfig.h"
#include "base/util/utils.h"
#include "base/globalsdef.h"
#include "base/util/KeyValuePair.h"
#include "spdm/constants.h"

USE_NAMESPACE



MHConfig::MHConfig() : responseTimeout(0),
                       requestTimeout(0),
                       uploadChunkSize(4096),
                       downloadChunkSize(4096)
{
}

MHConfig::~MHConfig() {}


void MHConfig::setRequestTimeout(const int timeout) { 
    requestTimeout = timeout;
}

void MHConfig::setResponseTimeout(const int timeout) { 
    responseTimeout = timeout;
}

void MHConfig::setUploadChunkSize(const int size) { 
    uploadChunkSize = size;
}

void MHConfig::setDownloadChunkSize(const int size) { 
    downloadChunkSize = size;
}

void MHConfig::setMaxRetriesOnError(const int retries) { 
    setIntProperty(PROPERTY_MAX_RETRIES_ON_ERROR, retries); 
}

void MHConfig::setSleepTimeOnRetry(const long msec) { 
    setLongProperty(PROPERTY_SLEEP_TIME_ON_RETRY, msec); 
}

void MHConfig::setResetStreamOnRetry(bool reset) {
    setBoolProperty(PROPERTY_RESET_STREAM_ON_RETRY, reset);
}

void MHConfig::setMinDataSizeOnRetry(const long size) {
    setLongProperty(PROPERTY_MIN_DATA_SIZE_ON_RETRY, size);
}

void MHConfig::setCaredServer(bool cared) {
    setBoolProperty(PROPERTY_IS_CARED_SERVER, cared);
}

int MHConfig::getRequestTimeout() {
    return requestTimeout;
}

int MHConfig::getResponseTimeout() {
    return responseTimeout;
}

int MHConfig::getUploadChunkSize() {
    return uploadChunkSize;
}

int MHConfig::getDownloadChunkSize() {
    return downloadChunkSize;
}

int MHConfig::getMaxRetriesOnError() {
    bool err = false;
    int ret = getIntProperty(PROPERTY_MAX_RETRIES_ON_ERROR, &err);
    if (err) {
        LOG.debug("%s: property %s not found: set to 0", __FUNCTION__, PROPERTY_MAX_RETRIES_ON_ERROR);
        return 0;
    }
    return ret;
}

long MHConfig::getSleepTimeOnRetry() {
    bool err = false;
    long ret = getLongProperty(PROPERTY_SLEEP_TIME_ON_RETRY, &err);
    if (err) {
        LOG.debug("%s: property %s not found: set to 0", __FUNCTION__, PROPERTY_SLEEP_TIME_ON_RETRY);
        return 0;
    }
    return ret;
}

bool MHConfig::getResetStreamOnRetry() {
    bool err = false;
    bool ret = getBoolProperty(PROPERTY_RESET_STREAM_ON_RETRY, &err);
    
    if (err) {
        LOG.debug("%s: property %s not found: set to false", __FUNCTION__, PROPERTY_RESET_STREAM_ON_RETRY);
        return false;
    }
    
    return ret;
}

long MHConfig::getMinDataSizeOnRetry() {
    bool err = false;
    long ret = getLongProperty(PROPERTY_MIN_DATA_SIZE_ON_RETRY, &err);
    if (err) {
        LOG.debug("%s: property %s not found: set to 0", __FUNCTION__, PROPERTY_MIN_DATA_SIZE_ON_RETRY);
        return 0;
    }
    
    return ret;
}

bool MHConfig::isCaredServer() {
    bool err = true;
    bool ret = getBoolProperty(PROPERTY_IS_CARED_SERVER, &err);
    
    if (err) {
        return true;
    }
    
    return ret;
}

void MHConfig::assign(const MHConfig& sc) {
    if (&sc == this) {
        return;
    }
    extraProps = sc.getExtraProps();
}


void MHConfig::setProperty(const char* propertyName, const char* propertyValue) {
    CustomConfig::setProperty(propertyName, propertyValue);
    if (strcmp(PROPERTY_REQUEST_TIMEOUT,propertyName) == 0) {
        setRequestTimeout(atoi(propertyValue));
    } else if (strcmp(PROPERTY_RESPONSE_TIMEOUT,propertyName) == 0) {
        setResponseTimeout(atoi(propertyValue));
    } else if (strcmp(PROPERTY_UPLOAD_CHUNK_SIZE,propertyName) == 0) {
        setUploadChunkSize(atoi(propertyValue));
    } else if (strcmp(PROPERTY_DOWNLOAD_CHUNK_SIZE,propertyName) == 0) {
        setDownloadChunkSize(atoi(propertyValue));
    }
}
