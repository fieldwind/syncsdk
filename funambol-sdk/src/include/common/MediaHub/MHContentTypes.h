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

#ifndef INCL_MH_CONTENT_TYPE
#define INCL_MH_CONTENT_TYPE
/** @cond DEV */

#include "base/fscapi.h"


BEGIN_FUNAMBOL_NAMESPACE

/**
 *
 */
class MHContentType {
    
public:

    /**
    * Static method to retrieve the content-type of a file given the extension.
    * @param val - the extension to retrieve the content-type, i.e. "jpg"
    * @return the content type associated to the extension i.e. image/ipeg
    */
    static StringBuffer getContentTypeByExtension(const char* val) {        

        if (val == NULL || strcmp(val, "") == 0) {
            return "application/octet-stream";
        }

        StringBuffer value(val);
        size_t pos = value.find(".");
        
        if (pos != StringBuffer::npos && pos > 0) { // at least the extension must be .something otherwise is not valid
            return "application/octet-stream";
        }
        if (pos == 0 && value.length() >= 2) {
            value = value.substr(1);
        }
        
        value.lowerCase();
        
        //
        // PICTURES
        //
        if (value == "bmp")  { return "image/bmp"               ; }     
        if (value == "cod" ) { return "image/cis-cod"           ; }     
        if (value == "gif" ) { return "image/gif"               ; }     
        if (value == "ief" ) { return "image/ief"               ; }     
        if (value == "jpe" ) { return "image/jpeg"              ; }     
        if (value == "jpeg") { return "image/jpeg"              ; }     
        if (value == "jpg" ) { return "image/jpeg"              ; }
        if (value == "png" ) { return "image/png"               ; }     
        if (value == "jfif") { return "image/pipeg"             ; }     
        if (value == "svg" ) { return "image/svg+xml"           ; }     
        if (value == "tif" ) { return "image/tiff"              ; }     
        if (value == "tiff") { return "image/tiff"              ; }     
        if (value == "ras" ) { return "image/x-cmu-raster"      ; }     
        if (value == "cmx" ) { return "image/x-cmx"             ; }     
        if (value == "ico" ) { return "image/x-icon"            ; }     
        if (value == "pnm" ) { return "image/x-portable-anymap" ; }     
        if (value == "pbm" ) { return "image/x-portable-bitmap" ; }     
        if (value == "pgm" ) { return "image/x-portable-graymap"; }     
        if (value == "ppm" ) { return "image/x-portable-pixmap" ; }     
        if (value == "rgb" ) { return "image/x-rgb"             ; }     
        if (value == "xbm" ) { return "image/x-xbitmap"         ; }     
        if (value == "xpm" ) { return "image/x-xpixmap"         ; }     
        if (value == "xwd" ) { return "image/x-xwindowdump"     ; }     
        
        //
        // VIDEO
        //
        
        if (value == "mp2"   ) { return "video/mpeg"            ; }
        if (value == "mpa"   ) { return "video/mpeg"            ; }
        if (value == "mpe"   ) { return "video/mpeg"            ; }
        if (value == "mpeg"  ) { return "video/mpeg"            ; }
        if (value == "mpg"   ) { return "video/mpeg"            ; }
        if (value == "mpv2"  ) { return "video/mpeg"            ; }
        if (value == "mov"   ) { return "video/quicktime"       ; }
        if (value == "qt"    ) { return "video/quicktime"       ; }
        if (value == "lsf"   ) { return "video/x-la-asf"        ; }
        if (value == "lsx"   ) { return "video/x-la-asf"        ; }
        if (value == "asf"   ) { return "video/x-ms-asf"        ; }
        if (value == "asr"   ) { return "video/x-ms-asf"        ; }
        if (value == "asx"   ) { return "video/x-ms-asf"        ; }
        if (value == "avi"   ) { return "video/x-msvideo"       ; }
        if (value == "movie" ) { return "video/x-sgi-movie"     ; }
        if (value == "dif"   ) { return "video/x-dv"            ; }
        if (value == "dv"    ) { return "video/x-dv"            ; }
        if (value == "m4u"   ) { return "video/vnd.mpegurl"     ; }
        if (value == "m4v"   ) { return "video/x-m4v"           ; }
        if (value == "mp4"   ) { return "video/mp4"             ; }
        if (value == "mxu"   ) { return "video/vnd.mpegurl"     ; }
        if (value == "3g2"   ) { return "video/3gpp2"           ; }
        if (value == "3gp"   ) { return "video/3gpp"            ; }
        if (value == "3gpp"  ) { return "video/3gpp"            ; }
        if (value == "swf"   ) { return "application/x-shockwave-flash"; }
        if (value == "flv"   ) { return "video/x-flv"           ; }
       
        //
        // FILES
        // values taken from /etc/mime.types
        //
        if (value == "ez"    ) { return "application/andrew-inset"; }
        if (value == "anx"   ) { return "application/annodex";      }
        if (value == "jar"   ) { return "application/java-archive"; }
        if ((value == "doc"  ) ||
            (value == "docx"  )) { return "application/msword";       }
        if (value == "mxf"   ) { return "application/mxf";          }
        if (value == "oda"   ) { return "application/oda";          }
        if (value == "ogx"   ) { return "application/ogg";          }
        if (value == "pdf"   ) { return "application/pdf";          }
        if (value == "pgp"   ) { return "application/pgp-signature";}
        if ((value == "ps"   ) || (value == "eps"))  
                               { return "application/postscript";   }
        if (value == "rar"   ) { return "application/rar";          }
        if (value == "rdf"   ) { return "application/rdf+xml";      }
        if (value == "rss"   ) { return "application/rss+xml";      }
        if (value == "rtf"   ) { return "application/rtf";          }
        if ((value == "xhtml") || (value == "xht" )) 
                               { return "application/xhtml+xml";    }
        if ((value == "xml") || (value == "xsl") || (value == "xsd")) 
                               { return "application/xml";          }
        if (value == "zip"   ) { return "application/zip";           }
        if (value == "apk"   ) { return "application/vnd.android.package-archive"; }
        if (value == "cdy"   ) { return "application/vnd.cinderella"; }
        if (value == "kml"   ) { return "application/vnd.google-earth.kml+xml"; }
        if (value == "kmz"   ) { return "application/vnd.google-earth.kmz"; }		
        if ((value == "xls"  ) || (value == "xlb") || (value == "xlt") ||
            (value == "ods"  ) || (value == "ots") || (value == "sxc") || 
            (value == "stc"  ) || (value == "xlw") || (value == "xlsx")||
            (value == "xlsm" ) || (value == "xlts")) 
                            { return "application/vnd.ms-excel";     }

        if ((value == "ppt"  ) || (value == "pps") ||(value == "pot")   || 
            (value == "pptx" ) || (value == "pptm") ||
            (value == "potx" ) || (value == "potm") || (value == "ppsx") ||
            (value == "odp"  ) || (value == "sxi"))
                                { return "application/vnd.ms-powerpoint"; }

        if (value == "xlsx"  ) { return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"; }
        if (value == "xltx"  ) { return "application/vnd.openxmlformats-officedocument.spreadsheetml.template"; }
        if (value == "pptx"  ) { return "application/vnd.openxmlformats-officedocument.presentationml.presentation"; }
        if (value == "ppsx"  ) { return "application/vnd.openxmlformats-officedocument.presentationml.slideshow"; }
        if (value == "potx"  ) { return "application/vnd.openxmlformats-officedocument.presentationml.template"; }
        if (value == "docx"  ) { return "application/vnd.openxmlformats-officedocument.wordprocessingml.document"; }
        if (value == "dotx"  ) { return "application/vnd.openxmlformats-officedocument.wordprocessingml.template"; }
        if (value == "sis"   ) { return "application/vnd.symbian.install"; }
        if (value == "vsd"   ) { return "application/vnd.visio"; }
        if (value == "wbxml" ) { return "application/vnd.wap.wbxml"; }
        if (value == "7z"    ) { return "application/x-7z-compressed"; }
        if (value == "abw"   ) { return "application/x-abiword"; }
        if (value == "dmg"   ) { return "application/x-apple-diskimage"; }
        if (value == "bcpio" ) { return "application/x-bcpio"; }
        if (value == "torrent") { return "application/x-bittorrent"; }
        if ((value == "deb") || (value == "udeb")) 
                               { return "application/x-debian-package"; }
        if (value == "iso"   ) { return "application/x-iso9660-image"; }
        if (value == "latex" ) { return "application/x-latex"; }
        if (value == "wmd"   ) { return "application/x-ms-wmd"; }
        if (value == "wmz"   ) { return "application/x-ms-wmz"; }
        if ((value == "com"  ) || (value == "exe") || (value == "bat") || (value == "dll")) 
                               { return "application/x-msdos-program"; }
        if (value == "msi"   ) { return "application/x-msi"; }
        if (value == "rpm"   ) { return "application/x-redhat-package-manager"; }
        if (value == "sh"    ) { return "application/x-sh"; }
        if (value == "shar"  ) { return "application/x-shar"; }
        if ((value == "tar"  ) || (value == "tgz")) 
                               { return "application/x-tar" ; }
        if ((value == "texinfo") || (value == "texi")) 
                                { return "application/x-texinfo"; }
        if (value == "m3u"   ) { return "audio/mpegurl"; }	
        if ((value == "oga"  ) || (value == "ogg") || (value == "spx")) 
                               { return "audio/ogg"; }
        if (value == "css"   ) { return "text/css"; }
        if (value == "csv"   ) { return "text/csv"; }
        if ((value == "html" ) || (value == "htm ") || (value == "shtml")) 
                               { return "text/html"; }	

        if ((value == "txt"  ) || (value == "text") || (value == "pot")  || (value == "brf")) 
                               { return "text/plain"; }

        if (value == "rtx"   ) { return "text/richtexta"; }
        if ((value == "h++"  ) || (value == "hpp") || (value == "hxx") || (value == "hh")) 
                               { return "text/x-c++hdr"; }	
        if ((value == "c++"  ) || (value == "cpp") || (value == "cxx") || (value == "cc")) 
                               { return "text/x-c++src"; }
        if (value == "h"     ) { return "text/x-chdr"; }
        if ((value == "diff" ) || (value == "patch")) 
                               { return "text/x-diff"; }
        if (value == "java"  ) { return "text/x-java"; }
        if (value == "sisx"  ) { return "x-epoc/x-sisx-app"; }
        
        // MUSIC
        if (value == "mp3"   ) { return "audio/mp3"; }
        if (value == "m4a"   ) { return "audio/m4a"; }

        return "application/octet-stream";                                                      
    }

};

END_NAMESPACE

/** @endcond */
#endif
