/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2009 Funambol, Inc.
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

#ifndef libfunambol_SapiStatusReport_h
#define libfunambol_SapiStatusReport_h

#include "base/util/StringBuffer.h"
/*
 Example request:
 POST /sapi/activity?action=save 
 
 { 
 "data": { 
 "deviceid": "fac-123456789", 
 "starttime": 1234567890, 
 "endtime": 1334567890, 
 "status": "200", 
 
 "activities": [ 
 { 
 "activitytype": "add", 
 "source": "picture", 
 "sent": 3 ,
 "received": 2 
 },{ 
 "activitytype": "add", 
 "source": "video", 
 "sent": 3 ,
 "received": 2 
 },{ 
 "activitytype": "delete", 
 "source": "video", 
 "sent": 3 ,
 "received": 2 
 },{ 
 "activitytype": "update", 
 "source": "video", 
 "sent": 3 ,
 "received": 2 
 } 
 ] 
 
 } 
}
 */
#include <vector>

BEGIN_FUNAMBOL_NAMESPACE

class SapiStatusReportActivity
{
private:
    StringBuffer    activitytype;
    StringBuffer    source;
    int             sent;
    int             received;
public:
    SapiStatusReportActivity();
    
    SapiStatusReportActivity(const char* _activitytype, const char* _source, int _sent, int _received);
    
    const char* getActivityType() const { return activitytype.c_str(); }
    const char* getSource() const { return source.c_str(); }
    int getSent() const { return sent; }
    int getReceived() const { return received; }
};


class SapiStatusReport
{
private:
    StringBuffer deviceid;
    unsigned long long starttime;
    unsigned long long endtime;
    int status;
    
    std::vector<const SapiStatusReportActivity*> activities;
    
public:
    SapiStatusReport();
    SapiStatusReport(const char* _deviceid, unsigned long long _starttime, unsigned long long endtime, int status);
    void addActivity(const SapiStatusReportActivity *activity);
    
    const char* getDeviceID() const { return deviceid.c_str(); }
    unsigned long long getStartTime() const { return starttime; }
    unsigned long long getEndTime() const { return endtime; }
    int getStatus() const { return status; }

    std::vector<const SapiStatusReportActivity*> getActivities() const {return activities;}
};


END_FUNAMBOL_NAMESPACE

#endif