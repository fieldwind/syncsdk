/*
* Funambol is a mobile platform developed by Funambol, Inc.
* Copyright (C) 2003 - 2010 Funambol, Inc.
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

#include "base/fscapi.h"
#include "base/Log.h"
#include "base/util/utils.h"
#include "ioStream/FileOutputStream.h"

#include <string.h>


USE_FUNAMBOL_NAMESPACE


FileOutputStream::FileOutputStream(const char* path, bool appendMode) : OutputStream() {
    f = NULL;
    pendingF = NULL;
    //offset = 0;
    if (path == NULL || strlen(path) == 0) {
       LOG.error("FileOutputStream error: empty file path");
       return;
    }
    
    this->path = path;
    
    if (appendMode) {
        // We do not open in append mode because we want to be able to perform writing operations at the beginning
        // of the file
        f = fileOpen(path, "r+b");
        if (f == NULL) {
            f = fileOpen(path, "w+b");
        }
        if (f != NULL) {
            bytesWritten = fgetsize(f);
            LOG.debug("[%s] file %s opened: append to byte %lld", __FUNCTION__, path, bytesWritten);
            setPosition(bytesWritten);
        }
    } else {
        f = fileOpen(path, "w+b");
    }
    
    if (f == NULL) {
        LOG.error("%s: Cannot open file for output stream %s",__FUNCTION__,path);
    }
}

FileOutputStream::~FileOutputStream() {
    close();
}

int FileOutputStream::close() {
    int ret = 1;
    if (f) {
        LOG.debug("[%s] closing file %s", __FUNCTION__, path.c_str());
        ret = fclose(f);
        f = NULL;
        pendingF = NULL;
    }
    if (pendingF) {
        LOG.debug("[%s] closing file %s", __FUNCTION__, path.c_str());
        ret = fclose(pendingF);
        pendingF = NULL;
    }
    return ret;
}

int FileOutputStream::softClose() {
    int ret = 1;
    if (f) {
        pendingF = f;
        f = NULL;
        ret = 0;
    }
    return ret;
}


int FileOutputStream::setPosition(int64_t offset) {
    if (f) {
        if (observer != NULL) {
            observer->movedToPosition(offset);
        }
        return fileSeek(f, offset, SEEK_SET);
    }
    return -1;
}

int64_t FileOutputStream::getPosition() {
    if (f) {
        return fileTell(f);
    }
    return 0;
}

int64_t FileOutputStream::writeBuffer(const void* buffer, int64_t size) {
    int64_t bytesCount = 0;
    
    if (size == 0) {
        return size;
    }

    if (!f) {
        LOG.error("FileOutputStream::write error: file is not opened");
        reset();
        return -1;
    }
    
    //LOG.debug("[%s] writing %d bytes at position %d", __FUNCTION__, size, ftell(f));
    
    bytesCount = fwrite(buffer, sizeof(char), size , f);
    
    if (!f) {
        LOG.error("FileOutputStream::write error: file is not written");
        reset();
        return -1;
    }
    
    if (bytesCount == 0) {
        if (ferror(f)) {
            LOG.error("%s: error writing to stream [%s]", __FUNCTION__, strerror(errno));

            return -1;
        }
    }

    bytesWritten += size;
    fflush(f);
    
    //LOG.debug("[%s] %d bytes written (total)", __FUNCTION__, bytesWritten);
    return size;
}


void FileOutputStream::reset() {
    if (f) {
        fileSeek(f, 0, SEEK_SET);
        if (observer != NULL) {
            observer->movedToPosition(0);
        }
    }
}
