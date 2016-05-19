/*
* Funambol is a mobile platform developed by Funambol, Inc. 
* Copyright (C) 2003 - 2009 Funambol, Inc.
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

#include "MediaHub/MHSyncItemInfo.h"
#include "base/Log.h"

BEGIN_FUNAMBOL_NAMESPACE

MHItemExifData::MHItemExifData() : maker(""), model(""), image_width(0), image_length(0), creation_date(0) {}

MHItemExifData::MHItemExifData(const char* maker_, const char* model_, int image_width_, int image_length_, int64_t creation_date_)
    : maker(maker_), model(model_), image_width(image_width_), image_length(image_length_), creation_date(creation_date_) {}


MHItemVideoMetaData::MHItemVideoMetaData() : codec(""), duration(0), bitrate(0), height(0), width(0) {}
MHItemVideoMetaData::MHItemVideoMetaData(const char* codec_, unsigned long duration_, unsigned int bitrate_, unsigned height_, unsigned width_) 
    : codec(codec_), duration(duration_), bitrate(bitrate_), height(height_), width(width_) {}
 
            
MHSyncItemInfo::MHSyncItemInfo(unsigned long id_,
                               const char* guid, 
                               const char* luid, 
                               const char* name, 
                               int64_t size,
                               const char* serverUrl,
                               const char* contentType, 
                               int64_t creationDate, 
                               int64_t modificationDate,
                               EItemInfoStatus status, 
                               time_t serverLastUpdate,
                               const char* remoteItemUrl,
                               const char* remoteThumbUrl,
                               const char* remotePreviewUrl,
                               const char* localThumbPath,
                               const char* localPreviewPath,
                               const char* localItemPath_,
                               const char* remoteItemETag,
                               const char* remoteThumbETag,
                               const char* remotePreviewETag)
{
    iD = id_;
    
    setGuid(guid);
    setLuid(luid);
    
    setName(name);
    this->size = size;
    
    setServerUrl(serverUrl);
    setContentType(contentType);

    this->creationDate = creationDate;
    this->modificationDate = modificationDate;

    this->status = status;
    this->serverLastUpdate  = serverLastUpdate; 

    setRemoteItemUrl(remoteItemUrl);
    setRemoteThumbUrl(remoteThumbUrl);
    setRemotePreviewUrl(remotePreviewUrl);
    setRemoteItemETag(remoteItemETag);
    setLocalThumbPath(localThumbPath);
    setLocalPreviewPath(localPreviewPath);
    setLocalItemPath(localItemPath_);
    setRemoteThumbETag(remoteThumbETag);
    setRemotePreviewETag(remotePreviewETag);
    
    setLocalItemETag("");
    setLocalPreviewETag("");
    setLocalThumbETag("");
    
    numUploadFailures = 0;
    exportedServices.clear();
    setValidationStatus(EValidationStatusUnknown);
}


// default ctor
MHSyncItemInfo::MHSyncItemInfo() {
    iD = 0L;
    guid = "";
    luid = "";
    contentType = "";
    serverUrl   = "";
    size = 0;

    creationDate = 0;   
    modificationDate = 0;
   
    status = EStatusUndefined;
    serverLastUpdate = 0; 
    remoteItemUrl    = "";
    remoteThumbUrl   = "";
    remotePreviewUrl = "";
    localThumbPath   = "";
    localPreviewPath = "";
    localItemPath    = "";
    
    exportedServices.clear();
    numUploadFailures = 0;
    setValidationStatus(EValidationStatusUnknown);
    shared = false;
}

MHSyncItemInfo::MHSyncItemInfo(const char* guid, const char* luid) 
{    
    this->iD               = 0L;
    
    if (guid) {
        this->guid = guid;
    } else {
        guid = "";
    }
    if (luid) {
        this->luid = luid;
    } else {
        luid = "";
    }

    contentType = "";
    size = 0;
    serverUrl = "";
    
    creationDate = 0;       
    modificationDate = 0;
   
    this->status = EStatusUndefined;
    this->serverLastUpdate = 0; 
    this->remoteItemUrl    = "";
    this->remoteThumbUrl   = "";
    this->remotePreviewUrl = "";
    this->localThumbPath   = "";
    this->localPreviewPath = ""; 
    this->localItemPath    = "";

    exportedServices.clear();
    numUploadFailures = 0;
    setValidationStatus(EValidationStatusUnknown);
    
    shared = false;
}

MHSyncItemInfo::~MHSyncItemInfo() {
    exportedServices.clear();
}

MHSyncItemInfo::MHSyncItemInfo(MHSyncItemInfo& copy) {
    iD = copy.getId();
    setGuid(copy.getGuid());
    setLuid(copy.getLuid());
    setName(copy.getName());
    setSize(copy.getSize());
    setServerUrl(copy.getServerUrl());
    setContentType(copy.getContentType());
    setCreationDate(copy.getCreationDate());
    setModificationDate(copy.getModificationDate());
    setStatus(copy.getStatus());
    setServerLastUpdate(copy.getServerLastUpdate());
    setRemoteItemUrl(copy.getRemoteItemUrl());
    setRemoteThumbUrl(copy.getRemoteThumbUrl());
    setRemotePreviewUrl(copy.getRemotePreviewUrl());
    setLocalThumbPath(copy.getLocalThumbPath());
    setLocalPreviewPath(copy.getLocalPreviewPath());
    setLocalItemPath(copy.getLocalItemPath());
    
    setLocalItemETag(copy.getLocalItemETag().c_str());
    setLocalThumbETag(copy.getLocalThumbETag().c_str());
    setLocalPreviewETag(copy.getLocalPreviewETag().c_str());

    setRemoteItemETag(copy.getRemoteItemETag().c_str());
    setRemoteThumbETag(copy.getRemoteThumbETag().c_str());
    setRemotePreviewETag(copy.getRemotePreviewETag().c_str());
    
    setItemExifData(copy.exifData);
    setItemVideoMetadata(copy.videoMetadata);
    setExportedServices(copy.exportedServices);
    setShared(copy.shared);
    setNumUploadFailures(copy.numUploadFailures);
}

MHSyncItemInfo* MHSyncItemInfo::clone() {
    MHSyncItemInfo* res = new MHSyncItemInfo(*this);
    return res;
}


void MHSyncItemInfo::setGuid(const char* guid){
    if (guid) {
        this->guid = guid;
    } else {
        guid = "";
    }

}

StringBuffer& MHSyncItemInfo::getGuid() {
    return guid;
}

void MHSyncItemInfo::setLuid(const char* luid){
    if (luid) {
        this->luid = luid;
    } else {
        luid = "";
    }
}

StringBuffer& MHSyncItemInfo::getLuid() {
    return luid;
}

void MHSyncItemInfo::setName(const char* name){
    if (name) {
        this->name = name;
    } else {
        name = "";
    }
}

StringBuffer& MHSyncItemInfo::getName() {
    return name;
}

void MHSyncItemInfo::setSize(int64_t size) {
    this->size = size;
}

int64_t MHSyncItemInfo::getSize() const {
    return size;
}


void MHSyncItemInfo::setServerUrl(const char* url){
    if (url) {
        this->serverUrl = url;
    } else {
        serverUrl = "";
    }        
}


StringBuffer& MHSyncItemInfo::getServerUrl() {
    return serverUrl;
}

void MHSyncItemInfo::setContentType(const char* contentType) {
    if (contentType) {
        this->contentType = contentType;
    } else {
        contentType = "";
    }
}

StringBuffer& MHSyncItemInfo::getContentType() {
    return contentType;
}

void MHSyncItemInfo::setCreationDate(int64_t date){
    this->creationDate = date;
}

void MHSyncItemInfo::setCreationDateSecs(time_t date){
    this->creationDate = (int64_t)date * 1000;
}

time_t MHSyncItemInfo::getCreationDateSecs() const {
    return (time_t)(creationDate/1000);
}

int64_t MHSyncItemInfo::getCreationDate() const {
    return creationDate;
}

void MHSyncItemInfo::setModificationDateSecs(time_t date){
    this->modificationDate = (int64_t)date * 1000;
}

void MHSyncItemInfo::setModificationDate(int64_t date){
    this->modificationDate = date;
}

time_t MHSyncItemInfo::getModificationDateSecs() const {
    return (time_t)(modificationDate / 1000);
}

int64_t MHSyncItemInfo::getModificationDate() const {
    return modificationDate;
}

void MHSyncItemInfo::setStatus(EItemInfoStatus status) {
    this->status = status;
}

void MHSyncItemInfo::setServerLastUpdate(time_t serverLastUpdate){
    this->serverLastUpdate = serverLastUpdate;
}

void MHSyncItemInfo::setRemoteItemUrl(const char* remoteItemUrl) {
    if (remoteItemUrl) {
        this->remoteItemUrl = remoteItemUrl;
    } else {
        remoteItemUrl = "";
    }        
}

void MHSyncItemInfo::setRemoteThumbUrl(const char* remoteThumbUrl) {
    if (remoteThumbUrl) {
        this->remoteThumbUrl = remoteThumbUrl;
    } else {
        remoteThumbUrl = "";
    }        
}

void MHSyncItemInfo::setRemotePreviewUrl(const char* remotePreviewUrl) {
    if (remotePreviewUrl) {
        this->remotePreviewUrl = remotePreviewUrl;
    } else {
        remotePreviewUrl = "";
    }        
}

void MHSyncItemInfo::setLocalThumbPath(const char* localThumbPath) {
    if (localThumbPath) {
        this->localThumbPath = localThumbPath;
    } else {
        this->localThumbPath = "";
    }        
}

void MHSyncItemInfo::setLocalPreviewPath(const char* localPreviewPath) {
    if (localPreviewPath) {
        this->localPreviewPath = localPreviewPath;
    } else {
        this->localPreviewPath = "";
    }
}

void MHSyncItemInfo::setLocalItemPath(const char* localItemPath) {
    if (localItemPath) {
        this->localItemPath = localItemPath;
    } else {
        this->localItemPath = "";
    }        
}

void MHSyncItemInfo::setRemoteItemETag(const char* remoteItemETag) {
    if (remoteItemETag) {
        this->remoteItemETag = remoteItemETag;
    } else {
        this->remoteItemETag = "";
    }
}

void MHSyncItemInfo::setRemoteThumbETag(const char* remoteThumbETag) {
    if (remoteThumbETag) {
        this->remoteThumbETag = remoteThumbETag;
    } else {
        this->remoteThumbETag = "";
    }
}


void MHSyncItemInfo::setRemotePreviewETag(const char* remotePreviewETag) {
    if (remotePreviewETag) {
        this->remotePreviewETag = remotePreviewETag;
    } else {
        this->remotePreviewETag = "";
    }
}

void MHSyncItemInfo::setId(unsigned long Id) {
    iD = Id;
}

unsigned long MHSyncItemInfo::getId() {
    return iD;
}
    
void MHSyncItemInfo::setExportedServices(StringMap& exportedServicesMap) 
{ 
    exportedServices = static_cast<StringMap&>(*exportedServicesMap.clone()); 
}
 
void MHSyncItemInfo::addExportedServices(const char* serviceName, time_t exportDate)
{
    if (serviceName) {
        StringBuffer exportDateStr;
        
        exportDateStr.sprintf("%ld", exportDate);
        
        exportedServices.put(serviceName, exportDateStr.c_str());
    }
}

bool MHSyncItemInfo::hasExportedService(const char* serviceName)
{
    bool res = false;
    
    if (serviceName) {
        if (exportedServices[serviceName].null() == false) {
            res = true;
        }
    }
    
    return res;
}

time_t MHSyncItemInfo::getExportTimeForService(const char* serviceName)
{
    time_t serviceExportTime = -1;
    
    if (serviceName) {
        StringBuffer exportTimeStr = exportedServices.get(serviceName);
        
        if (exportTimeStr.empty() == false) {
            serviceExportTime = atol(exportTimeStr.c_str());
        }
    }

    return serviceExportTime;
}

void MHSyncItemInfo::setExportedServicesFromString(const char* servicesList)
{
    StringBuffer services(servicesList);
    
    if (services.empty() == false) {
        ArrayList servicesParam;
        int servicesNum = 0;
        
        services.split(servicesParam, ",");
        
        servicesNum = servicesParam.size();
        
        for (int i = 0; i < servicesNum; i++) {
            StringBuffer* serviceParamStr = static_cast<StringBuffer *>(servicesParam.get(i));
                
            if ((serviceParamStr) && (serviceParamStr->empty() == false)) {
                ArrayList serviceParam;
                              
                serviceParamStr->split(serviceParam, ":");
                
                if (serviceParam.size() == 2) {
                    StringBuffer* serviceName = static_cast<StringBuffer *>(serviceParam.get(0));
                    StringBuffer* serviceExportTime = static_cast<StringBuffer *>(serviceParam.get(1));
                        
                    exportedServices.put(serviceName->c_str(), serviceExportTime->c_str());
                }
            }
        }
    }
}

StringBuffer MHSyncItemInfo::formatExportedServices()
{
    StringBuffer formattedExportedServices("");
    KeyValuePair kvp = exportedServices.front();
    
    while (kvp.null() == false) {
        StringBuffer serviceName = kvp.getKey();
        StringBuffer serviceExportTime = kvp.getValue();
    
        if (serviceName.empty() == false) {
            StringBuffer formattedService;
            
            formattedService.sprintf("%s:%s", serviceName.c_str(),
                serviceExportTime.c_str());
            
            if (formattedExportedServices.empty() == false) { 
                formattedExportedServices.append(",");
            }
            
            formattedExportedServices.append(formattedService.c_str());
        }
        
        kvp = exportedServices.next();
    }
    
    return formattedExportedServices;
}

void MHSyncItemInfo::setShared(bool value) {
    this->shared = value;
}

bool MHSyncItemInfo::getShared() {
    return shared;
}

int MHSyncItemInfo::getNumUploadFailures() {
    return numUploadFailures;
}

void MHSyncItemInfo::setNumUploadFailures(int value) {
    numUploadFailures = value;
}

void MHSyncItemInfo::setValidationStatus(char validationCode) {
    EValidationStatus newStatus;
    switch(validationCode) {
        case 'U':
            newStatus = EValidationStatusValid;
            break;
        case 'V':
            newStatus = EValidationStatusUnknown;
            break;
        case 'C':
            newStatus = EValidationStatusCopyrighted;
            break;
        case 'I':
            newStatus = EValidationStatusIllicit;
            break;
        default:
            newStatus = EValidationStatusUnknown;
            break;
    }
    setValidationStatus(newStatus);
}

bool MHSyncItemInfo::isLocallyAvailable() {
    // This is dirty, but at the moment this is the only way we can do it
    StringBuffer iD;
    iD.append(getId());
    if (iD == getLuid()) {
        // This is the a file item
        return getLocalItemPath().empty() == false;
    } else {
        // This is a pix or video
        return getLuid().empty() == false;
    }
}

bool MHSyncItemInfo::isSynced() {
    return isLocallyAvailable() && getGuid().empty() == false &&
           (status != EStatusUploading) &&
           (status != EStatusLocalMetaDataChanged);
}

bool MHSyncItemInfo::isRemotelyAvailable() {
    return getGuid().empty() == false && getStatus() != EStatusUploading;
}

END_FUNAMBOL_NAMESPACE

