/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2007 Funambol, Inc.
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

#ifndef INCL_MH_SYNC_ITEM_INFO
#define INCL_MH_SYNC_ITEM_INFO
/** @cond DEV */

#include <string>
#include <vector>

#include "base/fscapi.h"
#include "base/constants.h"
#include "base/util/StringMap.h"
#include "base/globalsdef.h"
#include "ioStream/FileInputStream.h"
#include "ioStream/FileOutputStream.h"
#include "MediaHub/MHStoreEntry.h"
#include "MediaHub/MHLabelInfo.h"

BEGIN_FUNAMBOL_NAMESPACE

// The possible MHSyncItemInfo status (note that to simplify backward compatibility
// there are holes in the enumeration because in the past there were more values)
// Because of this, please DO NOT change any value in this list and add new
// values only at the end.
typedef enum {
    EStatusLocal = 0,            // item is local only for the time being
    EStatusRemote,               // item is remote available
    EStatusUploading = 4,        // local item, full upload in progress
    EStatusDownloading,          // remote item, full download in progress
    EStatusRemoteOnly,           // remote item that cannot be downloaded/saved
    EStatusLocalNotUploaded = 8, // local item, not yet uploaded (i.e. exceeding quota)
    EStatusLocalOnly,            // local item, not supported by the server
    EStatusUndefined,
    EStatusLocallyRemoved,       // item locally removed (discard updates from server)
    EStatusLocalMetaDataChanged  // item has local modifications that don't affect its payload but 
                                 // only an update of its metadata (such as a rename)
} EItemInfoStatus;

typedef enum {
    EValidationStatusUnknown,
    EValidationStatusValid,
    EValidationStatusCopyrighted,
    EValidationStatusIllicit
} EValidationStatus;

struct MHItemExifData
{
    MHItemExifData();
    MHItemExifData(const char* maker_, const char* model_, int image_width_, int image_length_, int64_t creation_date_);

    StringBuffer maker;
    StringBuffer model;
    int image_width;
    int image_length;
    int64_t creation_date;
    // Note that this field is currently not stored in the DB, it is used for twin
    // detection only
    std::string downloadFingerPrint;
};

struct MHItemVideoMetaData
{
    MHItemVideoMetaData();
    MHItemVideoMetaData(const char* codec_, unsigned long duration_, unsigned int bitrate_, unsigned height_, unsigned width_);
    
    StringBuffer codec;
    unsigned long duration;
    unsigned int  bitrate;
    unsigned height;
    unsigned width;
};

/**
 * This class rapresents a single item to be synchronized.
 * In the DB it rapresents each entry.
 */
class MHSyncItemInfo : public MHStoreEntry {

private:

    unsigned long iD;                   // numeric item ID (for cache mapping)
    StringBuffer luid;                  // the local item ID
    StringBuffer guid;                  // the remote item ID
    
    StringBuffer name;
    int64_t size;
    int64_t creationDate;
    int64_t modificationDate;            // local mod date
    StringBuffer serverUrl;
    StringBuffer contentType;

    EItemInfoStatus status;
    time_t serverLastUpdate;            // remote mod date
    
    StringBuffer remoteItemUrl;         // full item url
    StringBuffer remoteThumbUrl;        // thumb 176 url
    StringBuffer remotePreviewUrl;      // thumb 504 url
    
    StringBuffer localThumbPath;        // full path of local thumbnail
    StringBuffer localPreviewPath;      // full path of local preview
    StringBuffer localItemPath;
    
    std::string remoteItemETag;
    std::string remoteThumbETag;
    std::string remotePreviewETag;
    
    std::string localItemETag;
    std::string localThumbETag;
    std::string localPreviewETag;
    
    MHItemExifData exifData;
    MHItemVideoMetaData videoMetadata;
    
    StringMap exportedServices;
    
    // Property that indicates if an item is part of a media set
    bool shared;
    int numUploadFailures;
    EValidationStatus validationStatus;
    
    std::string getResourceUri(StringBuffer& url);

    
public:

    /**
     * Initializes the SyncItemInfo
     * @param guid  const char*
     * @param luid  const char*
     * @param name  const char*
     * @param size  size_t
     * @param serverUrl const char* (server name)
     * @param contentType const char* (mime content type)
     * @param creationDate      time_t
     * @param modificationDate  time_t
     */
    MHSyncItemInfo(unsigned long id_,
                     const char* guid, 
                     const char* luid, 
                     const char* name, 
                     int64_t size,
                     const char* serverUrl,
                     const char* contentType, 
                     int64_t creationDate, 
                     int64_t modificationDate,
                     EItemInfoStatus status = EStatusUndefined, 
                     time_t serverLastUpdate = 0,
                     const char* remoteItemUrl = "",
                     const char* remoteThumbUrl = "",
                     const char* remotePreviewUrl = "",
                     const char* localThumbPath = "",
                     const char* localPreviewPath = "",
                     const char* localItemPath_ = "",
                     const char* remoteItemETag = "",
                     const char* remoteThumbETag = "",
                     const char* remotePreviewETag = "");
    // default ctor
    MHSyncItemInfo();
    MHSyncItemInfo(const char* guid, const char* luid);
    virtual ~MHSyncItemInfo();
    
    explicit MHSyncItemInfo(MHSyncItemInfo& copy);
    
    virtual MHSyncItemInfo* clone();
    
    void setGuid(const char* guid);
    StringBuffer& getGuid();

    void setLuid(const char* luid);
    StringBuffer& getLuid();

    void setName(const char* name);

    StringBuffer& getName();

    void setSize(int64_t size);
    int64_t getSize() const;
    
    void setServerUrl(const char* url);
    StringBuffer& getServerUrl();

    void setContentType(const char* contentType);
    StringBuffer& getContentType();
    
    void setCreationDate(int64_t date);
    void setCreationDateSecs(time_t date);
    time_t getCreationDateSecs() const;
    int64_t getCreationDate() const;
        
    void setModificationDate(int64_t date);
    void setModificationDateSecs(time_t date);
    time_t getModificationDateSecs() const;
    int64_t getModificationDate() const;
    
    EItemInfoStatus getStatus()         { return status;           }
    time_t getServerLastUpdate()        { return serverLastUpdate; }
    StringBuffer& getRemoteItemUrl()    { return remoteItemUrl;    }
    StringBuffer& getRemoteThumbUrl()   { return remoteThumbUrl;   }
    StringBuffer& getRemotePreviewUrl() { return remotePreviewUrl; }
    StringBuffer& getLocalThumbPath()   { return localThumbPath;   }
    StringBuffer& getLocalPreviewPath() { return localPreviewPath; }
    StringBuffer& getLocalItemPath()    { return localItemPath;    }
    
    std::string& getRemoteItemETag()      { return remoteItemETag;    }
    std::string& getRemoteThumbETag()     { return remoteThumbETag;   }
    std::string& getRemotePreviewETag()   { return remotePreviewETag; }
    
    void setStatus(EItemInfoStatus status);
    void setServerLastUpdate(time_t serverLastUpdate);
    void setRemoteItemUrl(const char* remoteItemUrl);
    void setRemoteThumbUrl(const char* remoteThumbUrl);
    void setRemotePreviewUrl(const char* remotePreviewUrl);
    void setLocalThumbPath(const char* localThumbPath);
    void setLocalPreviewPath(const char* localPreviewPath);
    void setLocalItemPath(const char* localItemPath);
    void setRemoteItemETag(const char* remoteItemETag);
    void setRemoteThumbETag(const char* remoteThumbETag);
    void setRemotePreviewETag(const char* remoteThumbETag);

    std::string& getLocalItemETag()       { return localItemETag;      }
    std::string& getLocalThumbETag()      { return localThumbETag;     }
    std::string& getLocalPreviewETag()    { return localPreviewETag;   }
    
    void setLocalItemETag(const char* localItemETag)       { this->localItemETag = localItemETag;       }
    void setLocalThumbETag(const char* localThumbETag)     { this->localThumbETag = localThumbETag;     }
    void setLocalPreviewETag(const char* localPreviewETag) { this->localPreviewETag = localPreviewETag; }
    
    void setId(unsigned long Id);
    unsigned long getId();
    
    // exif props getter/setter
    void setItemExifData(const MHItemExifData& exifData_) { exifData = exifData_; }
    const MHItemExifData& getItemExifData() const { return exifData; }

    // video metadata getter/setter
    void setItemVideoMetadata(const MHItemVideoMetaData& videoMetadata_) { videoMetadata = videoMetadata_; }
    const MHItemVideoMetaData& getItemVideoMetadata() const { return videoMetadata; }
    
    // wrapper for exif values
    const char* getExifMaker() const { return exifData.maker.c_str(); }
    const char* getExifModel() const { return exifData.model.c_str(); }
    time_t getExifCreationDate() const { return exifData.creation_date; }
    int getExifImageWidth() const { return exifData.image_width; }
    int getExifImageLength() const { return exifData.image_length; }
    
    // exported services
    void setExportedServices(StringMap& exportedServicesMap);
    const StringMap& getExportedServices() const { return exportedServices; }
    
    void addExportedServices(const char* serviceName, time_t exportDate);
    bool hasExportedService(const char* serviceName);
    time_t getExportTimeForService(const char* serviceName);
    
    // serialize/deserialize exported services data
    void setExportedServicesFromString(const char* servicesList);
    StringBuffer formatExportedServices();
    
    // Shared information
    bool getShared();
    void setShared(bool value);
    
    int getNumUploadFailures();
    void setNumUploadFailures(int value);
    
    virtual bool isLocallyAvailable();
    virtual bool isRemotelyAvailable();
    virtual bool isSynced();
    
    virtual EValidationStatus getValidationStatus() { return validationStatus; }
    virtual void setValidationStatus(EValidationStatus vStatus) { validationStatus = vStatus; }
    virtual void setValidationStatus(char validationCode);
};

END_FUNAMBOL_NAMESPACE

/** @endcond */
#endif
