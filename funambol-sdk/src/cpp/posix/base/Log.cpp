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


#include "base/globalsdef.h"
#include "base/posixlog.h"
#include "base/fscapi.h"
#include "base/util/utils.h"

#include <unistd.h>

//Change made for US11215 on Mac
#ifdef FUN_MAC
#import <Foundation/Foundation.h>
#endif

USE_NAMESPACE

POSIXLog::POSIXLog() :
    logFile(NULL),
    logFileStdout(false),
    logName(LOG_NAME),
    logPath(NULL),
    logRedirectStderr(false),
    fderr(-1),
    prefix("")
{
    // Init the log mutex
//    pthread_mutex_init(&logMutex, NULL);
}

void POSIXLog::setLogPath(const char*  configLogPath) {
    // If empty or null, set current dir.
    if(configLogPath == 0|| *configLogPath == '\0') {
        logPath = "./";
    }
    else {
        logPath.sprintf("%s/", configLogPath);
    }
}

void POSIXLog::setLogName(const char*  configLogName) {
    logName.sprintf("%s", configLogName ? configLogName : LOG_NAME);
}

void POSIXLog::setLogFile(const char *path, const char* name, bool redirectStderr) {
    
//    pthread_mutex_lock(&logMutex);
    
    if (logPath != path) {
        setLogPath(path);
    }
    if (logName != name) {
        setLogName(name);
    }
    logRedirectStderr = redirectStderr;

    if (logFile != NULL) {
        fclose(logFile);
        logFile = NULL;
    }
    logFileStdout = false;

    if (!strcmp(name, "-")) {
        // write to stdout
        logFileStdout = true;
    } else if (path) {
        char *filename = new char[strlen(path) + strlen(name) + 3];

        sprintf(filename, "%s/%s", path, name);
        logFile = fopen(filename, "a+" );
        delete [] filename;
    } else {
        logFile = fopen(name, "a+" );
    }
//    pthread_mutex_unlock(&logMutex);

    if (logFile) {
        char buffer[256];
        struct tm tm;
        time_t t = time(NULL);

        // We log UTC at the start of each line.
        // Log the current user's time offset.
        localtime_r(&t, &tm);
        strftime(buffer, sizeof(buffer),
                 "local timezone: %Z = GMT %z",
                 &tm);
        info("%s", buffer);
        asctime_r(&tm, buffer);
        if (/*buffer &&*/ strlen(buffer) > 2) {
            buffer[strlen(buffer)-1] = NULL;
        }
        info("local time: %s", buffer);
        gmtime_r(&t, &tm);
        asctime_r(&tm, buffer);
        if (/*buffer &&*/ strlen(buffer) > 2) {
            buffer[strlen(buffer)-1] = NULL;
        }
        info("world time: %s", buffer);
    }

    if (redirectStderr && logFile) {
        if (fderr == -1) {
            // remember original stderr
            fderr = dup(2);
        }
        // overwrite stderr with log file fd,
        // closing the current stderr if necessary
        dup2(fileno(logFile), 2);
    } else {
        if (fderr != -1) {
            // restore original stderr
            dup2(fderr, 2);
        }
    }
}

POSIXLog::~POSIXLog() {
    if (logFile != NULL) {
        fclose(logFile);
    }
}

void POSIXLog::error(const char*  msg, ...) {
    va_list argList;
    va_start (argList, msg);
    printMessage(LOG_LEVEL_NONE, LOG_ERROR, msg, argList);
    va_end(argList);
    //Change made for US11215 on Mac
#ifdef FUN_MAC2
    CFNotificationCenterRef nCenter = CFNotificationCenterGetDistributedCenter();
    CFMutableDictionaryRef cfInfo =
    CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFNotificationCenterPostNotification(nCenter, CFSTR("Funambol_LogsFile_Updated"), NULL, cfInfo, TRUE);
    CFRelease(cfInfo);
#endif
}

void POSIXLog::info(const char*  msg, ...) {
    if (isLoggable(LOG_LEVEL_INFO)) {
        va_list argList;
        va_start (argList, msg);
        printMessage(LOG_LEVEL_INFO, LOG_INFO, msg, argList);
        va_end(argList);
        //Change made for US11215 on Mac
#ifdef FUN_MAC2
        CFNotificationCenterRef nCenter = CFNotificationCenterGetDistributedCenter();
        CFMutableDictionaryRef cfInfo =
        CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFNotificationCenterPostNotification(nCenter, CFSTR("Funambol_LogsFile_Updated"), NULL, cfInfo, TRUE);
        CFRelease(cfInfo);
#endif
    }
}

void POSIXLog::developer(const char*  msg, ...) {
    if (isLoggable(LOG_LEVEL_INFO)) {
        va_list argList;
        va_start (argList, msg);
        printMessage(LOG_LEVEL_DEBUG, LOG_DEBUG, msg, argList);
        va_end(argList);
        //Change made for US11215 on Mac
#ifdef FUN_MAC2
        CFNotificationCenterRef nCenter = CFNotificationCenterGetDistributedCenter();
        CFMutableDictionaryRef cfInfo =
        CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFNotificationCenterPostNotification(nCenter, CFSTR("Funambol_LogsFile_Updated"), NULL, cfInfo, TRUE);
        CFRelease(cfInfo);
#endif
    }
}

void POSIXLog::debug(const char*  msg, ...) {
    if (isLoggable(LOG_LEVEL_DEBUG)) {
        va_list argList;
        va_start (argList, msg);
        printMessage(LOG_LEVEL_DEBUG, LOG_DEBUG, msg, argList);
        va_end(argList);
        //Change made for US11215 on Mac
#ifdef FUN_MAC2
        CFNotificationCenterRef nCenter = CFNotificationCenterGetDistributedCenter();
        CFMutableDictionaryRef cfInfo =
        CFDictionaryCreateMutable(NULL, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFNotificationCenterPostNotification(nCenter, CFSTR("Funambol_LogsFile_Updated"), NULL, cfInfo, TRUE);
        CFRelease(cfInfo);
#endif
    }
}

void POSIXLog::printLine(bool firstLine,
                         time_t time,
                         const char *fullTime,
                         const char *shortTime,
                         const char *utcTime,
                         LogLevel level,
                         const char *levelPrefix,
                         const char *line)
{
    // Altough stdio does not need explicit synchronization, we need it because the file gets rotated/reset
    // and we must make sure this does not happen when another thread is writing
//    pthread_mutex_lock(&logMutex);
    
    FILE *out = getLogFile();
    if (!out) {
        out = stdout;
    }
    if (firstLine) {
        fprintf(out, "%s [%s] %s%s\n",
                logFile ? utcTime : shortTime,
                levelPrefix,
                getPrefix().c_str(),
                line);
    } else {
        fprintf(out, "[%s] %s%s\n",
                levelPrefix,
                getPrefix().c_str(),
                line);
    }
    fflush(out);
    
//    pthread_mutex_unlock(&logMutex);
}

void POSIXLog::printMessage(LogLevel level, const char* levelPrefix, const char* msg, va_list argList) {
    time_t t = time(NULL);
    struct tm sys_time;
    struct tm utc_time;
    char fullTime[64], shortTime[32];
    char utcTime[32];

    localtime_r(&t, &sys_time);
    gmtime_r(&t, &utc_time);

    strftime(fullTime, sizeof(fullTime), "%F %T GMT %z", &sys_time);
    strftime(shortTime, sizeof(shortTime), "%T", &sys_time);
    sprintf(utcTime, "%02d-%02d-%04d %02d:%02d:%02d GMT",
            1 + utc_time.tm_mon,
            utc_time.tm_mday,
            1900 + utc_time.tm_year,
            utc_time.tm_hour,
            utc_time.tm_min,
            utc_time.tm_sec);

    if (!logFileStdout && !logFile) {
         setLogFile(logPath, logName, logRedirectStderr);
    }

    StringBuffer buffer;
    buffer.vsprintf(msg, argList);
    const char *start = buffer.c_str();
    const char *eol = strchr(start, '\n');
    bool firstLine = true;
    while (eol) {
        /* hack: StringBuffer does not really allow write access, but do it anyway */
        *(char *)eol = 0;
        printLine(firstLine,
                  t,
                  fullTime,
                  shortTime,
                  utcTime,
                  level,
                  levelPrefix,
                  start);
        firstLine = false;
        *(char *)eol = '\n';
        start = eol + 1;
        eol = strchr(start, '\n');
    }
    printLine(firstLine,
              t,
              fullTime,
              shortTime,
              utcTime,
              level,
              levelPrefix,
              start);
}


void POSIXLog::reset(const char*  title) {
    setLogFile(logPath, logName, logRedirectStderr);

    if (logFile) {
//        pthread_mutex_lock(&logMutex);
        ftruncate(fileno(logFile), 0);
//        pthread_mutex_unlock(&logMutex);
    }
}


size_t POSIXLog::getLogSize() {
    size_t ret = 0;

    if (logFile) {
        ret = fgetsize(logFile);
        fclose(logFile);
        logFile = NULL;
    }
    return ret;
}


Log *Log::logger;

Log &Log::instance() {
    if (!logger) {
        logger = new POSIXLog();
    }
    return *logger;
}

