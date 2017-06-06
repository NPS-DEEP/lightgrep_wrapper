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
#include <sstream>
#include <cassert>
#include "unit_test.h"
#include "../src/lightgrep_wrapper.hpp"

class user_data_t {
  public:
  const std::string text;
  std::vector<std::string> matches;
  lw::lw_scanner_t *lw_scanner;
  user_data_t(const std::string& p_text) :
                       text(p_text), matches(), lw_scanner(nullptr) {
  }
  std::string read(const uint64_t start, const uint64_t size,
                   const size_t padding) {
    if (lw_scanner == nullptr) {
      std::cerr << "set up lw_scanner first\n";
      assert(0);
    }
    return lw_scanner->read(start, size, padding);
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
     << ", size: " << size
     << ", match: " << user_data.read(start, size, 0)
     << ", context: " << user_data.read(start, size, 8);
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

  // add regex definitions
  lw.add_regex("abc", "UTF-8", false, false, &callback_function1);
  lw.add_regex("bc", "UTF-8", false, false, &callback_function2);
  lw.add_regex("cab", "UTF-8", false, false, &callback_function3);

  // finalize regex definitions
  lw.finalize_regex(false);

  // create a user data instance
  user_data_t user_data("test1");

  // get a lw_scanner
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data, 1000);

  // set the scanner pointer
  user_data.lw_scanner = lw_scanner;

  // make something to scan
  const char c[] = "abcbabcbabcbabc";

  // scan first bytes
  std::cout << "test1 start 15\n";
  lw_scanner->scan(c, 15);

  // scan again
  std::cout << "test1 scan again 5\n";
  lw_scanner->scan(c, 5);

  // scan across fence
  std::cout << "test1 scan fence\n";
  lw_scanner->scan_fence_finalize(c+5, 15-5);

  // scan first bytes again
  std::cout << "test1 scan first again\n";
  lw_scanner->scan(c, 15);

  // finalize scan
  std::cout << "test1 finalize\n";
  lw_scanner->scan_finalize();

  // show matches
  std::cout << "Matches:\n";
  for (auto it=user_data.matches.begin(); it != user_data.matches.end(); ++it) {
    std::cout << *it << "\n";
  }

  // validate size
  TEST_EQ(user_data.matches.size(), 20);
}

void callback_read(const uint64_t start,
                   const uint64_t size,
                   void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* d(static_cast<user_data_t*>(p_user_data));
  std::stringstream ss;
  ss << "start: " << start << ", size: " << size
     << ", p0: " << d->read(start, size, 0)
     << ", p1: " << d->read(start, size, 1)
     << ", p2: " << d->read(start, size, 2)
     << ", p3: " << d->read(start, size, 3)
     << ", p4: " << d->read(start, size, 4)
     << ", p5: " << d->read(start, size, 5);
  d->matches.push_back(ss.str());
}

void test_read_function() {
  // create a lightgrep wrapper instance
  lw::lw_t lw;

  // add regex definitions
  lw.add_regex("0", "UTF-8", false, false, &callback_read);
  lw.add_regex("2", "UTF-8", false, false, &callback_read);
  lw.add_regex("d", "UTF-8", false, false, &callback_read);

  // finalize regex definitions
  lw.finalize_regex(false);

  // create a user data instance
  user_data_t user_data("test_read_function");

  // get a lw_scanner
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data, 4);

  // set the scanner pointer
  user_data.lw_scanner = lw_scanner;

  // make something to scan
  const char c[] = "0123456789abcdef";

  // scan stream
  lw_scanner->scan(c, 16);
  lw_scanner->scan(c, 16);
  lw_scanner->scan(c, 16);
  lw_scanner->scan_finalize();

  // show matches
  std::cout << "Matches:\n";
  for (auto it=user_data.matches.begin(); it != user_data.matches.end(); ++it) {
    std::cout << *it << "\n";
  }

  // validate size
  TEST_EQ(user_data.matches.size(), 9);
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // tests
  test1();
  test_read_function();

  // done
  std::cout << "Tests Done.\n";
  return 0;
}

