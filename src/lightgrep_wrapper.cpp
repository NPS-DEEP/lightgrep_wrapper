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
#include <cassert>
#include <lightgrep/api.h>
#include "lightgrep_wrapper.hpp"

namespace lw {

  // parse error
  std::string compose_error(const std::string& regex, const LG_Error& error) {
    std::stringstream ss;
    ss << "Parse error in expression '" << regex << "': " << error.Message;
    return ss.str();
  }

  // stage 1 lightgrep callback function
  void lightgrep_callback(void* p_data_pair, const LG_SearchHit* hit) {

    // get data pair
    data_pair_t* data_pair(static_cast<data_pair_t*>(p_data_pair));

    // get the stage 2 user-provided scan callback function
    scan_callback_function_t f = data_pair->first->at(hit->KeywordIndex);

    // call out to the stage 2 user-provided scan callback function
    (*f)(hit->Start,
         hit->End - hit->Start,
         data_pair->second);
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

         // the list of scan callback function pointers
         function_pointers(),

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
    for (auto it = lw_scanners.begin(); it != lw_scanners.end(); ++it) {
      delete *it;
    }
  }

  // add_regex
  std::string lw_t::add_regex(const std::string& regex,
                              const std::string& character_encoding,
                              const bool is_case_insensitive,
                              const bool is_fixed_string,
                              const scan_callback_function_t f) {

    // configure LG_KeyOptions from regex_settings_t
    LG_KeyOptions key_options;
    key_options.FixedString = (is_fixed_string) ? 1 : 0;
    key_options.CaseInsensitive = (is_case_insensitive) ? 1 : 0;

    // potential error
    LG_Error* error;

    // parse regex into pattern
    int status = lg_parse_pattern(pattern_handle,
                                  regex.c_str(),
                                  &key_options,
                                  &error);

    // bad if parse error
    if (status == 0) {
      const std::string parse_error = compose_error(regex, *error);
      lg_free_error(error);
      error = nullptr;
      return parse_error;
    }

    // add the pattern
    int index = lg_add_pattern(fsm,
                               pattern_map,
                               pattern_handle,
                               character_encoding.c_str(),
                               &error);

    // bad if pattern error
    if (index < 0) {
      const std::string pattern_error = compose_error(regex, *error);
      lg_free_error(error);
      error = nullptr;
      return pattern_error;
    }

    // make sure index is in step with the function_pointers vector
    if (function_pointers.size() != index) {
      assert(0);
    }

    // record the scan callback function pointer at the pattern index position
    function_pointers.push_back(f);

    // no error
    return "";
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
  lw_scanner_t* lw_t::new_lw_scanner(void* user_data) {

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
                      new lw_scanner_t(searcher, context_options,
                                       function_pointers, user_data);
    lw_scanners.push_back(lw_scanner);
    return lw_scanner;
  }

  // private lw_scanner_t constructor
  lw_scanner_t::lw_scanner_t(LG_HCONTEXT p_searcher,
                             const LG_ContextOptions& p_context_options,
                             function_pointers_t& function_pointers,
                             void* user_data):
             searcher(p_searcher),
             data_pair(&function_pointers, user_data),
             start_offset(1000) {
  }

  // scan
  void lw_scanner_t::scan(const char* const buffer, size_t size) {
    lg_search(searcher,
              buffer,
              buffer + size,
              start_offset,
              &data_pair,
              lightgrep_callback);
    start_offset += size+100;
  }

  // scan_finalize
  void lw_scanner_t::scan_finalize() {
    lg_closeout_search(searcher,
                       &data_pair,
                       lightgrep_callback);

    start_offset = 0;
  }

  // scan_fence_finalize
  void lw_scanner_t::scan_fence_finalize(const char* const buffer,
                                         size_t size) {
    lg_search_resolve(searcher,
                      buffer,
                      buffer + size,
                      start_offset,
                      &data_pair,
                      lightgrep_callback);

    lg_closeout_search(searcher,
                       &data_pair,
                       lightgrep_callback);

    start_offset = 0;
  }
}

