/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2013 Funambol, Inc.
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

#ifndef __SERVER_INFO_H__
#define __SERVER_INFO_H__

#include <string>
#include <vector>

namespace Funambol {
  
class ServerInfo {
    public:
        ServerInfo() : portalUrl(""), mobileUrl(""), sapiVersion(""), manifacturer(""), mod(""),
                            hwv(""), fwv(""), oem(""), devId(""), devType(""), verDtd(""), utc(false), 
                            supportLargeObjs(false), supportNumberOfChanges(false), exts("") {}
    
        std::string getPortalUrl() const        { return portalUrl; }
        std::string getMobileUrl() const        { return mobileUrl; }
        std::string getSapiVersion() const      { return sapiVersion; }
        std::string getManifacturer() const     { return manifacturer; }
        std::string getMod() const              { return mod; }
        std::string getHwv() const              { return hwv; }
        std::string getFwv() const              { return fwv; }
        std::string getOem() const              { return oem; }
        std::string getDevId() const            { return devId; }
        std::string getDevType() const          { return devType; }
        std::string getVerDtd()  const          { return verDtd; }
        std::string getExts() const             { return exts; }
        bool getUtc() const                     { return utc; }
        bool getSupportLargeObjs() const        { return supportLargeObjs; }
        bool getSupportNumberOfChanges() const  { return supportNumberOfChanges; }
        
        void setPortalUrl(const std::string& value)     { portalUrl = value; }
        void setMobileUrl(const std::string& value)     { mobileUrl = value; }
        void setSapiVersion(const std::string& value)   { sapiVersion = value; }
        void setManifacturer(const std::string& value)  { manifacturer = value; }
        void setMod(const std::string& value)           { mod = value; }
        void setHwv(const std::string& value)           { hwv = value; }
        void setFwv(const std::string& value)           { fwv = value; }
        void setOem(const std::string& value)           { oem = value; }
        void setDevId(const std::string& value)         { devId = value; }
        void setDevType(const std::string& value)       { devType = value; }
        void setVerDtd(const std::string& value)        { verDtd = value; }
        void setExts(const std::string& value)          { exts = value; }
        void setUtc(bool value)                         { utc = value; }
        void setSupportLargeObjs(bool value)            { supportLargeObjs = value; }
        void setSupportNumberOfChanges(bool value)      { supportNumberOfChanges = value; }
        
    private:
        std::string portalUrl;
        std::string mobileUrl;
        std::string sapiVersion;
        std::string manifacturer;
        std::string mod;
        std::string hwv;
        std::string fwv;
        std::string oem;
        std::string devId;
        std::string devType;
        std::string verDtd;
        bool utc;
        bool supportLargeObjs;
        bool supportNumberOfChanges;
        std::string exts;
};
  
}


#endif
