// 
// Funambol is a mobile platform developed by Funambol, Inc.
// Copyright (C) 2012 Funambol, Inc.
// 
// This program is a free software; you can redistribute it and/or modify it pursuant to
// the terms of the GNU Affero General Public License version 3 as published by
// the Free Software Foundation with the addition of the following provision
// added to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED
// WORK IN WHICH THE COPYRIGHT IS OWNED BY FUNAMBOL, FUNAMBOL DISCLAIMS THE
// WARRANTY OF NON INFRINGEMENT OF THIRD PARTY RIGHTS.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; INCLUDING ANY WARRANTY OF MERCHANTABILITY OR FITNESS
// FOR A PARTICULAR PURPOSE, TITLE, INTERFERENCE WITH QUITE ENJOYMENT. THE PROGRAM
// IS PROVIDED “AS IS” WITH ALL FAULTS. Refer to the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU Affero General Public License
// along with this program; if not, see http://www.gnu.org/licenses or write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA 02110-1301 USA.
// 
// You can contact Funambol, Inc. headquarters at 1065 East Hillsdale Blvd., Suite
// 400, Foster City, CA 94404, USA, or at email address info@funambol.com.
// 
// The interactive user interfaces in modified source and object code versions
// of this program must display Appropriate Legal Notices, pursuant to
// Section 5 of the GNU Affero General Public License version 3.
// 
// In accordance with Section 7(b) of the GNU Affero General Public License
// version 3, these Appropriate Legal Notices must retain the display of the
// "Powered by Funambol" logo. If the display of the logo is not reasonably
// feasible for technical reasons, the Appropriate Legal Notices must display
// the words "Powered by Funambol".
// 

#include "Database.h"
#include <iostream>

Database::Database(char* fileName) {
	db = NULL;
	open(fileName);
}

Database::~Database() {}

bool Database::open(char* fileName)
{
	if(sqlite3_open(fileName, &db) == SQLITE_OK)
		return true;
		
	return false;   
}

void Database::close()
{
	sqlite3_close(db);   
}

vector<vector<string>> Database::query(const char* queryString)
{
	sqlite3_stmt *stmt;
	vector<vector<string>> res;

	if(sqlite3_prepare_v2(db, queryString, -1, &stmt, 0) == SQLITE_OK) {
		int col = sqlite3_column_count(stmt);
		int ret = 0;
        while(1) {
			ret = sqlite3_step(stmt);			
			if (ret == SQLITE_ROW) {
				vector<string> values;
				for(int val = 0; val < col; val++) {
					values.push_back((char*)sqlite3_column_text(stmt, val));
				}
				res.push_back(values);
			}
			else {
				break;   
			}
		}	   
		sqlite3_finalize(stmt);
	}	
	string err = sqlite3_errmsg(db);
    if(err != "not an error") { // string provided by sqlite
        cout << queryString << " " << err << endl;
    }
	
	return res;  
}

