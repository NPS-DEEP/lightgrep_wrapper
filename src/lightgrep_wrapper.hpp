// Author:  Bruce Allen
// Created: 5/26/2017
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
// Released into the public domain on May 26, 2017 by Bruce Allen.

/**
 * \file
 * Header file for the lightgrep_wrapper library.
 *
 * Usage:
 *     1 - Obtain a scanner_program_t instance.
 *     2 - Add regular expression definitions.
 *     3 - When done adding definitions, finalize your scanner program.
 *     4 - Once finalized, create lw_scanner_t instances to scan data for
           the regular expressions you compiled in your scanner program.
 *         Scanners are threadsafe, so get one per CPU for parallelization.
 *     5 - Use lw_scanner_t interfaces to scan data for matches to regular
 *         expressions.  Use scan to scan a stream of data in buffer
 *         intervals.  Use scan_finalize scans for final matches.  Use
 *         scan_fence if you need to capture matches that start before
 *         the fence but span across it.
 */

#ifndef LIGHTGREP_WRAPPER_HPP
#define LIGHTGREP_WRAPPER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <lightgrep/api.h>

/**
 * The version of lightgrep_wrapper.
 */
extern "C"
const char* lightgrep_wrapper_version();

/**
 * This is the typedef for user-provided scan callback functions with
 * user data.
 *
 * The function returns the start and size of the match data and a pointer
 * to your user data which you provided when you created your scanner
 * instance.
 *
 * Parameters:
 *   start - Start offset of the scan hit with respect to the beginning
 *           of the scan stream.
 *   size - The size of the scan hit data.
 *   user_data - The user data provided to lw_scanner_t.  Use this to
 *           save information about matches.
 */
typedef void (*scan_callback_function_t)(const uint64_t start,
                                         const uint64_t size,
                                         void* user_data);

namespace lw {

  // internal support structure
  typedef std::vector<scan_callback_function_t> function_pointers_t;
//  typedef std::pair<function_pointers_t*, void*> data_pair_t;
  class data_pair_t {
    public:
    const function_pointers_t* function_pointers;
    void* user_data;
    data_pair_t(const function_pointers_t* p_function_pointers,
                void* p_user_data);
  };

  /**
   * Build a scanner program instance to provide to your scanner.
   */
  class lw_scanner_program_t {

    // the scanner accesses the program handle and function pointers
    friend class lw_scanner_t;

    private:
    LG_HPATTERN     pattern_handle;
    LG_HFSM         fsm;
    LG_HPATTERNMAP  pattern_map;
    LG_HPROGRAM     program;

    // the scan callback function pointers
    std::vector<scan_callback_function_t> function_pointers;

    // do not allow copy or assignment
    lw_scanner_program_t(const lw_scanner_program_t&) = delete;
    lw_scanner_program_t& operator=(const lw_scanner_program_t&) = delete;

    public:
    /**
     * Begin a scanner program instance.
     */
    lw_scanner_program_t();

    /**
     * Release resources.
     */
    ~lw_scanner_program_t();

    /**
     * Add a regular expression to scan for.
     *
     * Parameters:
     *   regex - The regular expression text.
     *   character_encoding - Encoding, for example UTF-8, UTF-16LE.
     *   is_case_insensitive - Select upper/lower case insensitivity.
     *   is_fixed_string - false=grep, true=fixed-string.  Use false.
     *   callback_function - The function to call to service hits associated
     *                       with this regular expression.
     *
     * Returns:
     *   "" if accepted else error text on failure.
     */
    std::string add_regex(const std::string& regex,
                          const std::string& character_encoding,
                          const bool is_case_insensitive,
                          const bool is_fixed_string,
                          scan_callback_function_t f);

    /**
     * Finalize the regular expression scanner program used for scanning.
     * Once finalized, the program becomes valid, cannot be changed, and
     * may be provided to scanners,
     *
     * Parameters:
     *   is_determinized - false=NFA, true=DFA(pseudo).  Use false.  See
     *                     liblightgrep.
     */
    void finalize_program(bool is_determinized);
  };

  /**
   * A scanner instance that you can use for scanning.
   */
  class lw_scanner_t {

    private:
    const LG_ContextOptions context_options;
    const LG_HCONTEXT searcher;
    data_pair_t data_pair;

    // do not allow copy or assignment
    lw_scanner_t(const lw_scanner_t&) = delete;
    lw_scanner_t& operator=(const lw_scanner_t&) = delete;

    public:

    /**
     * True when the scanner program has been finalized.  The scanner will
     * fail if the scanner program has not been finalized.
     */
    const bool program_is_finalized;

    /**
     * Instantiate a scanner instance that you can use for scanning.
     * Fails with assert if scanner_program has not been finalized.
     *
     * Parameters:
     *   scanner_program - The scanner prrogram containing the regex program
     *           and the function callbacks that the scanner will use.
     *   user_data - The user data that this scanner will use.  Use it to
     *           save information about matches.
     */
    lw_scanner_t(const lw_scanner_program_t& scanner_program,
                 void* user_data);

    ~lw_scanner_t();

    /**
     * Scan bytes of data from a buffer.  Call repeatedly, as needed,
     * to scan a data stream that is larger than your buffer.
     *
     * Parameters:
     *   stream_offset - The offset into the stream to the start of the buffer.
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   match.
     */
    void scan(uint64_t stream_offset, const char* const buffer, size_t size);

    /**
     * End scanning, accepting any active hits that are valid.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   active match that is valid.  This can happen when a match being
     *   tracked could have been longer if there were more data.
     */
    void scan_finalize();

    /**
     * Scan into more bytes of data in order to find matches that started
     * before the fence but span across it, then end scanning, accepting
     * any hits that are valid.
     *
     * Parameters:
     *   stream_offset - The offset into the stream to the start of the buffer.
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   match that started before the fence.
     */
    void scan_fence_finalize(uint64_t stream_offset,
                             const char* const buffer, size_t size);
  };

  /**
   * This convenience function provides a read service for reading
   * match data in a streaming context.  You provide the buffer
   * and the previous buffer to read from, the offset to the buffer,
   * the offset and length of where to read, and any requested
   * additional padding to read.  The returned data may be smaller
   * than requested if the span requested is outside the bounds of
   * the two buffers you provide.
   *
   * Two buffers are provided to support stream scanning, where scans
   * started in one buffer may not be identified until scanning
   * in the next buffer.  It is your responsibility to provide a large
   * enough previous buffer in order to read all requested bytes.
   *
   * Parameters:
   *   buffer_offset - The offset to your buffer.
   *   previous_buffer - The buffer adjacent but before your buffer.
   *   previous_buffer_size - The size, in bytes, of your previous buffer.
   *   buffer - The buffer that backs your data being scanned.
   *   buffer_size - The size, in bytes, of your buffer.
   *   offset - The offset to the data to read.
   *   length - The length, in bytes, of the data to read.
   *   padding - Padding, in bytes, before and after the data to read,
   *             or 0 for no padding.
   *
   * Returns:
   *   The data from the buffers, which may be incomplete if the buffers
   *   you provide do not sufficiently back the read you request.
   */
  std::string read_buffer(const size_t buffer_offset,
                          const char* const previous_buffer,
                          const size_t previous_buffer_size,
                          const char* const buffer,
                          const size_t buffer_size,
                          const size_t offset,
                          const size_t length,
                          const size_t padding);
}

#endif

