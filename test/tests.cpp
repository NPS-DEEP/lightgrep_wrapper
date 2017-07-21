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

  // support read
  size_t buffer_offset;
  const char* previous_buffer;
  size_t previous_buffer_size;
  const char* buffer;
  size_t buffer_size;


  public:
  const std::string text;
  std::vector<std::string> matches;
  lw::lw_scanner_t* lw_scanner;
  user_data_t(const std::string& p_text) :
                       buffer_offset(0),
                       previous_buffer(nullptr), previous_buffer_size(0),
                       buffer(nullptr), buffer_size(0),
                       text(p_text), matches(), lw_scanner(nullptr) {
  }

  void scan_setup(const size_t p_buffer_offset,
                  const char* const p_buffer, const size_t p_buffer_size) {
    buffer_offset = p_buffer_offset;
    previous_buffer = buffer;
    previous_buffer_size = buffer_size;
    buffer = p_buffer;
    buffer_size = p_buffer_size;
  }

  std::string read(const uint64_t start, const uint64_t size) {
    if (lw_scanner == nullptr) {
      std::cerr << "Error: set up lw_scanner in user_data before calling read in user_data\n";
      assert(0);
    }
    return lw::read_buffer(buffer_offset,
                           previous_buffer, previous_buffer_size,
                           buffer, buffer_size,
                           start, size, 0);
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
  lw::lw_scanner_t* lw_scanner = lw.new_lw_scanner(&user_data);

  // set the scanner pointer
  user_data.lw_scanner = lw_scanner;

  // make something to scan
  const char c[] = "abcbabcbabcbabc";

  // scan first bytes
  std::cout << "test1 start 15\n";
  user_data.scan_setup(0, c, 15);
  lw_scanner->scan(c, 15);

  // scan again
  std::cout << "test1 scan again 5\n";
  user_data.scan_setup(15, c, 5);
  lw_scanner->scan(c, 5);

  // scan across fence
  std::cout << "test1 scan fence\n";
  user_data.scan_setup(20, c+5, 15-5);
  lw_scanner->scan_fence_finalize(c+5, 15-5);

  // scan first bytes again
  std::cout << "test1 scan first again\n";
  user_data.scan_setup(0, c, 0); // clear previous
  user_data.scan_setup(0, c, 15);
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

void test_read_bounds() {
  const char *pb;
  size_t pbs;
  const char *b;
  size_t bs;
  // both buffers empty
  pb = "";
  pbs = 0;
  b = "";
  bs = 0;
std::string zz = lw::read_buffer(0, pb, pbs, b, bs, 0,0,0);

  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,0,0), "");
  TEST_EQ(lw::read_buffer(1, pb, pbs, b, bs, 0,0,0), "");
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,0,1), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,0,0), "");

  // buffer empty
  pb = "12345";
  pbs = 5;
  b = "";
  bs = 0;
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,0,0), "");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,1,0), "1");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 1,1,0), "2");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,0,1), "1");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,1,1), "12");

  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,0,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,1,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,2,0), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,0,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,1,0), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,2,0), "12");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 99,1,0), "5");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,0,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,1,0), "");

  // previous_buffer empty
  pb = "";
  pbs = 0;
  b = "12345";
  bs = 5;
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,0,0), "");
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,1,0), "1");
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,1,0), "1");
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 1,1,0), "2");
  TEST_EQ(lw::read_buffer(0, pb, pbs, b, bs, 0,1,1), "12");

  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,0,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,1,0), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 101,1,0), "2");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,0,1), "1");

  // both buffers used
  pb = "12345";
  pbs = 5;
  b = "6789";
  bs = 4;
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,0,0), "");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,1,0), "1");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 1,1,0), "2");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,1,1), "12");
  TEST_EQ(lw::read_buffer(5, pb, pbs, b, bs, 0,0,20), "123456789");

  // no padding
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,1,0), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,1,0), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 99,1,0), "5");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,1,0), "6");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 103,1,0), "9");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 104,1,0), "");

  // no padding, 50 byte size
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 99,50,0), "56789");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,50,0), "6789");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 103,50,0), "9");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 104,50,0), "");

  // 1 byte padding, length 0
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,0,1), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,0,1), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 96,0,1), "12");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 99,0,1), "45");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,0,1), "56");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 103,0,1), "89");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 104,0,1), "9");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 105,0,1), "");

  // 1 byte padding, length 1
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 94,1,1), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 95,1,1), "12");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 99,1,1), "456");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 100,1,1), "567");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 103,1,1), "89");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 104,1,1), "9");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 105,1,1), "");

  // 50 byte padding
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 44,1,50), "");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 45,1,50), "1");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 153,1,50), "9");
  TEST_EQ(lw::read_buffer(100, pb, pbs, b, bs, 154,1,50), "");
}

// ************************************************************
// main
// ************************************************************
int main(int argc, char* argv[]) {

  // tests
  test1();
  test_read_bounds();

  // done
  std::cout << "Tests Done.\n";
  return 0;
}

