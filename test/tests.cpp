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

#include <config.h>
#include <cstdio>
#include <cstdint>
#include "unit_test.h"
#include "../src/lightgrep_wrapper.hpp"

class user_data_t {
  public:
  const std::string text;
  user_data_t(const std::string& p_text) : text(p_text) {
  }
};

void report(const std::string& name, 
            const uint64_t &start,
            const uint64_t &size,
            const user_data_t& user_data) {
  std::cout << "name: " << name
            << ", user_data text: '" << user_data.text
            << "', start: " << start
            << ", size: " << size << std::endl;

void callback_function1(const uint64_t &start,
                        const uint64_t &size,
                        const void* &p_user_data) {
  // typecast p_user_data
  user_data_t* user_data(static_cast<user_data_t*>(p_user_data);
  report ("callback_function1", start, size, *user_data);
}

void callback_function2(const uint64_t &start,
                        const uint64_t &size,
                        const void* &p_user_data) {
  // typecast p_user_data
  user_data_t* user_data(static_cast<user_data_t*>(p_user_data);
  report ("callback_function2", start, size, *user_data);
}

void test1() {
  // create a user data class and a lightgrep wrapper instance
  user_data_t user_data("some user data text");
  lw::lw_t lw;

  // add regex definitions
  lw.add_regex("abc", "UTF-8", false, false, callback_function1);
  lw.add_regex("bc", "UTF-8", false, false, callback_function2);

  // finalize regex definitions
  lw.finalize_regex(false);

  // get a lw_scanner
  lw_scanner_t lw_scanner(&user_data);

  // make something to scan, 35 letters
  const char c[] = "abcbabcbabcbabcbabcbabcbabcbabcbabc";

  // scan first bytes
  lw_scanner.scan(c, 20);

  // scan across fence
  lw_scanner.scan(c+20, 35-20);

  // finalize scan
  lw_scanner.scan_finalize);
}


// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // tests
  test1();

  // done
  std::cout << "Tests Done.\n";
  return 0;
}

