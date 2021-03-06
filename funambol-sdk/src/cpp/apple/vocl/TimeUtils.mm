/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2008 Funambol, Inc.
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

#include "vocl/TimeUtils.h"

BEGIN_FUNAMBOL_NAMESPACE

NSDate* stringToNSdate(const char* element, Timezone& timezone) 
{    
    if (element && strcmp(element,"") == 0) {
        LOG.error("stringToNSdate failed: value is NULL or empty");
        return nil;
    }
    
    int y = 1970, m = 1, d = 1;
    int h = -1, mm = 0, ss = 0;
	
    if (strlen(element) > 8) {        
        sscanf(element, "%04d%02d%02dT%02d%02d%02d", &y, &m, &d, &h, &mm, &ss);
    } else {
        sscanf(element, "%04d%02d%02d", &y, &m, &d);
    }
    NSDateComponents *comps = [[NSDateComponents alloc] init]; 
    [comps setYear:y];
    [comps setMonth:m];
    [comps setDay:d];
    if (h > -1) {
        [comps setHour:h];
        [comps setMinute:mm];
        [comps setSecond:ss];
    } else {
        h = 0; 
    }
    static NSCalendar* gregorian = nil;
    if (!gregorian) {
        gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    }
    
    NSTimeZone* tz;
    
    if (strstr(element, "Z") != 0) {
        tz = [NSTimeZone timeZoneWithName:@"GMT"]; // the timezone for the UTC
    } else {
        tz = timezone.getNSTimezone();
    }
    [gregorian setTimeZone:tz];        
    
    NSDate *nsdate = [gregorian dateFromComponents:comps];
    [comps release];
    //[gregorian release];
    return nsdate;
}


StringBuffer NSDateToString(NSDate* date, Timezone& timezone, bool isAllDay, bool localTime) {
    
    NSCalendar* gregorian = [[NSCalendar alloc]
                              initWithCalendarIdentifier:NSGregorianCalendar];
    NSTimeZone *tzutc = [NSTimeZone timeZoneWithName:@"GMT"]; // the timezone for the UTC
    
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
                         NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    StringBuffer ret;
	
    if (isAllDay) {
        NSDateComponents *comps = [gregorian components:unitFlags fromDate:date];
        NSInteger year  = [comps year];
        NSInteger month = [comps month];
        NSInteger day   = [comps day];
        ret.append(StringBuffer().sprintf("%04d", (int) year));
        ret.append(StringBuffer().sprintf("%02d", (int) month));
        ret.append(StringBuffer().sprintf("%02d", (int) day));        
    } else {
        // set the timezone of London to see the conversion in UTC. Otherwise keep the local one
        if (localTime) {
            gregorian.timeZone = timezone.getNSTimezone();
        } else {
            gregorian.timeZone = tzutc;
		}
        
        NSDateComponents *comps = [gregorian components:unitFlags fromDate:date];
        NSInteger year  = [comps year];
        NSInteger month = [comps month];
        NSInteger day   = [comps day];
        NSInteger h     = [comps hour];
        NSInteger m     = [comps minute];
        NSInteger s     = [comps second];
        ret.append(StringBuffer().sprintf("%04d", (int) year));
        ret.append(StringBuffer().sprintf("%02d", (int) month));
        ret.append(StringBuffer().sprintf("%02d", (int) day));      
        ret.append("T"); 
        ret.append(StringBuffer().sprintf("%02d", (int) h));
        ret.append(StringBuffer().sprintf("%02d", (int) m));
        ret.append(StringBuffer().sprintf("%02d", (int) s));         
        if (!localTime) {
            ret.append("Z");
        }
    }
    [gregorian release];
    return ret;
}




bool isAllDayInterval(NSDate* sdate, NSDate* edate) 
{    
    NSCalendar* gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
	NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    NSDateComponents *st = [gregorian components:unitFlags fromDate:sdate];
    NSDateComponents *en = [gregorian components:unitFlags fromDate:edate];
    
	bool ret = false;
    if ([st hour] == 0  && [st minute] == 0 &&
        [en hour] == 23 && [en minute] == 59 ) {
        ret = true;
    }
	if ([st hour] == 0  && [st minute] == 0 &&
        [en hour] == 0  && [en minute] == 0  ) {
        ret = true;
    }
	
    [gregorian release];
    return ret;    
}

NSDate* normalizeEndDayForAllDay(NSDate* sdate, Timezone& tz) 
{
    NSCalendar* gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
                         NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    NSDateComponents *st = [gregorian components:unitFlags fromDate:sdate];
    st.hour = 23;
    st.minute = 59;
    st.second = 59;

    gregorian.timeZone = tz.getNSTimezone();
    
    NSDate* ret = [gregorian dateFromComponents:st];
    [gregorian release];
    return ret;
}

NSDate* normalizeEndDayForAllDayToString(NSDate* sdate, Timezone& tz) {
    
    
    NSDate* ret = addComponentsToDate(sdate, tz.getNSTimezone(), 0, 0, 0, 0, 1, 0);
    return ret;
    /*
    
    NSCalendar* gregorian = [[[NSCalendar alloc]                             
							  initWithCalendarIdentifier:NSGregorianCalendar] autorelease];        
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
                         NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    NSDateComponents *st = [gregorian components:unitFlags fromDate:sdate];
    
    [st setHour:23];
	[st setMinute:59];
	[st setSecond:59];
    
    NSTimeZone* timezone = tz.getNSTimezone();
    [gregorian setTimeZone:timezone];
    
    NSDate* ret = [gregorian dateFromComponents:st];
    
    return ret;
    */
}

NSDate* resetHoursMinSecs(NSDate* date, Timezone& tz) {
	
	NSCalendar* gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
                         NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    NSDateComponents *st = [gregorian components:unitFlags fromDate:date];
	st.hour = 0;
    st.minute = 0;
    st.second = 0;
    gregorian.timeZone = tz.getNSTimezone();
    NSDate* d = [gregorian dateFromComponents:st];
    [gregorian release];
    return d;
}



NSDate* addComponentsToDate(NSDate* date, NSTimeZone* timeZone, int years, int months, int days, int hours,
							int min, int secs) {
    if (date == nil) {
        LOG.error("%s: invalid date argument", __FUNCTION__);
        
        return nil;
    }
    
    NSCalendar* gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    NSDateComponents *adding = [[NSDateComponents alloc] init];
    
    if (years != 0) {
        adding.year = years;
    }
	if (months != 0) {
        adding.month = months;
    }
	if (days != 0) {
        adding.day = days;
    }
	if (hours != 0) {
        adding.hour = hours;
    }
	if (min != 0) {
        adding.minute = min;
    }
	if (secs != 0) {
        adding.second = secs;
    }
    gregorian.timeZone = timeZone;
    
    NSDate *newdate = [gregorian dateByAddingComponents:adding toDate:date options:0];
    [gregorian release];
    [adding release];
	return newdate;    
}

NSString* formatDifference(int difference) {
    int absDifference = ABS(difference);
    int hours = absDifference / 3600;
    int minutes = (absDifference - 3600 * hours) / 60;
    if (difference >= 0) {
        return [NSString stringWithFormat:@"+%02d%02d", hours, minutes];
    } else {
        return [NSString stringWithFormat:@"-%02d%02d", hours, minutes];
    }
}


NSString* formatDifferenceForDate(NSDate* date, NSTimeZone* tz) {
    return formatDifference([tz secondsFromGMTForDate:date]);
}


NSInteger offsetFromGMT(NSTimeZone* tz) {
    NSDate* d = [tz nextDaylightSavingTimeTransition];
    if (nil == d) {
        return [tz secondsFromGMT];
    }
    while ([tz isDaylightSavingTimeForDate:d]) {
        d = [tz nextDaylightSavingTimeTransitionAfterDate:d];
    }
    return [tz secondsFromGMTForDate:d];
}

BOOL observesDST(NSTimeZone* tz) {
    return nil != [tz nextDaylightSavingTimeTransition];
}


/**
 * It returns the current timezone on the device
 */
NSTimeZone* getCurrentTimezone() {
    NSTimeZone* tz = [NSTimeZone systemTimeZone];
	return tz;
}

NSTimeZone* getCurrentCalendarTimezone() {
    NSTimeZone* tz = nil;
    CFCalendarRef _calendar = CFCalendarCopyCurrent();
    
    if(_calendar) {
        CFTimeZoneRef _timeZone = CFCalendarCopyTimeZone(_calendar);
        
        if(_timeZone) {
            tz = (NSTimeZone*)_timeZone;
            CFRelease(_timeZone);
        }
        
        CFRelease(_calendar);
    }
    
	return tz;
}

NSDateComponents* getDateComponentsFromDate1970(long date) {
    NSDate* nsdate = [NSDate dateWithTimeIntervalSince1970:date];
    NSCalendar* gregorian = [[[NSCalendar alloc]                             
                              initWithCalendarIdentifier:NSGregorianCalendar] autorelease];      
    unsigned unitFlags = NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit | NSHourCalendarUnit |
    NSMinuteCalendarUnit | NSSecondCalendarUnit;
    
    NSDateComponents *comps = [gregorian components:unitFlags fromDate:nsdate];
    // OSX 10.6 compatibility check
    if([comps respondsToSelector:@selector(setCalendar:cal:)]){
        [ comps setCalendar:gregorian];
    }
    return comps;
}

unsigned long getTimeIntervalSinceReferenceDate(NSDate* date) {
	return [date timeIntervalSinceReferenceDate];
}

NSDate* yearsFromNow(int years) {
    return addComponentsToDate([NSDate date], NULL, years, 0, 0, 0, 0, 0);
}

NSDate* monthsFromNow(int months) {
    return addComponentsToDate([NSDate date], NULL, 0, months, 0, 0, 0, 0);
}


END_FUNAMBOL_NAMESPACE
