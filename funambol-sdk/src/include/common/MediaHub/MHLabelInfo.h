/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2012 Funambol, Inc.
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



#ifndef INCL_MH_LABEL_INFO
#define INCL_MH_LABEL_INFO
/** @cond DEV */

#include <string>
#include "base/fscapi.h"
#include "base/constants.h"
#include "base/util/StringMap.h"
#include "base/globalsdef.h"
#include "ioStream/FileInputStream.h"
#include "ioStream/FileOutputStream.h"
#include "MediaHub/MHStoreEntry.h"

BEGIN_FUNAMBOL_NAMESPACE


#define META_LABEL_TYPE_NONE 0

/**
 * This class rapresents a label to which items can be associated. Such info is stored as a single row in the DB.
 */
class MHLabelInfo : public MHStoreEntry {

private:

    uint32_t luid;                   // numeric item ID
    uint32_t guid;                   // numeric item ID
    std::string name;
    // This value is not persistend and it is here to support meta labels which are plugin specific
    // A value of META_LABEL_TYPE_NONE means no meta label
    uint32_t metaLabelType;
    
public:

    MHLabelInfo(uint32_t guid, uint32_t luid, const char* name);
    // default ctor
    MHLabelInfo();
    virtual ~MHLabelInfo();
        
    MHLabelInfo(const MHLabelInfo& copy);
    MHLabelInfo(MHLabelInfo& copy);
    MHLabelInfo& operator= (const MHLabelInfo &other);
    
    void setName(const char* name);
    const std::string& getName() const;

    void setLuid(uint32_t luid);
    uint32_t getLuid() const;
    
    void setGuid(uint32_t guid);
    uint32_t getGuid() const;
    
    void setMetaLabelType(uint32_t metaType);
    int32_t getMetaLabelType();
};

END_FUNAMBOL_NAMESPACE

/** @endcond */
#endif
