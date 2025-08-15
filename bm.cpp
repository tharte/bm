#include "utils.h"

using namespace std;

struct Config {
    // input values:
    string  subcommand = "";
    bool    quietly    = false;
    string  wfilename  = "";
    string  wfunction  = "";
    uint    wnthreads  = Missing<uint>::NA();
    uint    wnlines    = Missing<uint>::NA();
    string  rfilename  = "";
    string  rfunction  = "read_c_nlines";
    ulong   rnlines    = Missing<ulong>::NA();
    uint    rnthreads  = Missing<uint>::NA();
    // calculated values:
    ulong   sleep_for  = 0;
    // double   sleep_for  = 0;

    friend ostream& operator<<(ostream& out, Config const& rhs) {
        out << "Config: subcommand='" << rhs.subcommand << "'" << endl;
        out << "Config: wfilename='"  << rhs.wfilename << "'" << endl;
        out << "Config: wfunction='"  << rhs.wfunction << "'" << endl;
        out << "Config: wnthreads="   << (is_NA(rhs.wnthreads) ? "NO THREADS" : to_string(rhs.wnthreads)) << endl;
        out << "Config: wnlines="     << (is_NA(rhs.wnlines) ? "AS FAST AS POSSIBLE" : comma_ify(rhs.wnlines) + " / second") << endl;
        out << "Config: rfunction='"  << rhs.rfunction << "'" << endl;
        out << "Config: rfilename='"  << rhs.rfilename << "'" << endl;
        out << "Config: rnthreads="   << (is_NA(rhs.rnthreads) ? "NO THREADS" : to_string(rhs.rnthreads)) << endl;
        out << "Config: rnlines="     << (is_NA(rhs.rnlines) ? "ALL" : comma_ify(rhs.rnlines)) << endl;
        out << "Config: sleep_for="   << (is_NA(rhs.sleep_for) ? "0" : comma_ify(rhs.sleep_for) + " microseconds") << endl;

        return out;
    }
};

enum class Flag {
    DUD,               // <- hack: otherwise, unrecognized field doesn't work
    help,
    wfilename,
    wfunction,
    wnthreads,
    wnlines,
    rfilename,
    rfunction,
    rnlines,
    rnthreads,
    quietly
};

map<string,Flag> flags = {
    {"--help", Flag::help},                {"-h", Flag::help},
    {"--write-filename", Flag::wfilename}, {"-o", Flag::wfilename}, 
    {"--write-function", Flag::wfunction}, {"-w", Flag::wfunction},
    {"--write-nthreads", Flag::wnthreads}, {"-u", Flag::wnthreads},
    {"--write-nlines", Flag::wnlines},     {"-N", Flag::wnlines},
    {"--read-filename", Flag::rfilename},  {"-i", Flag::rfilename}, 
    {"--read-function", Flag::rfunction},  {"-r", Flag::rfunction},
    {"--read-nlines", Flag::rnlines},      {"-n", Flag::rnlines},    
    {"--read-nthreads", Flag::rnthreads},  {"-t", Flag::rnthreads},  
    {"--quietly", Flag::quietly},          {"-q", Flag::quietly},  
};

template<typename T>
vector<string> keys(map<string,T> const& m) {
    vector<string> o;
    for (auto const& e : m) o.push_back(e.first);

    return o;
}

template<typename T>
bool is_key(string const& key, map<string,T> const& m) {
    vector<string> k = keys(m);
    if (find(k.begin(), k.end(), key) == k.end()) return false;
    return true;
}

std::vector<string> read_c_nlines(Config const& config, uint task=Missing<uint>::NA()) {
    Timer t(std::cerr);
    if (!is_NA(task)) {
        stringstream s; s << "task #" << task << ": ";
        t.start(s.str());
    }

    FILE* fi = fopen(config.rfilename.c_str(), "r");
    char* line = NULL;
    size_t len = 0;

    std::vector<string> array;
    if (is_NA(config.rnlines)) {
        while ((getline(&line, &len, fi)) != -1) {
            array.push_back(line);
        }
    }
    else {
        for (ulong i=0; i<config.rnlines; ++i) {
            if ((getline(&line, &len, fi)) != -1) {
                array.push_back(line);
            }
        }
    }

    fclose(fi);
    if (line) free(line);

    if (!is_NA(task)) {
        t.stop();
    }

    return array;
}

std::vector<string> read_cpp_nlines(Config const& config, uint task=Missing<uint>::NA()) {
    ifstream fi(config.rfilename);
    string line;
    std::vector<string> array;

    Timer t(std::cerr);
    if (task!=UINT_MAX) {
        stringstream s; s << "task #" << task << ": ";
        t.start(s.str());
    }

    if (is_NA(config.rnlines)) {
        while (getline(fi, line)) {
            array.push_back(line);
        }
    }
    else {
        for (ulong i=0; i<config.rnlines; ++i) {
            getline(fi, line);
            array.push_back(line);
        }
    }

    if (task!=UINT_MAX) {
        t.stop();
    }

    return array;
}

void write_c_ORIGINAL(vector<string> const& DUD /* <- not used */, Config const& config, uint task=Missing<uint>::NA()) {
    string filename = config.wfilename;
    
    if (!is_NA(task)) {
        string basename=remove_extension(filename);
        string ext=get_extension(filename);
        stringstream ff; ff << basename << "_task_" << task << "." << ext; 
        filename=ff.str();
    }

    // fixed-cost: do not include in write timing - use fastest read (read_c_nlines)
    // https://developers.redhat.com/blog/2019/04/12/understanding-when-not-to-stdmove-in-c
    // std::vector<string> array = std::move(read_c_nlines(config));
    std::vector<string> array = read_c_nlines(config);

    Timer t(std::cerr);
    if (is_NA(task)) {
        t.start("write_c_ORIGINAL: ");
    }
    else {
        stringstream s; s << "write_c_ORIGINAL - task #" << task << ": ";
        t.start(s.str());
    }

    FILE* fo = fopen(filename.c_str(), "w");

    for (ulong i=0; i<array.size(); ++i) {
        fprintf(fo, "%s", array[i].c_str());
    }

    fclose(fo);
    t.stop();

    return;
}

void write_c(vector<string> const& array, Config const& config, uint task=Missing<uint>::NA()) {
    string filename = config.wfilename;
    
    if (!is_NA(task)) {
        string basename=remove_extension(filename);
        string ext=get_extension(filename);
        stringstream ff; ff << basename << "_task_" << task << "." << ext; 
        filename=ff.str();
    }

    Timer t(std::cerr);
    if (is_NA(task)) {
        t.start("write_c: ");
    }
    else {
        stringstream s; s << "write_c - task #" << task << ": ";
        t.start(s.str());
    }

    FILE* fo = fopen(filename.c_str(), "w");

    for (ulong i=0; i<array.size(); ++i) {
        fprintf(fo, "%s", array[i].c_str());
    }

    fclose(fo);
    t.stop();

    return;
}

void write_c_throughput_only(vector<string> const& array, Config const& config, uint task=Missing<uint>::NA()) {
    string filename = config.wfilename;
    
    FILE* fo = fopen(filename.c_str(), "w");

    for (ulong i=0; i<array.size(); ++i) {
        fprintf(fo, "%s", array[i].c_str());
    }

    fclose(fo);

    return;
}

void write_c_nlines_per_second(vector<string> const& array, Config const& config, uint task=Missing<uint>::NA()) {
    string filename = config.wfilename;
    
    if (!is_NA(task)) {
        string basename=remove_extension(filename);
        string ext=get_extension(filename);
        stringstream ff; ff << basename << "_task_" << task << "." << ext; 
        filename=ff.str();
    }

    Timer t(std::cerr);
    if (is_NA(task)) {
        t.start("write_c_nlines_per_second: ");
    }
    else {
        stringstream s; s << "write_c_nlines_per_second - task #" << task << ": ";
        t.start(s.str());
    }

    FILE* fo = fopen(filename.c_str(), "w");

    for (ulong i=0; i<array.size(); ++i) {
	this_thread::sleep_for(std::chrono::microseconds(config.sleep_for));
        fprintf(fo, "%s", array[i].c_str());
        fflush(NULL);
    }

    fclose(fo);
    t.stop();

    return;
}

void write_cpp(vector<string> const& array, Config const& config, uint task=Missing<uint>::NA())  {
    string filename = config.wfilename;
    
    if (!is_NA(task)) {
        string basename=remove_extension(filename);
        string ext=get_extension(filename);
        stringstream ff; ff << basename << "_task_" << task << "." << ext; 
        filename=ff.str();
    }

    Timer t(std::cerr);
    if (is_NA(task)) {
        t.start("write_cpp: ");
    }
    else {
        stringstream s; s << "write_cpp - task #" << task << ": ";
        t.start(s.str());
    }


    ofstream fo; fo.open(filename);

    for (ulong i=0; i<array.size(); ++i) {
        fo << array[i] << endl;
    }

    fo.close();
    t.stop();

    return;
}

namespace Functions {
    map<string, function<void(vector<string> const&, Config const&, uint)> > write = {
	{"write_c",   write_c},
	{"write_cpp", write_cpp},
	{"write_c_ORIGINAL", write_c_ORIGINAL},
	{"write_c_nlines_per_second", write_c_nlines_per_second},
	{"write_c_throughput_only", write_c_throughput_only}
    };
    map<string, function<vector<string>(Config const&, uint)> > read = {
	{"read_c_nlines",   read_c_nlines},
	{"read_cpp_nlines", read_cpp_nlines}
    };
};

class Parser {
public:
    Parser(int argc, char* argv[], Config& config) {
	if (argc < 2) {
	    Parser::usage(argv);
	}
        if (string(argv[1])=="read") {
            config.subcommand = "read";
            Read(argc, argv, config);
        }
        else if (string(argv[1])=="write") {
            config.subcommand = "write";
            Write(argc, argv, config);
        }
        else {
            cerr << "unrecognized argument '" << argv[1] << "'\n\n";
            Parser::usage(argv);
            exit(1);
        }
    }
    static void usage(char* argv[]) {
        cerr << "Usage:\n\t" << argv[0] << " read|write <option(s)>\n\n"
	    << "Two subcommands:\n\n"
	    << "read\tread from a file (possibly growing)\n\n"
	    << "\tOptions:\n"
	    << "\t-h,--help                                    : show this help message\n\n"
	    << "write\twrite to a file\n\n"
	    << "\tOptions:\n"
	    << "\t-h,--help                                    : show this help message\n\n";
	exit(1);
    }

    class Read {
        int _argc;
        char** _argv;
    public:
        Read(int argc, char* argv[], Config& config) : _argc(argc), _argv(argv) {
	    if (argc==2) {
		Read::usage(argv);
	    }

            for (int i=2; i<argc; ++i) {  // <- start at i=2: argv[0]==program name, argv[1]==read
                switch ( flags[string(argv[i])] ) {
                    case Flag::help:
                        Parser::Read::usage(argv);
                        break;
                    case Flag::quietly:
                        config.quietly=true;
                        break;
                    case Flag::rfilename:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rfilename=argv[++i];
                        }
                        else { 
                            cerr << "Parser::Read: --read-filename requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rfunction:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rfunction=argv[++i];
                            if (!is_key(config.rfunction, Functions::read)) {
                                cerr << "Parser::Read: --read-function : unrecognized read function '" << config.rfunction << "'\n\n";
                                exit(1);
                            }
                        }
                        else { 
                            cerr << "Parser::Read: --read-function requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rnlines:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rnlines=stol(argv[++i]);
                        }
                        else { 
                            cerr << "Parser::Read: --read-nlines requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rnthreads:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            uint value=stoi(argv[++i]);
                            if (value!=0) {
                                config.rnthreads=value;
                            }
                        }
                        else { 
                            cerr << "Parser::Read: --read-nthreads requires argument\n\n";
                            exit(1);
                        }
                        break;
                    default:
                        cerr << "Parser::Read: unrecognized flag '" << argv[i] << "'\n\n";
                        exit(1);
                        break;
                }
            }
            
            if (config.rfilename.length()==0) {
                cerr << "Parser::Read: --read-filename not specified\n\n";
                exit(1);
            }
            if (!file_exists(config.rfilename)) {
                cerr << "Parser::Read: --read-filename '" << config.rfilename << "' does not exist\n\n";
                exit(1);
            }
            if (!is_NA(config.rnlines)) {
                ulong nlines = nlines_in_file_cpp(config.rfilename);
                if (nlines < config.rnlines) {
                    cerr << "Parser::Read: --read-filename='" << config.rfilename << "' contains " << comma_ify(nlines) << " lines, but --read-nlines=" << config.rnlines << "\n\n";
                    exit(1);
                }
            }
	}
	static void usage(char* argv[]) {
	    cerr << "Usage:\n\t" << argv[0] << " read <option(s)>\n\n"
	         << "read from a file (possibly growing) - read from filename nlines at a time with 0 or nthreads using specified function\n\n"
		 << "Options:\n"
		 << "\t-h,--help                                    : show this help message\n"
		 << "\t-i,--read-filename FILENAME                  : filename to read from\n"
		 << "\t-r,--read-function                           : function to read from file specified by --read-filename\n"
		 << "\t   [read_c_nlines|read_cpp_nlines|...]       : [choices]\n"
		 << "\t-n,--read-nlines   NLINES                    : number of lines to read from file specified by --read-filename\n"
		 << "\t-t,--read-nthreads NUMTHREADS                : number of threads to spawn for function specified by --read-function\n"
		 << "\t-q,--quietly                                 : produce quiet output\n\n";
	    exit(1);
	}
    };
    class Write {
        int _argc;
        char** _argv;
    public:
        Write(int argc, char* argv[], Config& config) : _argc(argc), _argv(argv) {
	    if (_argc==2) {
		Write::usage(_argv);
	    }

            for (int i=2; i<argc; ++i) {  // <- start at i=2: argv[0]==program name, argv[1]==write
                switch ( flags[string(argv[i])] ) {
                    case Flag::help:
                        Parser::Write::usage(argv);
                        break;
                    case Flag::quietly:
                        config.quietly=true;
                        break;
                    case Flag::wfilename:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.wfilename=argv[++i];
                        }
                        else { 
                            cerr << "Parser::Write: --write-filename requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::wfunction:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.wfunction=argv[++i];
                            if (!is_key(config.wfunction, Functions::write)) {
                                cerr << "Parser::Write: --write-function : unrecognized write function '" << config.wfunction << "'\n\n";
                                exit(1);
                            }
                        }
                        else { 
                            cerr << "Parser::Write: --write-function requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::wnthreads:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            uint value=stoi(argv[++i]);
                            if (value!=0) {
                                config.wnthreads=value;
                            }
                        }
                        else { 
                            cerr << "Parser::Write: --read-nthreads requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::wnlines:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.wnlines=stol(argv[++i]);
                        }
                        else { 
                            cerr << "Parser::Write: --write-nlines requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rfilename:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rfilename=argv[++i];
                        }
                        else { 
                            cerr << "Parser::Write: --read-filename requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rfunction:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rfunction=argv[++i];
                            if (!is_key(config.rfunction, Functions::read)) {
                                cerr << "Parser::Write: --read-function : unrecognized read function '" << config.rfunction << "'\n\n";
                                exit(1);
                            }
                        }
                        else { 
                            cerr << "Parser::Write: --read-function requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rnlines:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            config.rnlines=stol(argv[++i]);
                        }
                        else { 
                            cerr << "Parser::Write: --read-nlines requires argument\n\n";
                            exit(1);
                        }
                        break;
                    case Flag::rnthreads:
                        if (is_safe_to_read(i+1, argc, argv)) {
                            uint value=stoi(argv[++i]);
                            if (value!=0) {
                                config.rnthreads=value;
                            }
                        }
                        else { 
                            cerr << "Parser::Write: --read-nthreads requires argument\n\n";
                            exit(1);
                        }
                        break;
                    default:
                        cerr << "Parser::Write: unrecognized flag '" << argv[i] << "'\n\n";
                        exit(1);
                        break;
                }
            }
            // required arguments
            if (config.wfilename.length()==0) {
                cerr << "Parser::Write: --write-filename not specified\n\n";
                exit(1);
            }
            if (config.wfunction.length()==0) {
                cerr << "Parser::Write: --write-function not specified\n\n";
                exit(1);
            }
            if (config.rfilename.length()==0) {
                cerr << "Parser::Write: --read-filename not specified\n\n";
                exit(1);
            }
            if (!file_exists(config.rfilename)) {
                cerr << "Parser::Write: --read-filename '" << config.rfilename << "' does not exist\n\n";
                exit(1);
            }
            if (!is_NA(config.rnlines)) {
                ulong nlines = nlines_in_file_cpp(config.rfilename);
                if (nlines < config.rnlines) {
                    cerr << "Parser::Write: --read-filename='" << config.rfilename << "' contains " << comma_ify(nlines) << " lines, but --read-nlines=" << config.rnlines << "\n\n";
                    exit(1);
                }
            }
            if (config.rfunction.length()==0) {
                cerr << "Parser::Write: --read-function not specified\n\n";
                exit(1);
            }
	}
	static void usage(char* argv[]) {
	    cerr << "Usage:\n\t" << argv[0] << " write <option(s)>\n\n"
	         << "write to a file - reads contents of filename specified by read-filename and write to write-filename at max throughput\n"
	         << "or at nlines per second with 0 or nthreads using specified write-function; \n"
	         << "if -t 0 [default] -o out.log are specified, write contents of filename specified by read-filename to out.log\n"
	         << "if -t 3 -o out.log are specified, write contents of filename specified by read-filename to n files:\n"
	         << "    out_task_0.log\n"
	         << "    out_task_1.log\n"
	         << "    out_task_2.log\n\n"
		 << "Options:\n"
		 << "\t-h,--help                                    : show this help message\n"
		 << "\t-o,--write-filename FILENAME                 : filename to write to\n"
		 << "\t-w,--write-function                          : function to write to file specified by --write-filename\n"
		 << "\t   [write_c|write_c_ORIGINAL|write_c_throughput_only|write_c_nlines_per_second]\n"
		 << "\t-u,--write-nthreads NUMTHREADS               : number of threads to spawn for function specified by --write-function\n"
                 << "\t-N,--write-nlines NLINES / second            : number of lines of file specified by --read-filename to write per second\n"
                 << "\t                                             : [option can only be used with write_c_nlines_per_second]\n"
		 << "\t-i,--read-filename FILENAME                  : filename to read from\n"
		 << "\t-r,--read-function                           : function to read from file specified by --read-filename\n"
		 << "\t   [read_c_nlines|read_cpp_nlines|...]       : [choices]\n"
                 << "\t-q,--quietly                                 : produce quiet output\n\n";
	    exit(1);
	}
    };
};

class Runner {
    Config& _config;
    
    // disallow copy, assignment:
    Runner(Runner const& rhs);
    Runner& operator=(Runner const& rhs);
public:
    Runner(Config& config) : _config(config) {}

    void run() {
        if (_config.subcommand=="write") {
            write();
        }
        if (_config.subcommand=="read") {
            read();
        }
        if (_config.subcommand=="") {
            cerr << "Runner::run: subcommand not set" << endl;
            exit(1);
        }
    }

    void write() {
        // fixed-cost: do not include in write timing - use fastest read (read_c_nlines)
        // https://developers.redhat.com/blog/2019/04/12/understanding-when-not-to-stdmove-in-c
        // std::vector<string> array = std::move(read_c_nlines(_config));
        std::vector<string> array = read_c_nlines(_config);

        if (!is_NA(_config.wnlines)) {
            if (!_config.quietly) {
                cout << "Performing speed test... " << endl;
            }
            
            ulong nlines = nlines_in_file_cpp(_config.rfilename);
            if (nlines < _config.wnlines) {
                cerr << "Runner::write: " << comma_ify(_config.wnlines)
                     << " lines per second requested, but " << comma_ify(nlines)
                     << " lines in " << _config.rfilename << ": ignoring"
                     << endl;
            }
            else {
                function<ulong(vector<string> const&)> get_nlines_per_second = [&](vector<string> const& array) -> ulong {
                    // N = throughput (lines / second)
                    string tmpfilename = rm_newlines(exec("mktemp"));

                    Timer t(std::cerr);
                    t.start();
                    Functions::write["write_c_throughput_only"](array, _config, Missing<uint>::NA());
                    t.stop();
                    double time_in_ms        = t.diff();
                    // double time_in_seconds   = static_cast<double>(time_in_ms / 1000.0);
                    ulong nlines_per_second = static_cast<ulong>((array.size() / time_in_ms) * 1000.0);

                    if (nlines_per_second < _config.wnlines) {
                        cerr << "Runner::write::get_nlines_per_second: " << comma_ify(_config.wnlines)
                            << " lines per second requested, but throughput only " << comma_ify(nlines_per_second)
                            << " lines per second: ignoring"
                            << endl;
                        exit(1);
                    }

                    return nlines_per_second;
                };
                // N: max throughput (lines / second)
                ulong N = get_nlines_per_second(array);
                // M: requested lines to be written (lines / second) - M <= N (else error)
                ulong M = _config.wnlines;
                // It takes 1/N seconds to write each line at max throughput
                // M lines would take M * 1/N seconds to write, where (M * 1/N) <= 1, as M <= N (else error)
                assert (M <= N);
                // I: total idle time needed per second to write M lines, given N lines / second throughput
                double I = 1.0 - (static_cast<double>(M) / N);                                // idle time (seconds)
                // S: divide total idle time per second into the M lines we need to write - this is the time we need to sleep between each line written 
                double S = (I / M);                                                           // sleep for (seconds)

                _config.sleep_for = static_cast<ulong>(floor(S * 1000000.0));
                
                if (!_config.quietly) {
                    cout << "Runner::write: max throughput="            << comma_ify(N) << " (lines/second)" << endl;
                    cout << "Runner::write: requested="                 << comma_ify(M) << " (lines/second)" << endl;
                    cout << "Runner::write: idle time per second="      << I            << " (seconds)" << endl;
                    cout << "Runner::write: sleep for="                 << S            << " (seconds per line written)" << endl;
                    cout << "Runner::write: sleep for=" << comma_ify(_config.sleep_for) << " (microseconds per line written)" << endl;
                }
            }
        }

        if (!_config.quietly) {
            cerr << "Runner::Runner:\n" << _config << endl;
        }

        Timer timer(std::cerr);
        timer.start("Runner::write duration: ");

        if (is_NA(_config.wnthreads)) {
	    Functions::write[_config.wfunction](array, _config, Missing<uint>::NA());
        }
        else {
            std::vector<thread> threads;
            for (uint task=0; task<_config.wnthreads; ++task) {
                threads.push_back(thread(Functions::write[_config.wfunction], array, _config, task));
            }
            for (uint task=0; task<_config.wnthreads; ++task) {
                threads[task].join();
            }
        }

        timer.stop();
    }

    void read() {
        Timer timer(std::cerr);
        timer.start("Runner::read duration: ");

        if (is_NA(_config.rnthreads)) {
	    Functions::read[_config.rfunction](_config, Missing<uint>::NA());
        }
        else {
            std::vector<thread> threads;
            for (uint task=0; task<_config.rnthreads; ++task) {
                threads.push_back(thread(Functions::read[_config.rfunction], _config, task));
            }
            for (uint task=0; task<_config.rnthreads; ++task) {
                threads[task].join();
            }
        }

        timer.stop();
    }
};

int main(int argc, char* argv[]) {
    Config config;
    Parser(argc, argv, config);

    Runner runner(config);
    runner.run();
    
    return 0;
}
