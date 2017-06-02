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
#include <iostream>
#include "unit_test.h"
#include "../src/lightgrep_wrapper.hpp"

class user_data_t {
  public:
  const std::string text;
  std::vector<std::string> matches;
  user_data_t(const std::string& p_text) : text(p_text), matches() {
  }
};

void report(const std::string& name, 
            const uint64_t &start,
            const uint64_t &size,
            user_data_t& user_data) {
  std::stringstream ss;
  ss << "name: " << name
     << ", user_data text: '" << user_data.text
     << "', start: " << start
     << ", size: " << size;
  std::cout << ss.str() << std::endl;
  user_data.matches.push_back(ss.str());
}

void callback_function1(const uint64_t start,
                        const uint64_t size,
                        void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* user_data(static_cast<user_data_t*>(p_user_data));
  report ("abc", start, size, *user_data);
}

void callback_function2(const uint64_t start,
                        const uint64_t size,
                        void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* user_data(static_cast<user_data_t*>(p_user_data));
  report ("bc", start, size, *user_data);
}

void callback_function3(const uint64_t start,
                        const uint64_t size,
                        void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* user_data(static_cast<user_data_t*>(p_user_data));
  report ("cab", start, size, *user_data);
}

void test1() {
  // create a lightgrep wrapper instance
  lw::lw_t lw;

scan_callback_function_t scf1 = &callback_function1;
std::cout << "&tests.scf1: " << &scf1 << std::endl;

  // add regex definitions
  lw.add_regex("abc", "UTF-8", false, false, &scf1);

//  lw.add_regex("bc", "UTF-8", false, false, &callback_function2);
scan_callback_function_t scf2 = &callback_function2;
std::cout << "&tests.scf2: " << &scf2 << std::endl;

  lw.add_regex("bc", "UTF-8", false, false, &scf2);


scan_callback_function_t scf3 = &callback_function3;
  lw.add_regex("cab", "UTF-8", false, false, &scf3);

  // finalize regex definitions
  lw.finalize_regex(false);

  // create a user data instance
  user_data_t user_data("test1");
std::cout << "&tests.user_data: " << &user_data << std::endl;

  // get a lw_scanner
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data);

  // make something to scan
  const char c[] = "abcbabcbabcbabcbabcbabcbabcbabcbabc";

  // scan first bytes
  lw_scanner->scan(c, 35);

  // scan again
  lw_scanner->scan(c, 20);

  // scan across fence
  lw_scanner->scan_fence_finalize(c+20, 35-20);

  // scan first bytes again
  lw_scanner->scan(c, 35);

  // finalize scan
  lw_scanner->scan_finalize();

  // show matches
  std::cout << "Matches:\n";
  for (auto it=user_data.matches.begin(); it != user_data.matches.end(); ++it) {
    std::cout << *it << "\n";
  }

  // validate size
  TEST_EQ(user_data.matches.size(), 30);
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

