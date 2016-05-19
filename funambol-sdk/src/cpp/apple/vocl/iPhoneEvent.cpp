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
#include "vocl/iPhoneEvent.h"
#include "vocl/TimeUtils.h"
#include "vocl/VConverter.h"
#include "base/globalsdef.h"

BEGIN_FUNAMBOL_NAMESPACE


// Constructor
iPhoneEvent::iPhoneEvent() : AppleEvent() {
}

// Constructor: fills propertyMap parsing the passed data
iPhoneEvent::iPhoneEvent(const StringBuffer & dataString) {
    parse(dataString);
}

// Destructor
iPhoneEvent::~iPhoneEvent() {
}

//
// Parse a vCalendar string and fills the propertyMap.
//
#pragma mark Parsing vCalendar
int iPhoneEvent::parse(const StringBuffer& dataString) {
    
	char* element = NULL;
	
	StringBuffer startDateValue;
	StringBuffer endDateValue;
	NSDate* sdate = nil;
	NSDate* edate = nil;
	
	VObject* vo = VConverter::parse(dataString.c_str());
	if (!vo) {        
		LOG.error("Error in parsing event");
		return 1;
	}       
	
	// Check if VObject type and version are the correct ones.
    if (!checkVCalendarTypeAndVersion(vo)) {
        if (vo) delete vo;
        return 1;
    }
	
	// TIMEZONE HANDLING 
	// parsing the timezone anyway it returns the current default timezone even if it is not set
	Timezone tz(vo);
	setEventTimezone(tz);
	
	// 
	// DTSTART, DTEND:
	// Set the start and end date. If the start is 00:00 and end is 23:59 the appointment is decided to be
	// an all day event. So the AllDayEvent property is set to '1' in the propertyMap.
	//
	if((element = vo->getPropertyValue("DTSTART"))){
		//LOG.debug("the startdate is %s", element);     
		startDateValue = element;
		sdate = stringToNSdate(element, tz);
		setStart(sdate);
	}
    
	if((element = vo->getPropertyValue("DTEND"))) {
        //LOG.debug("the enddate is %s", element);  
		endDateValue = element;
		edate = stringToNSdate(element, tz);
		setEnd(edate);
	}
	
	if (nil != sdate && nil != edate) {

        // check that startdate is lower than enddate
        if (NSOrderedAscending != [sdate compare: edate]) {
            // otherwise endate becomes startdate + 1 minute. This allows iOS
            // to store the event. Increasing the time by one minute makes the
            // behavior of the client matching the one of the portal UI
            LOG.info("warning: fix end date (is earlier than start date)");
            edate = addComponentsToDate(sdate, tz.getNSTimezone(), 0, 0, 0, 0, 1, 0);
            setEnd(edate);
        }
        
        bool isAllDay = false;
        
		if ((element = vo->getPropertyValue( "X-FUNAMBOL-ALLDAY"))) {
			isAllDay = ((strcmp(element, "1") == 0) || (strcmp(element, "TRUE") == 0)) ? true : false;
		}      
		
        if (!isAllDay) {
			// All-day check #2: interval [00:00 - 23:59]
			isAllDay = isAllDayInterval(sdate, edate);
		}
        
		if (!isAllDay) {
			// All-day check #3: format "yyyyMMdd"
			if (startDateValue.length() == 8 && endDateValue.length() == 8 ) {
				isAllDay = true;
			}
		}

		if (isAllDay) {			
			// for EndDates like "20071121T235900": Apple need we convert into "20071121T235959"
			edate = normalizeEndDayForAllDay(edate, tz);
			sdate = resetHoursMinSecs(sdate, tz);
			setStart(sdate);
			setEnd(edate);
		}
		
        setAllDayEvent(isAllDay);               
	}
    
    if ((element = vo->getPropertyValue("SUMMARY"))) {
        setTitle(element);
    }
    if ((element = vo->getPropertyValue("LOCATION"))) {
        setLocation(element);
    }
    if ((element = vo->getPropertyValue("DESCRIPTION"))) {
        setNote(element);
    }
    if ((element = vo->getPropertyValue("CATEGORIES"))) {
        setCategories(element);
    }

	if((element = vo->getPropertyValue("PRIORITY"))) {         // anyway it's not supported by iphone
		Importance imp = ImportanceNormal;
		if (!strcmp(element, "0") ) {
			imp =  ImportanceLow; 
		}
		else if (!strcmp(element, "2") ) {
			imp = ImportanceHigh;
		}                
		setImportance(imp);
	}
	if ((element = vo->getPropertyValue("CLASS"))) {          // anyway it's not supported by iphone
		Sensitivity s = NormalEvt;
		if (!strcmp(element, "CONFIDENTIAL") ) {
			s = ConfidentialEvt; 
		}
		else if (!strcmp(element, "PRIVATE") ) {
			s = PrivateEvt;
		}
		setSensitivity(s);
	}
	if((element = vo->getPropertyValue("X-MICROSOFT-CDO-BUSYSTATUS"))) {         // anyway it's not supported by iphone
		BusyStatus busy = Free;
		if (!strcmp(element, "1") ) {
			busy = Tentative; 
		}
		else if (!strcmp(element, "2") ) {
			busy = Busy;
		} 
		else if (!strcmp(element, "3") ) {
			busy = OutOfOffice;
		}                
		setBusyStatus(busy);        
	}
	
	if ((element = vo->getPropertyValue( "RRULE")) && (strlen(element) > 0)) {
		
		setIsRecurring(true);
        this->recurrence.setStartDate(getStart());
        this->recurrence.setTimeZone(tz);
        this->recurrence.parse(element);

        if (vo->containsProperty("EXDATE")) {
            VProperty * property = vo->getProperty("EXDATE");
            for (int i=0; i < property->valueCount(); i++) {
                element = property->getValue(i);
                if (element && strlen(element) > 0) {
                    [exclusions addObject:stringToNSdate(element, tz) ];
                }
            }
                                     
        }
	}
	else {
		// Not recurring.
		setIsRecurring(false);
	}
    
	if((element = vo->getPropertyValue("AALARM"))) {
		char* runTimeValue = vo->getProperty("AALARM")->getPropComponent(1);
		if ((runTimeValue != NULL) && (strlen(runTimeValue) > 0)) {
			setReminder(true);
			NSDate* date = stringToNSdate(runTimeValue, tz);
			setAlarm1(date);
		} else {
			setReminder(false);
		}
		
	}
    
	if (vo) { delete vo; vo = NULL; }	
    
    return 0;
}


// Utility to check the productID and version of VObject passed.
bool iPhoneEvent::checkVCalendarTypeAndVersion(VObject* vo) {
    
    WCHAR* prodID  = vo->getProdID();
    WCHAR* version = vo->getVersion();
    
    if (!prodID) {
        LOG.error(ERR_ITEM_VOBJ_TYPE_NOTFOUND, "VCALENDAR");
        return false;
    }
    if (strcmp(prodID, "VCALENDAR")) {
        LOG.error(ERR_ITEM_VOBJ_WRONG_TYPE, prodID, L"VCALENDAR");
        return false;
    }
    
    if (!version) {
        // Just log a warning...
        LOG.info(INFO_ITEM_VOBJ_VERSION_NOTFOUND, VCALENDAR_VERSION);
    }
    else if (wcscmp(version, VCALENDAR_VERSION)) {
        // Just log a warning...
        LOG.info(INFO_ITEM_VOBJ_WRONG_VERSION, version, VCALENDAR_VERSION);
    }
    return true;
}

void iPhoneEvent::addTimezone(VObject* vo) {
    NSTimeZone* tz = timezone.getNSTimezone();
	if (nil == tz)
        return;
    
    vo->addProperty(TEXT("TZ"), [formatDifference(offsetFromGMT(tz)) UTF8String]);
    if (!observesDST(tz)) {
        vo->addProperty(TEXT("DAYLIGHT"), TEXT("FALSE"));
    }
    
    NSDateComponents* components = getDateComponentsFromDate1970([getStart() timeIntervalSince1970]);
    
    int startYear = components.year - 1;
    int endYear = components.year + 2;
    // Compatibility check
    if([components respondsToSelector:@selector(setTimeZone:tz:)]){
        [ components setTimeZone:tz];
    }
    components.year = startYear;
    components.month = 1;
    components.day = 1;
    components.minute = 0;
    components.hour = 0;
    components.second = 0;
    NSCalendar *calendar = [NSCalendar currentCalendar];
    NSDate* date = [calendar dateFromComponents:components];
    for (int currentYear = startYear; currentYear <= endYear; currentYear++) {
        NSDate* firstTransition = [tz nextDaylightSavingTimeTransitionAfterDate:date];
        if (firstTransition) {
            NSDate* correctedDate = addComponentsToDate(firstTransition, tz, 0, 0, 0, 0, 0, -1);
            NSDate* secondTransition = [tz nextDaylightSavingTimeTransitionAfterDate:firstTransition];
            NSTimeInterval offset = [tz daylightSavingTimeOffsetForDate: firstTransition];
            
            VProperty* vp = new VProperty(TEXT("DAYLIGHT"));
            vp->addValue(TEXT("TRUE"));               // DST flag
            vp->addValue([formatDifferenceForDate(firstTransition, tz) UTF8String]);
            Timezone t([NSTimeZone timeZoneForSecondsFromGMT:[tz secondsFromGMTForDate:correctedDate]]);
            vp->addValue(NSDateToString(
                                        firstTransition,
                                        t,
                                        false,
                                        true));
            correctedDate = [secondTransition dateByAddingTimeInterval:offset];
            vp->addValue(NSDateToString(correctedDate, timezone, false, true));
            vp->addValue([tz.name UTF8String]);      // Standard time designation (optional, could be empty)
            vp->addValue([tz.name UTF8String]);      // DST designation (optional, could be empty)
            vo->addProperty(vp);
            delete vp; vp = NULL;
            date = secondTransition;
        }
    }
}

#pragma mark Format object to vCalendar
// Format and return a vCalendar string from the propertyMap.
StringBuffer iPhoneEvent::toString() {
	
    StringBuffer vCalendar("");
	
    //
    // Conversion: WinEvent -> vObject.
    // --------------------------------
    //
    VObject* vo = new VObject();
    VProperty* vp  = NULL;
  
    vo->addProperty("BEGIN", "VCALENDAR");
    vo->addProperty("VERSION", VCALENDAR_VERSION);
	
    // TIMEZONE: placed out of VEVENT. 
    // Adding it only if the event is recurring.
    addTimezone(vo);
	
    vo->addProperty("BEGIN", "VEVENT");
	
	// AllDayEvent
	vo->addProperty("X-FUNAMBOL-ALLDAY", getAllDayEvent() ? "1" : "0");          
  
	// start date
	// 
	StringBuffer sDate = NSDateToString(getStart(), getEventTimezone(), getAllDayEvent(), getAllDayEvent());
	vo->addProperty(TEXT("DTSTART"), sDate.c_str());         
  
	// end date
    NSDate* tmpEnd = getEnd();
    if (getAllDayEvent()) {
        tmpEnd = normalizeEndDayForAllDayToString(getEnd(), getEventTimezone());
    }
	StringBuffer eDate = NSDateToString(tmpEnd, getEventTimezone(), getAllDayEvent(),getAllDayEvent());
	vo->addProperty(TEXT("DTEND"), eDate.c_str());         
	
	
	// Categories: add only if not empty
    const char* categories = getCategories();
    if (categories && strlen(categories) > 0) {
        vo->addProperty("CATEGORIES", getCategories());
    }
	
	// Note
	vo->addProperty("DESCRIPTION", getNote());
	
	// Location
	vo->addProperty("LOCATION", getLocation());          
  
	// Title
	vo->addProperty("SUMMARY", getTitle());  
	
    //
    // ReminderSet
    //
    if (getReminder()) {		
    
        NSDate* alarm1 = getAlarm1();
        if (alarm1 == nil) {    // means the reminder is exactly at the start time of the event
            alarm1 = getStart();
        }
        //NSString* tmp = [alarm1 description];
        //LOG.debug("alarm1 time = %s", [tmp UTF8String]);
        
        // if allday, the AALARM time should not be in UTC
        bool useLocalTime = getAllDayEvent();
		StringBuffer alarmDate = NSDateToString(alarm1, getEventTimezone(), false, useLocalTime);
    
    
		// complete the alarm
		vp = new VProperty("AALARM");
		vp->addValue(alarmDate.c_str());              // "RunTime"
		vp->addValue("");                             // "Snooze Time" (empty)
		vp->addValue("0");                            // "Repeat Count"
		vp->addValue("");                             // "Audio Content" = sound file path
		vo->addProperty(vp);
		delete vp; vp = NULL;
	}
	else {
		// No reminder: send empty "AALARM:"
		vo->addProperty(TEXT("AALARM"), NULL);
	}    
  
    if (getIsRecurring()) {
        StringBuffer rule = this->recurrence.toString();
        if (rule.length() > 0) {
            vo->addProperty("RRULE", rule);
        }
        if ([exclusions count] > 0) {
            vp = new VProperty("EXDATE");
            for (NSDate* excludedDate in exclusions) {
                StringBuffer exDate = NSDateToString(excludedDate, getEventTimezone(), getAllDayEvent(), true);
                if (getAllDayEvent()) {
                    vp->addValue(exDate.append("T000000"));
                } else {
                    vp->addValue(exDate);
                }
            }
            vo->addProperty(vp);
          
        } else {
            vo->addProperty("EXDATE", NULL);
        }
    } else {
        vo->addProperty("RRULE", NULL);
    }
	
	// TODO: implementing recurrence rules
	/*
   if (getIsRecurring()) {
   //
   // Recurrence pattern -> RRULE
   //
   
   wstring rRule = recPattern.toString();
   if(rRule != "") {
   vo->addProperty("RRULE", rRule.c_str());             
   }
   
   // Exceptions: EXDATE
   vp = new VProperty("EXDATE");
   for (it  = excludeDate.begin(); it != excludeDate.end(); it++) {
   wstring date = (*it);
   vp->addValue(date.c_str());
   }
   vo->addProperty(vp);
   delete vp; vp = NULL;
   
   // Exceptions: RDATE (should be empty for Outlook and WM)
   vp = new VProperty("RDATE");
   vo->addProperty(vp);
   delete vp; vp = NULL;
   
   }
   else {
   // Not recurring: send empty "RRULE:"
   vo->addProperty("RRULE", NULL);
   }
   */
  
  
  
  vo->addProperty("END", "VEVENT");         
  vo->addProperty("END", "VCALENDAR");    
	
	
  //
  // Format the vCalendar.
  // ---------------------
  //
  char* tmp = vo->toString();
  if (tmp) {
    vCalendar = tmp;
    delete [] tmp;
  }
  if (vo) { delete vo; vo = NULL; }
  return vCalendar;
}

END_FUNAMBOL_NAMESPACE






