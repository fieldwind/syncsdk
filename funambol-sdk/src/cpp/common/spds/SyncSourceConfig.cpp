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



#include "spds/SyncSourceConfig.h"
#include "base/util/utils.h"
#include "base/globalsdef.h"
#include "base/util/KeyValuePair.h"
#include "spdm/constants.h"

USE_NAMESPACE



SyncSourceConfig::SyncSourceConfig() {
    name            = NULL;
    uri             = NULL;
    syncModes       = NULL;
    type            = NULL;
    sync            = NULL;
    encodings       = NULL;
    version         = NULL;
    supportedTypes  = NULL;
    encryption      = NULL;
    mode            = SYNC_MODE_AUTO;
    allowed         = true;
    last            = 0;
    lastSourceError = 0;
    lastServerTime  = 0;
}

SyncSourceConfig::~SyncSourceConfig() {
    if (name) {
        delete [] name;
    }
    if (uri) {
        delete [] uri;
    }
    if (syncModes) {
        delete [] syncModes;
    }
    if (type) {
        delete [] type;
    }
    if (sync) {
        delete [] sync;
    }
    if (encodings) {
        delete [] encodings;
    }
    if (version) {
        delete [] version;
    }
    if (supportedTypes) {
        delete [] supportedTypes;
    }
    if (encryption) {
        delete [] encryption;
    }
}


const char* SyncSourceConfig::getName() const {
    return name;
}

void SyncSourceConfig::setName(const char* n) {
    safeDelete(&name);
    name = stringdup(n);
}

const char* SyncSourceConfig::getURI() const {
//    return "http://211.138.118.199/funambol/ds";
    return uri ? uri :
        name ? name :
        "";
}

void SyncSourceConfig::setURI(const char* u) {
    safeDelete(&uri);
    uri = stringdup(u);
}


const char* SyncSourceConfig::getSyncModes() const {
    return syncModes ? syncModes :
        "";

}

void SyncSourceConfig::setSyncModes(const char* s) {
    safeDelete(&syncModes);

    if (s) {
        syncModes = new char[strlen(s)+1];
        strcpy(syncModes, s);
    }
}

const char* SyncSourceConfig::getType() const {
    return type ? type :
        "";
}

void SyncSourceConfig::setType(const char* t) {
    safeDelete(&type);

    if (t) {
        type = new char[strlen(t)+1];
        strcpy(type, t);
    }
}

const char* SyncSourceConfig::getSync() const {
    return sync ? sync :
        "";
}

void SyncSourceConfig::setSync(const char *s) {
    safeDelete(&sync);

    if (s) {
        sync = new char[strlen(s)+1];
        strcpy(sync, s);
    }
}

void SyncSourceConfig::setLast(unsigned long t) {
    last = t;
}

unsigned long SyncSourceConfig::getLast() const {
    return last;
}

void SyncSourceConfig::setEndSyncTime(unsigned long t) {
    setLongProperty(PROPERTY_SYNC_END, (unsigned long)t);
}

unsigned long SyncSourceConfig::getEndSyncTime() {
    bool err;
    long ret = getLongProperty(PROPERTY_SYNC_END, &err);
    if (err || ret < 0) {
        ret = 0;
    }
    return (unsigned long) ret;
}

void SyncSourceConfig::setBeginSyncTime(unsigned long t) {
    setLongProperty(PROPERTY_SYNC_BEGIN, (unsigned long)t);
}

unsigned long SyncSourceConfig::getBeginSyncTime() {
    bool err;
    long ret = getLongProperty(PROPERTY_SYNC_BEGIN, &err);
    if (err || ret < 0) {
        ret = 0;
    }
    return (unsigned long) ret;
}

const char* SyncSourceConfig::getEncoding() const {
    return encodings ? encodings :
        "";
}

void SyncSourceConfig::setEncoding(const char* s) {
    safeDelete(&encodings);

    if (s) {
        encodings = stringdup(s);
    }
}

const char* SyncSourceConfig::getVersion() const {
    return version;
}
void SyncSourceConfig::setVersion(const char* v) {
    safeDelete(&version);
    version = stringdup(v);
}

const char* SyncSourceConfig::getSupportedTypes() const {
    return supportedTypes;
}
void SyncSourceConfig::setSupportedTypes(const char* s) {
    safeDelete(&supportedTypes);
    supportedTypes = stringdup(s);
}

bool SyncSourceConfig::isEnabled() const {
    return mode == SYNC_MODE_AUTO || mode == SYNC_MODE_MANUAL;
}

void SyncSourceConfig::setSyncMode(SourceSyncMode newMode) {
    mode = newMode;
}

SourceSyncMode SyncSourceConfig::getSyncMode() const {
    return mode;
}


/// getter and setters for the new config 'allowed' setted by server (depends by the roles of user)
bool SyncSourceConfig::isAllowed() const {
    return allowed;
}

void SyncSourceConfig::setIsAllowed(const bool s) {
	allowed = s;
}



CTCap* SyncSourceConfig::createCtCap(ArrayList *props, const char *ct_Type, const char *ver_CT, bool fLevel) {

    return new CTCap(ct_Type , ver_CT, fLevel, *props);
}

void SyncSourceConfig::addCtCap(ArrayList *props, const char *ct_Type, const char *ver_CT, int fLevel){
    const char* cttype;
    const char* verct;
    bool fieldlevel;
    cttype = ( ct_Type ) ? ct_Type : getType()    ; 
    verct  = ( ver_CT  ) ? ver_CT  : getVersion() ;
    if ( fLevel == FLEVEL_UNDEFINED ){
        fieldlevel = getFieldLevel();
    }else if ( fLevel == FLEVEL_DISABLED ){
        fieldlevel = false;
    }else {
        assert( fLevel == FLEVEL_ENABLED );
        fieldlevel = true;
    }
    
    CTCap* ctcaps = createCtCap(props, cttype, verct, fieldlevel);

    ctCaps.add(*(ctcaps));
    delete ctcaps;

}

const char* SyncSourceConfig::getEncryption() const {
    return encryption;
}

void SyncSourceConfig::setEncryption(const char* n) {
    safeDelete(&encryption);
    encryption = stringdup(n);

}


int SyncSourceConfig::getLastSourceError() const {
    return lastSourceError;
}

void SyncSourceConfig::setLastSourceError(const int errorCode) {
    lastSourceError = errorCode;
}


void SyncSourceConfig::assign(const SyncSourceConfig& sc) {
    if (&sc == this) {
        return;
    }

    setName          (sc.getName          ());
    setURI           (sc.getURI           ());
    setSyncModes     (sc.getSyncModes     ());
    setType          (sc.getType          ());
    setSync          (sc.getSync          ());
    setLast          (sc.getLast          ());
    setEncoding      (sc.getEncoding      ());
    setVersion       (sc.getVersion       ());
    setSupportedTypes(sc.getSupportedTypes());
    setSyncMode      (sc.getSyncMode      ());
	setIsAllowed     (sc.isAllowed        ());
//    setCtCap         (sc.getCtCap         ());
    setEncryption    (sc.getEncryption    ());
    setLastSourceError(sc.getLastSourceError());
    setLastSyncServerTime(sc.AbstractSyncSourceConfig::getLastSyncServerTime());
    
    extraProps = sc.getExtraProps();
}
