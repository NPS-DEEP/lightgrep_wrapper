# Author:  Bruce Allen
# Created: 5/26/2017
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
# Released into the public domain on May 26, 2017 by Bruce Allen.

# builds liblightgrep_wrapper.so

LW_INCS = \
	lightgrep_wrapper.cpp \
	read_buffer.cpp \
	lightgrep_wrapper.hpp

lib_LTLIBRARIES = liblightgrep_wrapper.la

AM_CFLAGS = $(LW_CFLAGS)
AM_CXXFLAGS = $(LW_CXXFLAGS)

liblightgrep_wrapper_la_LDFLAGS = $(BOOST_LDFLAGS) -pthread

liblightgrep_wrapper_la_SOURCES = $(LW_INCS)

include_HEADERS = lightgrep_wrapper.hpp

