#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm> // for_each
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>   
#include <functional>
#include <limits>
#include <sys/stat.h>
#include <cassert>
#include "date.h"    // provides a stream insertion operator for system_clock::time_point (in namespace date)

using namespace std;
// using namespace std::chrono_literals;
using namespace date;

typedef unsigned int uint;
typedef unsigned long ulong;

template<typename T>
struct Missing;

template<>
struct Missing<uint> {
    static uint NA() { return numeric_limits<uint>::max(); }
};

template<>
struct Missing<ulong> {
    static ulong NA() { return numeric_limits<ulong>::max(); }
};

bool is_NA(uint x) { return x == Missing<uint>::NA() ? true : false; }
bool is_NA(ulong x) { return x == Missing<ulong>::NA() ? true : false; }

class Timer {
    chrono::steady_clock::time_point _begin;
    chrono::steady_clock::time_point _end;
    ulong _diff;
    string _msg;
    ostream& out = std::cout;
    // disable assignment:
    Timer& operator=(Timer const& rhs);
public:
    Timer(ostream& o=std::cout) : out(o) {
        this->_begin = chrono::steady_clock::now();
        this->_msg = "";
    }
    chrono::steady_clock::time_point start(string msg="") {
        this->_msg   = msg;
        this->_begin = chrono::steady_clock::now();

        return this->_begin;
    }
    chrono::steady_clock::time_point stop() {
        this->_end  = chrono::steady_clock::now();
        this->_diff = chrono::duration_cast<chrono::milliseconds>(this->_end-this->_begin).count();
        if (this->_msg.length()) {
            out << this->_msg << this->_diff << " ms" << endl;
        }

        return this->_end;
    }
    ulong diff() {
        return this->_diff;
    }
    static std::string now() {
        stringstream ss;
        ss << chrono::system_clock::now() << " (UTC+0)";

        return ss.str();
    }

};

string rm_newlines(string const& s) {
    std::string o = s;
    o.erase(std::remove(o.begin(), o.end(), '\n'), o.cend());

    return o;
};

string exec(string const& cmd) {
    // Source: https://raymii.org/s/articles/Execute_a_command_and_get_both_output_and_exit_code.html
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) result += buffer;
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);

    return result;
};

string comma_ify(unsigned long value) {
    // Source: https://stackoverflow.com/questions/7276826/format-number-with-commas-in-c
    auto s = std::to_string(value);
    int n = s.length() - 3;
    int end = (value >= 0) ? 0 : 1; // Support for negative numbers
    while (n > end) {
        s.insert(n, ",");
        n -= 3;
    }
    return s;
}

string get_extension(string const& filename) {
  string::size_type const p(filename.find_last_of('.'));
  return p > 0 && (p+1) != string::npos ? filename.substr(p+1, filename.length()) : filename;
}

string remove_extension(string const& filename) {
  string::size_type const p(filename.find_last_of('.'));
  return p > 0 && p != string::npos ? filename.substr(0, p) : filename;
}

bool is_short_option(string const& arg) {
    if (arg.rfind("-")==0 && arg.rfind("--")!=0) return true;
    return false; 
}

bool is_long_option(string const& arg) {
    if (arg.rfind("--")==0) return true;
    return false; 
}

bool is_safe_to_read(int i, int argc, char* argv[]) {
    if (i < argc && (!is_short_option(argv[i]) && !is_long_option(argv[i]))) return true;
    return false; 
}

inline bool file_exists(string const& filename) {
    // Source: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exists-using-standard-c-c11-14-17-c
    struct stat buffer;   
    return (stat (filename.c_str(), &buffer) == 0); 
}

unsigned long nlines_in_file_cpp(string const& filename) {
    // odd: this is about 10% faster than nlines_in_file_c
    if (!file_exists(filename)) {
        cerr << "nlines_in_file_cpp : error: '" << filename << "' does not exist" << endl;;
        exit(1);
    }
    unsigned long nlines = 0;
    ifstream fi(filename);
    string line;
    while (getline(fi, line)) {
        ++nlines;
    }

    return nlines;
}
    
unsigned long nlines_in_file_c(string const& filename) {
    if (!file_exists(filename)) {
        cerr << "nlines_in_file_c : error: '" << filename << "' does not exist" << endl;;
        exit(1);
    }
    unsigned long nlines = 0;
    FILE* fi = fopen(filename.c_str(), "r");
    char* line = NULL;
    size_t len = 0;
    while ((getline(&line, &len, fi)) != -1) {
        ++nlines;
    }
    fclose(fi);
    if (line) free(line);

    return nlines;
}

unsigned long nlines_in_file_c_SLOW(string const& filename) {
    if (!file_exists(filename)) {
        cerr << "nlines_in_file_c : error: '" << filename << "' does not exist" << endl;;
        exit(1);
    }
    unsigned long nlines = 0;
    FILE* fi = fopen(filename.c_str(), "r");
    int ch;
    while (EOF != (ch=getc(fi))) {
        if ('\n' == ch) ++nlines;
    }
    fclose(fi);

    return nlines;
}
    
