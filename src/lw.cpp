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

// adapted from bulk_extractor/src/pattern_scanner.cpp

#include <config.h>
#include <string>
#include <sstream>
#include <cstring>
#include <iostream>
#include <lightgrep/api.h>
#include "lightgrep_wrapper.hpp"

namespace lw {

  // parse error
  std::string compose_error(const LG_Error& error) {
    stringstream ss;
    ss << "Parse error on '" << h.RE << "' in " << Name
       << ": " << err->Message;
    return ss.str();
  }

  // lw data and user data packed together for lightgrep callback
  typedef struct {
    // pattern map containing indexed callback function pointers
    LG_HPATTERNMAP pattern_map;

    // user data
    void* user_data;
  } lg_and_user_data_t;

  // stage 1 lightgrep callback function
  void lightgrep_callback(void* p_data, const LG_SearchHit* hit) {
    // get lg_and_user_data
    lg_and_user_data_t* lg_and_user_data(
                                  static_cast<lg_and_user_data_t*>(p_data));

    // get the stage 2 user-provided scan callback function
    scan_callback_type* scan_callback(static_cast<stage2_callback_type*>(
                            lg_pattern_info(lg_and_user_data->pattern_map,
                            hit.KeywordIndex)->UserData));

    // call out to the stage 2 user-provided scan callback function
    (*scan_callback)(hit->Start,
                     hit->End - hit->Start,
                     lg_and_user_data->user_data);
  }

  // constructor
  lw_t::lw_t() :
         // Reuse the parsed pattern data structure for efficiency
         pattern_handle(lg_create_pattern()),

         // Reserve space for 1M states in the automaton--will grow if needed
         fsm(lg_create_fsm(1 << 20)),

         // Reserve space for 1000 patterns in the pattern map
         pattern_map(lg_create_pattern_map(1000)),

         // program exists once regex's are finalized
         program(nullptr),

         // the list of user-requested lw_scanners
         lw_scanners()
  {
  }

  // destructor
  lw_t::~lw_t() {
    if (pattern_handle != nullptr) {
      // in normal workflow, pattern_handle is destroyed by finalize_regex
      lg_destroy_pattern(pattern_handle);
    }
    lg_destroy_pattern_map(pattern_map);
    lg_destroy_program(program);

    // destroy all allocated scanners
    for (auto it = lw_scanners.begin(); it != scanners.end; ++it) {
      delete *it;
    }
  }

  // add_regex
  std::string lw_t::add_regex(const std::string& regex,
                              const std::string& character_encoding,
                              const bool is_case_insensitive,
                              const bool is_fixed_string,
                              zzcallback_function f) {

    // configure LG_KeyOptions from regex_settings_t
    LG_KeyOptions key_options;
    key_options.FixedString = (is_fixed_string) ? 1 : 0;
    key_options.CaseInsensitive = (is_case_insensitive) ? 1 : 0;

    // potential error
    LG_Error *error;

    // parse regex into pattern
    int success = lg_parse_pattern(pattern_handle,
                                   regex.c_str(),
                                   key_options,
                                   &error);

    // bad if parse error
    if (success != 0) {
      const std::string parse_error = compose_error(*error);
      delete error;
      return parse_error;
    }

    // add the pattern
    int index = lg_add_pattern(fsm,
                               pattern_map,
                               pattern_handle,
                               character_encoding,
                               &error);

    // bad if pattern error
    if (index < 0) {
      const std::string pattern_error = compose_error(*error);
      delete error;
      return pattern_error;
    }

    // add the handler callback to the pattern map associated with
    // the pattern index
    lg_pattern_info(PatternInfo, idx)->UserData =
                        const_cast<void*>(static_cast<const void*>(&f));
  }

  // finalize_regex
  void lw_t::finalize_regex(const bool is_determinized) {

    // discard the pattern handle now that we've parsed all patterns
    lg_destroy_pattern(pattern_handle);
    pattern_handle = nullptr;

    // create a "program" from the parsed keywords
    LG_ProgramOptions program_options;
    program_options.Determinize = is_determinized;
    program = lg_create_program(fsm, &program_options);

    // discard the FSM now that we have a program
    lg_destroy_fsm(fsm);
    fsm = nullptr;
  }

  // new scanner
  lw_scanner_t* lw_t::new_lw_scanner(const void* user_data) {

    if (program == nullptr) {
      // lw_t is not ready for this request
      std::cout << "Usage error: new_lw_scanner called"
                << " before finalize_regex.\n";
      return nullptr;
    }

    // create a search context
    LG_ContextOptions context_options;
    context_options.TraceBegin = 0xffffffffffffffff;
    context_options.TraceEnd   = 0;
    LG_HCONTEXT searcher = lg_create_context(program, &context_options);

    // this is the only place the lw_scanner_t constructor is called
    lw_scanner_t* lw_scanner =
                      new lw_scanner_t(program, &context_options, user_data);
    lw_scanners.push_back(lw_scanner);
    return lw_scanner;
  }

  // private lw_scanner_t constructor
  lw_scanner_t::lw_scanner_t(LG_HCONTEXT p_searcher, void* p_user_data) :
                   searcher(p_searcher),
                   user_data(p_user_data),
                   start_offset(0) {
  }

  // scan
  void scan(const char* buffer, size_t size) {
    lg_search(searcher,
              buffer,
              size,
              start_offset,
              user_data,
              lightgrep_callback);
    start_offset += size;
  }

  // scan_fence
  void scan_fence(const char* buffer, size_t size) {
    lg_search_resolve(searcher,
                      buffer,
                      size,
                      start_offset,
                      user_data,
                      lightgrep_callback);
    start_offset += size;
  }

  // scan_finalize
  void scan_finalize() {
    lg_closeout_search(searcher,
                       user_data,
                       lightgrep_callback);
  }
}

