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

#include "base/util/utils.h"
#include "base/Log.h"
#include "vocl/AppleRecurrence.h"
#include "TimeUtils.h"

USE_NAMESPACE

// Constructor
AppleRecurrence::AppleRecurrence() {
    occurences = 0;
    startDate = nil;
    endDate = nil;
    weekNumber = 0;
    daysOfMonth = [[NSMutableArray alloc] init];
    weekDays = [[NSMutableArray alloc] init];
    monthsOfYear = [[NSMutableArray alloc] init];
}

// Destructor
AppleRecurrence::~AppleRecurrence() {
    [daysOfMonth release];
    [weekDays release];
    [monthsOfYear release];
    [startDate release];
    [endDate release];
}

// Format and return a RRULE string
StringBuffer AppleRecurrence::toString() {
    
    StringBuffer recurrence("");
    
    
    switch(this->recurrenceFrequency) {
            //
            // Daily = 0
            //
        case EvtRecurrenceFrequencyDaily:
            if(this->interval > 0) {
                recurrence.sprintf("D%d", this->interval);
            }
            break;
        case EvtRecurrenceFrequencyWeekly:
            if (hasWeekDays()) {
                recurrence.sprintf("W%d %s", this->interval, weekDaysAsString().c_str());
            } else {
                recurrence.sprintf("W%d", this->interval);
            }
            break;
        case EvtRecurrenceFrequencyMonthly:
            if (hasDaysOfMonth()) {
                recurrence.sprintf("MD%d %s", this->interval, daysOfMonthAsString().c_str());
            } else if (hasWeekDays()) {
                if (weekNumber == -1) {
                    weekNumber = 5;
                }
                recurrence.sprintf("MP%d %d%c %s", this->interval, ABS(weekNumber), weekNumber < 0 ? '-' : '+', weekDaysAsString().c_str());
            } else {
                recurrence.sprintf("MD%d", this->interval);
            }
            break;
        case EvtRecurrenceFrequencyYearly:
            recurrence.sprintf("YM%d", this->interval);
            if (hasWeekDays()) {
                if (weekNumber == -1) {
                    weekNumber = 5;
                }
                recurrence.append(StringBuffer().sprintf(" %d%c %s", ABS(weekNumber), weekNumber < 0 ? '-' : '+', weekDaysAsString().c_str()));
            }
            if (hasMonthsOfYear()) {
                recurrence.append(StringBuffer().sprintf(" %s", monthsOfYearAsString().c_str()));
            }
            break;
        default: {
            LOG.error("Error formatting the RRULE property: unexpected rec type = %d", this->recurrenceFrequency);
            break;
        }
             
    }
    
    //
    // Append duration info (it's the same for all rec types)
    //
    if(hasNoEnd()) {
        recurrence.append(" #0");
    } 
    else {
        if (nil != endDate) {
            if (allDay) {
                normalizeEndDayForAllDayToString(endDate, timeZone);
                recurrence.append(StringBuffer().sprintf(" %sT000000 #0", NSDateToString(endDate, timeZone, allDay, true).c_str()));
            } else {
                recurrence.append(StringBuffer().sprintf(" %s #0", NSDateToString(endDate, timeZone, allDay, true).c_str()));
            }
        } else {
            recurrence.append(StringBuffer().sprintf(" #%d", this->occurences));
        }
    }
    
    return recurrence;
}




// Parse a RRULE string and fills the propertyMap.
int AppleRecurrence::parse(const StringBuffer& dataString) {
    
    int ret = 0;
    char* str = strdup(dataString);
    char seps[] = " ";
    char* token = strtok(str, seps);
    
    if (!token) {
        goto error;
    }
    
    
    //
    // First token will be: "D|W|MP|MD|YM|YD<interval>"
    //
    if(token[0] == TEXT('D')) {
        recurrenceFrequency = EvtRecurrenceFrequencyDaily;
        token ++;
    }
    else if(token[0] == TEXT('W')) {
        recurrenceFrequency = EvtRecurrenceFrequencyWeekly;
        token ++;
    }
    else if(token[0] == TEXT('M') && token[1] == TEXT('D')) {
        recurrenceFrequency = EvtRecurrenceFrequencyMonthly;
        token += 2;
    }
    else if(token[0] == TEXT('M') && token[1] == TEXT('P')) {
        recurrenceFrequency = EvtRecurrenceFrequencyMonthly;
        token += 2;
    }
    else if(token[0] == TEXT('Y') && token[1] == TEXT('D')) {
        // "YD" Not supported!!
        LOG.info("AppleRecurrence::parse - Warning: RecurrenceType 'YD' not supported: \"%ls\"", dataString.c_str());
        goto finally;
    }
    else if(token[0] == TEXT('Y') && token[1] == TEXT('M')) {
        recurrenceFrequency = EvtRecurrenceFrequencyYearly;
        token += 2;
    }
    
    interval = _wtoi(token);
    if(!interval || recurrenceFrequency == -1) {
        goto error;
    }
    
    
    token = strtok(NULL, seps);
    while (token) {
        switch (recurrenceFrequency) {
            case EvtRecurrenceFrequencyDaily:
                if(strchr(token, '#')) {
                    this->occurences = atoi(token+1);
                    token ++;
                }
                else if(token[8] == 'T') {
                    setEndDate(stringToNSdate(token, timeZone));
                }
                break;
            case EvtRecurrenceFrequencyWeekly:
                if(strchr(token, '#')) {
                    this->occurences = _wtoi(token+1);
                    token++;
                }
                else if(token[8] == 'T') {
                    setEndDate(stringToNSdate(token, timeZone));
                } else {
                    addWeekDayByString(token);
                }

                break;
            case EvtRecurrenceFrequencyMonthly:
                if (strchr(token, '#')) {
                    occurences = atoi(token+1);
                }
                else if (strchr(token, 'T')) {
                    setEndDate(stringToNSdate(token, timeZone));
                }
                else if (strchr(token, '+') || strchr(token, '-')) {
                    weekNumber = atoi(token);
                    if (token[1] == '-') {
                        if (token[0] == '1') {
                            weekNumber = -1;
                        } else {
                            weekNumber = 5 - weekNumber;
                        }
                    } else if (token[0] == '5') {
                        weekNumber = -1;
                    }
                    token = wcstok(NULL, seps);
                    if (isWeekDay(token)) {
                        addWeekDayByString(token);
                    } else {
                        LOG.error("Wrong rrule representation: %S", dataString.c_str());
                        goto error;
                    }
                }
                else {
                    int dayOfMonth = atoi(token);
                    if (0 == dayOfMonth) {
                        goto error;
                    }
                    addDayOfMonth(dayOfMonth);
                }
                break;
            case EvtRecurrenceFrequencyYearly:
                if (strchr(token, '#')) {
                    occurences = atoi(token+1);
                    token++;
                } else if (token[8] == 'T') {
                    setEndDate(stringToNSdate(token, timeZone));
                } else if (strstr(token, "MD")) {
                    token = strtok(NULL, seps);
                    int dayOfMonth = atoi(token);
                    if (0 == dayOfMonth) {
                        LOG.error("Wrong rrule representation: %S", dataString.c_str());
                        goto error;
                    }
                    addDayOfMonth(dayOfMonth);
                } else if (strstr(token, "MP")) {
                    // nothing to do
                } else if (strchr(token, '+') || strchr(token, '-')) {
                    weekNumber = atoi(token);
                    if (token[1] == '-') {
                        if (token[0] == '1') {
                            weekNumber = -1;
                        } else {
                            weekNumber = 5 - weekNumber;
                        }
                    } else if (token[0] == '5') {
                        weekNumber = -1;
                    }
                    token = wcstok(NULL, seps);
                    if (isWeekDay(token)) {
                        addWeekDayByString(token);
                    } else {
                        LOG.error("Wrong rrule representation: %S", dataString.c_str());
                        goto error;
                    }
                } else {
                    addMonthOfYear(atoi(token));
                }
        }
        token = wcstok(NULL, seps);
    }
    
    ret = 0;
    goto finally;
    
error:
    LOG.error("AppleRecurrence::parse error, bad RRULE format: %s", dataString.c_str());
    ret = 1;
    goto finally;
    
finally:
    if (str)     delete [] str;
    return ret;
}
