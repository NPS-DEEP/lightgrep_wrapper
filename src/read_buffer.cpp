// Author:  Bruce Allen
// Created: 3/2/2017
//
// The software provided here is released by the Naval Postgraduate
// School, an agency of the U.S. Department of Navy.  The software
// bears no warranty, either expressed or implied. NPS does not assume
// legal liability nor responsibility for a User's use of the software
// or the results of such use.
//
// Please note that within the United States, copyright protection,
// under Section 105 of the United States Code, Title 17, is not
// available for any work of the United States Government and/or for
// any works created by United States Government employees. User
// acknowledges that this software contains work which was created by
// NPS government employees and is therefore in the public domain and
// not subject to copyright.
//
// Released into the public domain on March 2, 2017 by Bruce Allen.

#include <config.h>
#include <string>
#include <sstream>
#include <stdint.h>
#include <iostream>
#include <cassert>

namespace lw {

  /*
   * Read bytes from buffer, possibly reading from previous_buffer.
   * Bytes that fall outside the bounds of the two buffers provided are
   * not returned.
   */
  std::string read_buffer(const size_t buffer_offset,
                          const char* const previous_buffer,
                          const size_t previous_buffer_size,
                          const char* const buffer,
                          const size_t buffer_size,
                          const size_t offset,
                          const size_t length,
                          const size_t padding) {

    // check for invalid input
    if (buffer_offset < previous_buffer_size) {
      std::cerr << "Error in lightgrep_wrapper read_buffer usage, "
                << "buffer_offset: " << buffer_offset
                << ", previous_buffer_size: " << previous_buffer_size
                << std::endl;
      return "";
    }

    // start offset of the previous buffer
    const size_t previous_buffer_offset = buffer_offset - previous_buffer_size;

    // requested start offset, non-negative
    size_t start_offset = (offset < padding) ? 0 : offset - padding;
    if (start_offset < previous_buffer_offset) {
      start_offset = previous_buffer_offset;
    }

    // requested end offset, one byte after end
    size_t end_offset = offset + length + padding;

    // build the return string
    std::stringstream ss;
    size_t start;
    size_t end;

    // add any part from previous_buffer
    if (start_offset < buffer_offset) {
      start = (start_offset < previous_buffer_offset)
               ? previous_buffer_offset : start_offset;
      end = (end_offset > buffer_offset) ? buffer_offset : end_offset;
      if (start < end) {
        ss << std::string(&previous_buffer[start - previous_buffer_offset],
                          end - start);
      }
    }

    // add any part from current buffer
    if (end_offset > buffer_offset) {
      start = (start_offset < buffer_offset) ? buffer_offset : start_offset;
      end = (end_offset > buffer_offset + buffer_size)
             ? buffer_offset + buffer_size : end_offset;
      if (start < end) {
        ss << std::string(&buffer[start - buffer_offset], end - start);
      }
    }

    return ss.str();
  }
}

