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
 */

#ifndef LIGHTGREP_WRAPPER_HPP
#define LIGHTGREP_WRAPPER_HPP

#include <string>
#include <stdint.h>

namespace lw {

  /**
   * Configure regular expression settings as desired.
   */
  class regex_settings_t {
    public:
    const std::string encoding;
    const bool is_case_insensitive;
    const bool is_fixed_string;
  }

  /**
   * The lightgrep wrapper singleton.  Usage:
   *     - Add regular expression definitions.
   *     - When done adding definitions, finalize them before scanning.
   *     - Once finalized, get threadsafe scanners and use them to scan data.
   */
  class lw_t {
    private:

    public:
    /**
     * Get the Lightgrep Wrapper singleton.
     *
     * Returns:
     *  The Lightgrep Wrapper singleton.
     */
    static lw_t* get();

    /**
     * Add a regular expression definition to scan for.
     *
     * Parameters:
     *   regex - The regular expression text.
     *   regex_settings - Regular expression settings to apply to this
     *                    regular expression.
     *   callback_function - The function to call to service hits associated
     *                       with this regular expression.
     *
     * Returns:
     *   "" if accepted else error text on failure.
     */
    std::string add_regex(const std::string& regex,
                          const regex_settings_t& regex_settings,
                          zzcallback_function f);

    /**
     * Finalize the regular expression engine so it can be used for scanning.
     */
    void finalize_regex();

    /**
     * Get a scanner to scan data with.  Get one for each CPU, they are
     * threadsafe.  Returns nullprt if called before calling finalize_regex().
     *
     * Returns:
     *   A scanner instance to scan data with.  Threadsafe.  Delete when
     *   done to avoid a memory leak.
     */
    lw_scanner_t* new_scanner();

    /**
     * Deallocates all resources including any open scanners.
     */
    static void close();

  };

  /**
   * A threadsafe scanner instance.  Delete it when done to avoid a
   * memory leak.
   */
  class lw_scanner_t {
    private:

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
     *   match hit.
     */
    void scan(char* buffer, size_t count);

    /**
     * Scan into more bytes of data in order to find matches that started
     * before the fence but span across it.  Potential hits across the
     * fence are not tracked.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   match hit that started before the fence.
     */
    void scan_fence(char* buffer, size_t count);

    /**
     * End scanning, accepting any active hits that are valid.  The scanner
     * is reset so that it may be used again.
     *
     * Parameters:
     *   buffer - The buffer to scan.
     *   size - The size, in bytes, of the buffer to scan.
     *
     * Returns:
     *   Nothing, but the associated callback function is called for each
     *   active match hit that is valid.  This can happen when a match
     *   could have been longer if there were more data.
     */
    void scan_finalize();
  };
}

#endif

