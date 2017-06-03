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
 *     1 - Obtain a lw_t instance.
 *     2 - Add regular expression definitions.
 *     3 - When done adding definitions, finalize them before scanning.
 *     4 - Once finalized, get lw_scanner_t instances to use to scan data.
 *         Scanners are threadsafe, so get one per CPU for parallelization.
 *     5 - Use scanner instances to scan data using scan, scan_fence, and
 *         scan_finalize interfaces.  scan_finalize scans for final
 *         matches and then resets the scanner by resetting the stream
 *         offset count back to 0.  Use scan_fence if you need to capture
 *         matches that start before the fence but span across it.
 */

#ifndef LIGHTGREP_WRAPPER_HPP
#define LIGHTGREP_WRAPPER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>
#include <lightgrep/api.h>

/**
 * the typedef for user-provided scan callback functions with user data.
 *
 * The function returns the start and size of the match data and the
 * user data you provided when you created your scanner instance.
 *
 * Parameters:
 *   start - Start offset of the scan hit with respect to the beginning
 *           of the scan stream.
 *   size - The size of the scan hit data.
 *   user_data - The user data provided to lw_t::new_lw_scanner
 */
typedef void (*scan_callback_function_t)(const uint64_t start,
                                         const uint64_t size,
                                         void* user_data);

namespace lw {

  class lw_scanner_t;
  typedef std::vector<scan_callback_function_t> function_pointers_t;
  typedef std::pair<function_pointers_t*, void*> data_pair_t;

  /**
   * The lightgrep wrapper.
   */
  class lw_t {

    private:
    LG_HPATTERN     pattern_handle;
    LG_HFSM         fsm;
    LG_HPATTERNMAP  pattern_map;
    LG_HPROGRAM     program;

    // the scan callback function pointers
    std::vector<scan_callback_function_t> function_pointers;

    // lw_scanners
    std::vector<lw_scanner_t*> lw_scanners;

    // do not allow copy or assignment
    lw_t(const lw_t&) = delete;
    lw_t& operator=(const lw_t&) = delete;

    public:
    /**
     * Create a lightgrep wrapper object to use for building a scan program
     * and obtaining scanners.
     */
    lw_t();

    /**
     * Release resources.
     */
    ~lw_t();

    /**
     * Add a regular expression definition to scan for.
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
     * Finalize the regular expression engine so it can be used for scanning.
     *
     * Parameters:
     *   is_determinized - false=NFA, true=DFA(pseudo).  Use false.
     */
    void finalize_regex(const bool is_determinized);

    /**
     * Get a scanner to scan data with.  Get one for each CPU, they are
     * threadsafe.  Be sure to use your user_data in a threadsafe way.
     * Returns nullprt if called before calling finalize_regex().
     *
     * Do not delete these.  Let lw_t do this.  Re-use these instead of
     * creating more than you need.
     *
     * Parameters:
     *   user_data - User data for your callback function provided in
     *               the add_regex function.
     *
     * Returns:
     *   A lw_scanner instance to scan data with.
     */
    lw_scanner_t* new_lw_scanner(void* user_data);
  };

  /**
   * A lw_scanner instance.
   */
  class lw_scanner_t {

    // lw_t uses the lw_scanner_t constructor
    friend class lw_t;

    private:
    const LG_HCONTEXT searcher;
    data_pair_t data_pair;
    uint64_t start_offset;

    lw_scanner_t(const LG_HCONTEXT p_searcher,
                 const LG_ContextOptions& p_context_options,
                 function_pointers_t& function_pointers,
                 void* user_data);

    // do not allow copy or assignment
    lw_scanner_t(const lw_scanner_t&) = delete;
    lw_scanner_t& operator=(const lw_scanner_t&) = delete;

    public:

    /**
     * Scan bytes of data from a buffer.  Call repeatedly, as needed,
     * to scan a data stream that is larger than your buffer.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   match.
     */
    void scan(const char* const buffer, size_t size);

    /**
     * End scanning, accepting any active hits that are valid.  The stream
     * counter is reset so that the scanner may be used again.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   active match that is valid.  This can happen when a match could
     *   have been longer if there were more data.
     */
    void scan_finalize();

    /**
     * Scan into more bytes of data in order to find matches that started
     * before the fence but span across it, then end scanning, accepting
     * any hits that are valid.  When done, the stream counter is reset
     * so that the scanner may be used again.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   match that started before the fence.
     */
    void scan_fence_finalize(const char* const buffer, size_t size);
  };
}

#endif

