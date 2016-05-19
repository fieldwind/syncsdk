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

#ifndef INCL_APPLERECURRENCE
#define INCL_APPLERECURRENCE

/** @cond API */
/** @addtogroup apple_adapter */
/** @{ */

#include "base/fscapi.h"
#include "base/globalsdef.h"
#include "Foundation/Foundation.h"
#include "Timezone.h"



BEGIN_NAMESPACE


typedef enum {
    EvtRecurrenceFrequencyDaily,
    EvtRecurrenceFrequencyWeekly,
    EvtRecurrenceFrequencyMonthly,
    EvtRecurrenceFrequencyYearly
} EvtRecurrenceFrequency;

typedef enum {
    EvtSunday = 1,
    EvtMonday,
    EvtTuesday,
    EvtWednesday,
    EvtThursday,
    EvtFriday,
    EvtSaturday
} EvtRecurrenceWeekDay;

/**
 * Represents a recurrence pattern object for Apple Clients.
 * The object can be filled passing a vCalendar RRULE string, or filling
 * directly the map. Calling 'toString()' a vCalendar RRULE is formatted and returned.
 */
class AppleRecurrence {
    
private:
    EvtRecurrenceFrequency recurrenceFrequency;
    int interval;
    int occurences;
    Timezone timeZone;
    NSDate* endDate;
    /// The start date (double format) of the correspondent event/task
    /// It's necessary to parse correctly the RRULE string
    NSDate* startDate;
    
    bool isWeekDay(char* data) {
        return !strcmp(data,"SU") || !strcmp(data,"MO") || !strcmp(data,"TU") || !strcmp(data,"WE") || !strcmp(data,"TH") || !strcmp(data,"FR") || !strcmp(data,"SA");
    }
    
    bool allDay;
    
    NSMutableArray* weekDays;
    NSMutableArray* daysOfMonth;
    NSMutableArray* monthsOfYear;
    
    StringBuffer nsIntArrayToString(NSArray* intArray) {
        StringBuffer sb;
        for (NSNumber* number in intArray) {
            sb.append(StringBuffer().sprintf("%d ",[number intValue]));
        }
        return sb.length() > 0 ? sb.substr(0, sb.length() - 1) : sb;
    }
    
    int weekNumber;
    
protected:
    
    
    /// true if the parent calendar has a timezone information, and uses it.
    /// Recurring props are in local time if this flag is true.
    bool useTimezone;
    
    
public:
    
    /// Default Constructor
    AppleRecurrence();
    
    /// Destructor
    virtual ~AppleRecurrence();
    
    
    /**
     * Parse a vCalendar RRULE string and fills the propertyMap.
     * @param dataString   input RRULE string to parse
     * @return             0 if no errors
     */
    virtual int parse(const StringBuffer & dataString);
    
    /// Format and return a vCalendar RRULE string from the propertyMap.
    virtual StringBuffer toString();
    
    
    void setStartDate(NSDate* d)       {
        [d retain];
        [startDate release];
        startDate = d;
    }
    
    NSDate* getStartDate()   const     { return startDate; }
    
    void setEndDate(NSDate* d) {
        [d retain];
        [endDate release];
        endDate = d;
    }
    
    NSDate* getEndDate() const {
        return endDate;
    }

    /// Returns the 'useTimezone' flag value.
    const bool hasTimezone()           { return useTimezone; }
    
    /// Sets the 'useTimezone' flag to the given value.
    void setUseTimezone(bool val)      { useTimezone = val;  } 
    
    void setTimeZone(const Timezone& tz) { timeZone = tz; }
    Timezone& getTimeZone() { return timeZone; }
    
    EvtRecurrenceFrequency getFrequency() const { return recurrenceFrequency; }
    void setFrequency(EvtRecurrenceFrequency frequency) { recurrenceFrequency = frequency; }
    
    int getInterval() const { return interval; }
    void setInterval(int interval) { this->interval = interval; }
    
    int getOccurences() const { return occurences; }
    void setOccurences(int occurences) { this->occurences = occurences; }
    
    bool hasNoEnd() { return nil == endDate && 0 == occurences; }
    
    void addWeekDay(EvtRecurrenceWeekDay weekDay) {
        [weekDays addObject:[NSNumber numberWithInt:weekDay]];
    }
    
    void addWeekDayByString(char* data) {
        if (!strcmp(data,"MO")) addWeekDay(EvtMonday);
        if (!strcmp(data,"TU")) addWeekDay(EvtTuesday);
        if (!strcmp(data,"WE")) addWeekDay(EvtWednesday);
        if (!strcmp(data,"TH")) addWeekDay(EvtThursday);
        if (!strcmp(data,"FR")) addWeekDay(EvtFriday);
        if (!strcmp(data,"SA")) addWeekDay(EvtSaturday);
        if (!strcmp(data,"SU")) addWeekDay(EvtSunday);
    }
    
    bool hasWeekDay(EvtRecurrenceWeekDay weekDay) {
        return [weekDays containsObject:[NSNumber numberWithInt:weekDay]];
    }
    
    int hasWeekDays() {
        return [weekDays count] > 0;
    }
    
    StringBuffer weekDaysAsString() {
        StringBuffer weekDaysString;
        static const char * weekDayStrings [] = { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };
        for (NSNumber* weekDay in [weekDays sortedArrayUsingSelector:@selector(compare:)]) {
            weekDaysString.append(weekDayStrings[[weekDay intValue] - 1]);
            weekDaysString.append(" ");
        }
        return weekDaysString.length() > 0 ? weekDaysString.substr(0, weekDaysString.length() - 1) : weekDaysString;
    }
    
    void setAllDay(bool allDay) { this->allDay = allDay; }
    
    NSMutableArray* getDaysOfMonth() const { return daysOfMonth; }
    bool hasDaysOfMonth() const { return [daysOfMonth count] > 0; }
    void addDayOfMonth(int dayOfMonth) { [daysOfMonth addObject:[NSNumber numberWithInt:dayOfMonth]]; }
    StringBuffer daysOfMonthAsString() { return nsIntArrayToString(daysOfMonth); }
    
    NSMutableArray* getMonthsOfYear() const { return monthsOfYear; }
    bool hasMonthsOfYear() const { return [monthsOfYear count] > 0; }
    void addMonthOfYear(int month) { [monthsOfYear addObject: [NSNumber numberWithInt:month]]; }
    StringBuffer monthsOfYearAsString() { return nsIntArrayToString(monthsOfYear); }
    
    int getWeekNumber() const { return weekNumber; }
    void setWeekNumber(int weekNumber) { this->weekNumber = weekNumber; }
};


END_NAMESPACE

/** @} */
/** @endcond */
#endif
