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



#include <string.h>
#include <stdlib.h>

#include "base/util/utils.h"
#include "spds/SyncItem.h"
#include "spds/DataTransformerFactory.h"
#include "base/globalsdef.h"
#include "ioStream/BufferInputStream.h"
#include "base/util/EncodingHelper.h"
#include "base/Log.h"

USE_NAMESPACE

const char* const SyncItem::encodings::plain = "bin";
const char* const SyncItem::encodings::escaped = "b64";
const char* const SyncItem::encodings::des = "des;b64";

/*
 * Default constructor
 */
SyncItem::SyncItem() {
    initialize();
}


/*
 * Constructs a new SyncItem identified by the given key. The key must
 * not be longer than DIM_KEY (see SPDS Constants).
 *
 * @param key - the key
 */
SyncItem::SyncItem(const WCHAR* itemKey) {
    initialize();
    if (itemKey) {
        wcsncpy(key, itemKey, DIM_KEY);
        key[DIM_KEY-1] = 0;
    }
}

/**
 * Initializes private members
 */
void SyncItem::initialize() {
    type[0] = 0;
    data = NULL;
    encoding = NULL;
    size = -1;
    lastModificationTime = -1;
    key[0] = 0;
    targetParent = NULL;
    sourceParent = NULL;

    inputStream = NULL;
}

/*
 * Destructor. Free the allocated memory (if any)
 */
SyncItem::~SyncItem() {
    if (data) {
        delete [] data; data = NULL;
    }
    if (encoding) {
        delete [] encoding; encoding = NULL;
    }
    if (targetParent) {
        delete [] targetParent; targetParent = NULL;
    }
    if (sourceParent) {
        delete [] sourceParent; sourceParent = NULL;
    }

    if (inputStream) {
        inputStream->close();
    }
    delete inputStream;
}

const char* SyncItem::getDataEncoding() const {
    return encoding;
}

void SyncItem::setDataEncoding(const char* enc) {
    if (encoding) {
        delete [] encoding;
    }
    encoding = stringdup(enc);
}

int SyncItem::changeDataEncoding(const char* enc, const char* encryption, const char* credentialInfo) {

    int ret = ERR_NONE;
    EncodingHelper helper(enc, encryption, credentialInfo);

    // if the current encoding is NULL or plain we do nothing on going out. we are decoding
    if (strcmp(encodings::encodingString(encoding), encodings::plain)) {
        unsigned long s = size;
        char* transformed = helper.decode(encodings::encodingString(encoding), data, &s);
        if (!transformed) {
            LOG.debug("SyncItem changeDataEncoding: item not changed");
            ret = ERR_UNSPECIFIED;
            return ret;
        } else {
            setData(transformed, s);
            delete [] transformed;   
            setDataEncoding(encodings::plain);        
        }
    }
        
    if (size >= 0) {
        unsigned long s = size;
        char* transformed = helper.encode(encodings::encodingString(encoding), data, &s);
        if (!transformed) {
            LOG.debug("SyncItem changeDataEncoding: item not changed");
            ret = ERR_UNSPECIFIED;
        } else {
            setData(transformed, s);
            delete [] transformed;   
            setDataEncoding(helper.getDataEncoding());        
        }
    }

    return ret;
}

/*
 * Returns the SyncItem's key. If key is NULL, the internal buffer is
 * returned; if key is not NULL, the value is copied in the caller
 * allocated buffer and the given buffer pointer is returned.
 *
 * @param key - buffer where the key will be stored
 */
const WCHAR* SyncItem::getKey() const {
        return key;
    }

/*
 * Changes the SyncItem key. The key must not be longer than DIM_KEY
 * (see SPDS Constants).
 *
 * @param key - the key
 */
void SyncItem::setKey(const WCHAR* itemKey) {
    wcsncpy(key, itemKey, DIM_KEY);
    key[DIM_KEY-1] = 0;
}

/*
 * Sets the SyncItem modification timestamp. timestamp is a milliseconds
 * timestamp since a reference time (which is platform specific).
 *
 * @param timestamp - last modification timestamp
 */
 void SyncItem::setModificationTime(long timestamp) {
     lastModificationTime = timestamp;
 }

/*
 * Returns the SyncItem modeification timestamp. The returned value
 * is a milliseconds timestamp since a reference time (which is
 * platform specific).
 */
long SyncItem::getModificationTime() const {
    return lastModificationTime;
}

/*
 * Sets the SyncItem content data. The passed data are copied into an
 * internal buffer so that the caller can release the buffer after
 * calling setData(). The buffer is fred in the destructor.
 * If when calling setData, there was an existing allocated data block,
 * it is reused (shrinked or expanded as necessary).
 */
void* SyncItem::setData(const void* itemData, long dataSize) {
    if (data) {
        delete [] data; data = NULL;
    }

    size = dataSize;

    // Not yet set.
    if (size == -1) {
        data = NULL;
        return data;
    }

    data = new char[size + 1];
    if (data == NULL) {
        //lastErrorCode = ERR_NOT_ENOUGH_MEMORY;
        //sprintf(lastErrorMsg, ERRMSG_NOT_ENOUGH_MEMORY, (int)dataSize);
        setErrorF(ERR_NOT_ENOUGH_MEMORY, ERRMSG_NOT_ENOUGH_MEMORY, (int)dataSize);
        return NULL;
    }

    if (itemData) {
        memcpy(data, itemData, size);
        data[size] = 0;  // FIXME: needed?
    } else {
        memset(data, 0, size + 1);
    }

    // Set a new BufferInputStream linked to this data
    if (inputStream) {
        inputStream->close();
        delete inputStream;
    }
    inputStream = new BufferInputStream(data, size);

    return data;
}

InputStream* SyncItem::getInputStream() {
    return inputStream;
}

/*
 * Returns the SyncItem data buffer. It is deleted in the destructor.
 */
void* SyncItem::getData() const {
    return data;
}

/*
 * Returns the SyncItem data size.
 */
long SyncItem::getDataSize() const {
    return size;
}

/*
 * Sets the SyncItem data size.
 */
void SyncItem::setDataSize(long s) {
    size = s;
}

/*
 * Sets the SyncItem data mime type
 *
 * @param - type the content mimetype
 */
void SyncItem::setDataType(const WCHAR* mimeType) {
    wcsncpy(type, mimeType, DIM_MIME_TYPE);
    type[DIM_MIME_TYPE-1] = 0;
}

/*
 * Returns the SyncItem data mime type.
 *
 */
const WCHAR* SyncItem::getDataType() const {
    return type;
}

/*
 * Sets the SyncItem state
 *
 * @param state the new SyncItem state
 */
void SyncItem::setState(SyncState newState) {
    state = newState;
}

/*
 * Gets the SyncItem state
 */
SyncState SyncItem::getState() const {
    return state;
}

/**
 * Gets the taregtParent property
 *
 * @return the taregtParent property value
 */
const WCHAR* SyncItem::getTargetParent() const {
    return targetParent;
}

/**
 * Sets the taregtParent property
 *
 * @param parent the taregtParent property
 */
void SyncItem::setTargetParent(const WCHAR* parent) {
    if (targetParent) {
        delete [] targetParent; targetParent = NULL;
    }
    targetParent = wstrdup(parent);
}

/**
 * Gets the sourceParent property
 *
 * @return the sourceParent property value
 */
const WCHAR* SyncItem::getSourceParent() const {
    return sourceParent;
}

/**
 * Sets the sourceParent property
 *
 * @param parent the sourceParent property
 */
void SyncItem::setSourceParent(const WCHAR* parent) {
    if (sourceParent) {
        delete [] sourceParent; sourceParent = NULL;
    }
    sourceParent = wstrdup(parent);
}

ArrayElement* SyncItem::clone() {
    SyncItem* ret = new SyncItem(key);

    ret->setData(data, size);
    ret->setDataType(type);
    ret->setModificationTime(lastModificationTime);
    ret->setState(state);
    ret->setSourceParent(sourceParent);
    ret->setTargetParent(targetParent);

    return ret;
}

int SyncItem::getSyncStatus() const {
    return syncStatus;
}

void SyncItem::setSyncStatus(int syncStatus) {
    this->syncStatus = syncStatus;
}
