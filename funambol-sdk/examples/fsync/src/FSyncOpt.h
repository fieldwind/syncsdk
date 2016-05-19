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

#ifndef INCL_FSYNCOPT
#define INCL_FSYNCOPT

#include "base/fscapi.h"
#include "base/util/StringMap.h"
#include "client/OptionParser.h"

USE_NAMESPACE

typedef enum VerboseLevel{
        QUIET   = -1,
        NORMAL  = 0,
        VERBOSE = 1
} VerboseLevel;

/**
 * This class allows to get options from the command line.
 */
class FSyncOpt
{
    public:
        /** Default constructor */
        FSyncOpt(const char *progname);

        /** parse the command line */
        bool parseCmdline(int argc, char** argv);

        /** Get parser error */
        const char *getErr() const { return parser.getErrMsg(); }

        /** Check option presence */
        bool optionSet(const char *ln) { return opts.get(ln) ? true : false; }

        /** Get options value */
        const char* getOptionVal(const char *ln);

        VerboseLevel getVerbosity() const { return verbose; } 

    private:
        /** The command line parser */
        OptionParser parser;
        StringMap opts;
        ArrayList args;
        
        /** The verbosity level */
        VerboseLevel verbose;
};


#endif
