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

#include "base/fscapi.h"

#include "base/adapter/PlatformAdapter.h"
#include "base/util/StringBuffer.h"
#include "base/util/utils.h"
#include "base/Log.h"
#include <iostream>
#include "Database.h"

#ifndef EXIT_SUCCESS
#	define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#	define EXIT_FAILURE 1
#endif

USE_NAMESPACE
using namespace std;

static const char *progname = "winapp";

/**
 * Send message through a named pipe
 */
void testNamedPipe() {

    HANDLE hFile;
    BOOL flg;
    DWORD dwWrite;
    
    // using wchar_t
    wchar_t message1[512];
    memset(message1, 0, 512*sizeof(wchar_t));
    wcscpy(message1, TEXT("Message from c++ project"));
    
    // using std
    std::wstring end = TEXT("{cheers}");

    /* NOT WORKING PROPERLY */
    /*
    LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\maverick\\push");
    CHAR chReadBuf[1024];
    DWORD cbRead;
    BOOL fResult;  

    fResult = CallNamedPipe( 
        lpszPipename,          // pipe name 
        message1,              // message to server 
        wcslen(message1) * sizeof(wchar_t),      // message length 
        chReadBuf,             // buffer to receive reply 
        sizeof(chReadBuf),     // size of read buffer 
        &cbRead,               // number of bytes read 
        NMPWAIT_NOWAIT); // wait;-) 
    
    const wchar_t* m = end.c_str();
    fResult = CallNamedPipe( 
        lpszPipename,          // pipe name 
        (LPVOID)m,              // message to server 
        end.length() * sizeof(wchar_t),      // message length 
        chReadBuf,             // buffer to receive reply 
        sizeof(chReadBuf),     // size of read buffer 
        &cbRead,               // number of bytes read 
        NMPWAIT_NOWAIT); // wait;-) 
    */

    hFile = CreateFile(L"\\\\.\\pipe\\maverick\\push", GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING,
                             0, NULL);
    
    if(hFile == INVALID_HANDLE_VALUE)
    { 
        DWORD dw = GetLastError();
        printf("CreateFile failed for Named Pipe client\n:" );
    }
    else
    {
        flg = WriteFile(hFile, message1, wcslen(message1) * sizeof(wchar_t), &dwWrite, NULL);
        if (FALSE == flg)
        {
            printf("WriteFile failed for Named Pipe client\n");
        }
        else
        {
            printf("WriteFile succeeded for Named Pipe client\n");
        }

        // closing message
        flg = WriteFile(hFile, end.c_str(), end.length() * sizeof(wchar_t), &dwWrite, NULL);
        CloseHandle(hFile);
    }

}

/**
 * Test the creation of a test database
 */
int testSqliteDatabase() {
    Database *db;
    db = new Database("Database.sqlite");
    db->query("CREATE TABLE a (a INTEGER, b INTEGER);");
    db->query("INSERT INTO a VALUES(1, 2);");
    db->query("INSERT INTO a VALUES(5, 4);");
    vector<vector<string> > result = db->query("SELECT a, b FROM a;");
    for(vector<vector<string> >::iterator it = result.begin(); it < result.end(); ++it)
    {
	    vector<string> row = *it;
	    cout << "Values: (A=" << row.at(0) << ", B=" << row.at(1) << ")" << endl;
    }
    db->close();
    return 0;
}



#define WINAPP_APPLICATION_URI         "Funambol/winapp"

//------------------------------------------------------------------------ Main
int main(int argc, char** argv) 
{
    // create a fake database just to test sqlite integration
    testSqliteDatabase();
    
    // test a named pipe communication
    // testNamedPipe();
    
    PlatformAdapter::init(WINAPP_APPLICATION_URI);
   
    StringBuffer rep("Test running...");
    LOG.info("\n%s", rep.c_str());
    
    std::string test("Second test...");
    LOG.info("\n%s", test.c_str());
    
    exit(EXIT_SUCCESS);
}

