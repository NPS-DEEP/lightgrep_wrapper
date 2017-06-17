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
  private:
  // do not allow copy or assignment
  user_data_t(const user_data_t&) = delete;
  user_data_t& operator=(const user_data_t&) = delete;

  public:
  const std::string text;
  std::vector<std::string> matches;
  lw::lw_scanner_t *lw_scanner;
  user_data_t(const std::string& p_text) :
                       text(p_text), matches(), lw_scanner(nullptr) {
  }
  std::string read(const uint64_t start, const uint64_t size) {
    if (lw_scanner == nullptr) {
      std::cerr << "Error: set up lw_scanner in user_data before calling read in user_data\n";
      assert(0);
    }
    return lw_scanner->read(start, size);
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
     << ", match: " << user_data.read(start, size);
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

void callback_stream_read(const uint64_t start,
                   const uint64_t size,
                   void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* d(static_cast<user_data_t*>(p_user_data));
  std::stringstream ss;
  ss << "start: " << start << ", size: " << size
     << ", data: " << d->read(start, size);
  d->matches.push_back(ss.str());
}

void test_clipped_stream_read() {
  // create a lightgrep wrapper instance
  lw::lw_t lw;

  // add regex definitions
  lw.add_regex("ef0", "UTF-8", false, false, &callback_stream_read);

  // finalize regex definitions
  lw.finalize_regex(false);

  // create a user data instance
  user_data_t user_data("test_clipped_stream_read");

  // get a lw_scanner
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data, 1);

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
  TEST_EQ(user_data.matches.size(), 2);
  TEST_EQ(user_data.matches[0], "start: 14, size: 3, data: f0");
  TEST_EQ(user_data.matches[1], "start: 30, size: 3, data: f0");
}

void callback_read_bounds(const uint64_t start,
                          const uint64_t size,
                          void* p_user_data) {
  // typecast void* p_user_data into user_dat_t* user_data
  user_data_t* d(static_cast<user_data_t*>(p_user_data));
  std::string result = "0,1'"+d->read(0,1) +
                      "' 15,1'"+d->read(15,1) +
                      "' 15,2'"+d->read(15,2) +
                      "' 15,17'"+d->read(15,17) +
                      "' 15,18'"+d->read(15,18) +
                      "' 16,1'"+d->read(16,1) +
                      "' 16,16'"+d->read(16,16) +
                      "'";
  d->matches.push_back(result);
}

void test_read_bounds() {
  // create a lightgrep wrapper instance
  lw::lw_t lw;

  // add regex definitions
  lw.add_regex("0", "UTF-8", false, false, &callback_read_bounds);

  // finalize regex definitions
  lw.finalize_regex(false);

  // create a user data instance
  user_data_t user_data("test_read_bounds");

  // get a lw_scanner
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data, 16);

  // set the scanner pointer
  user_data.lw_scanner = lw_scanner;

  // make something to scan
  const char c[] = "0123456789abcdef";

  // scan stream round 1
  std::cout << "test_read_bounds round 1\n";
  lw_scanner->scan(c, 16);
  TEST_EQ(user_data.matches.size(), 1);
  TEST_EQ(user_data.matches[0], "0,1'0' 15,1'f' 15,2'' 15,17'' 15,18'' 16,1'' 16,16''");

  // scan stream round 2
  std::cout << "test_read_bounds round 2\n";
  lw_scanner->scan(c, 16);
  TEST_EQ(user_data.matches.size(), 2);
  TEST_EQ(user_data.matches[1], "0,1'' 15,1'f' 15,2'f0' 15,17'f0123456789abcdef' 15,18'' 16,1'0' 16,16'0123456789abcdef'");

  // scan stream round 3
  std::cout << "test_read_bounds round 3\n";
  lw_scanner->scan(c, 16);
  TEST_EQ(user_data.matches.size(), 3);
  TEST_EQ(user_data.matches[2], "0,1'' 15,1'' 15,2'' 15,17'0123456789abcdef' 15,18'0123456789abcdef0' 16,1'' 16,16'0123456789abcdef'");
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // tests
  test1();
  test_clipped_stream_read();
  test_read_bounds();

  // done
  std::cout << "Tests Done.\n";
  return 0;
}

