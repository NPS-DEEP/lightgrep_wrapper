# Author:  Bruce Allen
# Created: 2/25/2013
#
# The software provided here is released by the Naval Postgraduate
# School, an agency of the U.S. Department of Navy.  The software
# bears no warranty, either expressed or implied. NPS does not assume
# legal liability nor responsibility for a User's use of the software
# or the results of such use.
#
# Please note that within the United States, copyright protection,
# under Section 105 of the United States Code, Title 17, is not
# available for any work of the United States Government and/or for
# any works created by United States Government employees. User
# acknowledges that this software contains work which was created by
# NPS government employees and is therefore in the public domain and
# not subject to copyright.
#
# Released into the public domain on March 2, 2017 by Bruce Allen.

# This file assists in building libbe_scan.so

CLEANFILES = scan_email_flex.cpp

BE_SCAN_INCS = \
	artifact.cpp \
	be_scan.cpp \
	be_scan.hpp \
	extract_context.cpp \
	extract_context.hpp \
	flex_scan_exception.hpp \
	flex_extra_parameters.hpp \
	scan_email.cpp \
	scan_email_flex.flex \
	scan_email.hpp \
	scanner.hpp \
	scan_zip_gzip.cpp \
	scan_zip_gzip.hpp \
	uncompress_gzip.cpp \
	uncompress_zip.cpp

DEV_INCS = \
	scan_names.cpp \
	scan_names.hpp

lib_LTLIBRARIES = libbe_scan.la

AM_CFLAGS = $(BE_SCAN_CFLAGS)
AM_CXXFLAGS = $(BE_SCAN_CXXFLAGS)

#libbe_scan_la_LDFLAGS = -PIC -avoid-version -module 
libbe_scan_la_LDFLAGS = -PIC -avoid-version \
                        -export-dynamic -no-undefined

libbe_scan_la_SOURCES = $(BE_SCAN_INCS) $(DEV_INCS)

include_HEADERS = be_scan.hpp

.flex.cpp:
	$(LEX) -R -t $< > $@

