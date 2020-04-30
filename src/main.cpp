/*
 * Copyright 2020 Florent Bondoux
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"

#include <cstdio>
#include <cstring>
#include <cinttypes>

#include <string>
#include <map>
#include <typeinfo>

#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>

#include "ReadMasks.h"
#include "utf_conv.h"

using namespace Maskgen;

static void usage() {
    const char *help_string = 
    "Usage: maskgen [OPTIONS] (mask|maskfile)\n"
    "Generate words based on templates (masks) describing each position's charset\n"
    "\n"
    " Behavior:\n"
    "  -u, --unicode                Allow UTF-8 characters in the charsets\n"
    "                               Without this option, the charsets can only\n"
    "                               contain 8-bit (ASCII compatible) values\n"
    "                               This option slows down the generation and\n"
    "                               disables the '?b' built-in charset"
    "\n"
    " Range:\n"
    "  -j, --job=J/N                Divide the generation in N equal parts and\n"
    "                               generate the 'J'th part (counting from 1)\n"
    "  -b, --begin=N                Start the generation at the Nth word\n"
    "                               counting from 0\n"
    "  -e, --end=N                  Stop after the Nth word counting from 0\n"
    "\n"
    " Output control:\n"
    "  -o, --output=FILE            Write the words into FILE\n"
    "  -z, --zero                   Use the null character as a word delimiter\n"
    "                               instead of the newline character\n"
    "  -n, --no-delim               Don't use a word delimiter\n"
    "  -s, --size                   Show the number of words that will be\n"
    "                               generated and exit\n"
    "  -h, --help                   Show this help message and exit\n"
    "      --version                Show the version number and exit\n"
    "\n"
    " Charsets:\n"
    "  A charset is a named variable describing a list of characters. Unless\n"
    "  the --unicode option is used, only 8-bit characters are allowed.\n"
    "  The name of a charset is a single character. It is reffered using '?'\n"
    "  followed by its name (example : ?d). A charset definition can refer to\n"
    "  other named charsets.\n"
    "\n"
    "  Built-in charsets:\n"
    "   ?l = abcdefghijklmnopqrstuvwxyz\n"
    "   ?u = ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
    "   ?d = 0123456789\n"
    "   ?s =  !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\n"
    "   ?a = ?l?u?d?s\n"
    "   ?h = 0123456789abcdef\n"
    "   ?H = 0123456789ABCDEF\n"
    "   ?n = \\n (new line)\n"
    "   ?r = \\r (carriage ret)\n"
    "   ?b = 0x00 - 0xFF (only without --unicode)\n"
    "\n"
    "  Custom charsets:\n"
    "   Custom named charsets are defined either inline or by reading a file.\n"
    "   To include a single '?' in a charset, escape it with another '?' (?\?).\n"
    "   Pay attention to trailing newline when reading a file or to the shell\n"
    "   expansion for inline definitions ('?' or '*' chars...)\n"
    "\n"
    "   -1, --custom-charset1=CS    Define the charsets named '1', '2', '3' or\n"
    "   -2, --custom-charset2=CS    '4'. The argument is either the content of\n"
    "   -3, --custom-charset3=CS    the charset or a file to read.\n"
    "   -4, --custom-charset4=CS\n"
    "\n"
    "   -c, --charset=K:CS          Define a charset named 'K' with the content\n"
    "                               'CS'. 'K' may be an UTF-8 char onlye if\n"
    "                               --unicode is used. Otherwise it's a single\n"
    "                               8-bit char.\n"
    "\n"
    " Masks:\n"
    "  Masks are templates defining which characters are allowed for each\n"
    "  positions. Masks are single line strings build by concatenating\n"
    "  for each positions either: \n"
    "  - a static character\n"
    "  - a charset reference indicated by a '?' followed by the charset\n"
    "    name\n"
    "  For example, '@?u?l?l?l?d@' would generate the words from\n"
    "  '@Aaaa0@' to '@Zzzz9@'\n"
    "\n"
    "  The mask argument is either a single mask definition or a file\n"
    "  containing a list of masks definitions.\n"
    "\n"
    "  Mask files can also embed charset definitions. The general syntax for\n"
    "  a single line is:\n"
    "   [:1:,][:2:,]...[:9:,]:mask:\n"
    "  where the placeholders are as follows:\n"
    "   :1: the named custom charset '1' (overrides --custom-charset1 or \n"
    "        --charset) will be set to this value, optional\n"
    "   :2: the named custom charset '2' (overrides --custom-charset2 or \n"
    "       --charset) will be set to this value, optional\n"
    "   ...\n"
    "   :9: the named custom charset '9' (overrides --charset) will be set to\n"
    "       this value, optional\n"
    "   :mask: the mask which may refer to the previously defined charsets\n"
    "\n"
    "  The characters ',' and '?' can be escaped by writing '?,' or '?\?'.\n"
    "\n"
    ;
    
    printf("%s", help_string);
}

struct Options {
    bool m_unicode;
    uint64_t m_job_number;
    uint64_t m_job_total;
    bool m_job_set;
    uint64_t m_start_word;
    bool m_start_word_set;
    uint64_t m_end_word;
    bool m_end_word_set;
    std::string m_output_file;
    bool m_zero_delim;
    bool m_no_delim;
    bool m_print_size;
    std::map<char, std::string> m_charsets_short_defs; // for -1, -2, ... -4 arguments
    std::vector<std::string> m_charsets_long_defs; // for "-c k:def" arguments (unparsed)
    
    Options() :
    m_unicode(false)
    , m_job_number(), m_job_total(), m_job_set(false)
    , m_start_word(), m_start_word_set(false), m_end_word(), m_end_word_set(false)
    , m_output_file()
    , m_zero_delim(false), m_no_delim(false)
    , m_print_size(false)
    , m_charsets_short_defs()
    , m_charsets_long_defs()
    {}
};

struct Helper8bit {
    class Print8bit
    {
    public:
        inline void print(const char *buffer, size_t len, int fdout)
        {
            if (write(fdout, buffer, len) != (ssize_t) len) {
                fprintf(stderr, "Error while writing the output data : %m");
                exit(1);
            }
        }
    };
    typedef char CharType;
    typedef Print8bit Printer;
    static inline void initDefaultCharsets(CharsetMap<char> &map)
    {
        return initDefaultCharsetsAscii(map);
    }
    static inline bool readCharset(const char *spec, std::vector<char> &charset)
    {
        return readCharsetAscii(spec, charset);
    }
    static bool parseCharsetArg(const char *spec, char &key, std::vector<char> &charset) {
        size_t spec_len = strlen(spec);
        if (spec_len < 3 || spec[1] != ':') {
            return false;
        }
        key = spec[0];
        return readCharsetAscii(spec + 2, charset);
    }
    static inline std::vector<char> expandCharset(const std::vector<char> &charset, const CharsetMap<char> &default_charsets, char charset_name)
    {
        return expandCharsetAscii(charset, default_charsets, charset_name);
    }
    static inline bool readMaskList(const char *spec, const CharsetMap<char> &default_charsets, MaskList<char> &ml)
    {
        return readMaskListAscii(spec, default_charsets, ml);
    }
};

struct HelperUnicode {
    class PrintUnicode
    {
        char *conv_buffer;
        size_t conv_buffer_size;

    public:
        PrintUnicode() : conv_buffer(NULL), conv_buffer_size(0) {}
        ~PrintUnicode()
        {
            free(conv_buffer);
        };
        inline void print(const uint32_t *buffer, size_t len, int fdout)
        {
            size_t consumed = 0, written = 0;
            if (UTF::encode_utf8(buffer, len, &conv_buffer, &conv_buffer_size, &consumed, &written) == UTF::RetCode::OK) {
                if (write(fdout, conv_buffer, written) != (ssize_t) written) {
                    fprintf(stderr, "Error while writing the output data : %m");
                    exit(1);
                }
            } else {
                fprintf(stderr, "Error: could not encode the generated words into UTF-8\n");
                exit(1);
            }
        }
    };
    typedef uint32_t CharType;
    typedef PrintUnicode Printer;
    static inline void initDefaultCharsets(CharsetMap<uint32_t> &map)
    {
        return initDefaultCharsetsUnicode(map);
    }
    static inline bool readCharset(const char *spec, std::vector<uint32_t> &charset)
    {
        return readCharsetUtf8(spec, charset);
    }
    static bool parseCharsetArg(const char *spec, uint32_t &key, std::vector<uint32_t> &charset)
    {
        // need to parse K:file_or_charset
        // where K may be an UTF-8 char
        // first pick the first char
        // then check that the following char is ASCII ':'
        size_t spec_len = strlen(spec);
        size_t consumed = 0;
        if (UTF::decode_one_utf8(spec, spec_len, &key, &consumed) != UTF::RetCode::OK) {
            return false;
        }
        spec += consumed;
        spec_len -= consumed;
        if (*spec != ':') {
            return false;
        }
        spec += 1;
        spec_len --;
        
        return readCharsetUtf8(spec, charset);
    }
    static inline std::vector<uint32_t> expandCharset(const std::vector<uint32_t> &charset, const CharsetMap<uint32_t> &default_charsets, uint32_t charset_name)
    {
        return expandCharsetUnicode(charset, default_charsets, charset_name);
    }
    static inline bool readMaskList(const char *spec, const CharsetMap<uint32_t> &default_charsets, MaskList<uint32_t> &ml)
    {
        return readMaskListUtf8(spec, default_charsets, ml);
    }
};

template<typename T>
int work(const struct Options &options, const char *mask_arg) {
    static_assert(std::is_same<T, char>::value || std::is_same<T, uint32_t>::value, "word requires char or uint32_t as template parameter");
    typedef typename std::conditional<std::is_same<T, char>::value, Helper8bit, HelperUnicode>::type Helper;
    CharsetMap<T> charsets; // create our built-in charsets
    Helper::initDefaultCharsets(charsets);
    
    // create the charsets from the "-1, -2... -4" command line arguments
    for (auto p : options.m_charsets_short_defs) {
        std::vector<T> charset;
        if (!Helper::readCharset(p.second.c_str(), charset)) {
            fprintf(stderr, "Error while reading the charset '%lc'\n", p.first);
            return 1;
        }
        charsets[p.first] = DefaultCharset<T>(charset, false);
    }
    // create the charsets from the "-c" command line arguments
    for (const auto &s : options.m_charsets_long_defs) {
        T key;
        std::vector<T> charset;
        if (!Helper::parseCharsetArg(s.c_str(), key, charset)) {
            fprintf(stderr, "Error while reading the charset definition '%s'\n", s.c_str());
            return 1;
        }
        charsets[key] = DefaultCharset<T>(charset, false);
    }
    
    // expand all the unexpanded charsets
    for (auto &p : charsets) {
        if (p.second.final) {
            continue;
        }
        p.second.cset = Helper::expandCharset(p.second.cset, charsets, p.first);
        if (p.second.cset.empty()) {
            fprintf(stderr, "Error while expanding the charset '%lc' (undefined charset ?)\n", p.first);
            return 1;
        }
        p.second.final = true;
    }
    
    // now read our masks
    MaskList<T> ml;
    if (!Helper::readMaskList(mask_arg, charsets, ml)) {
        fprintf(stderr, "Error while reading the mask definition '%s'\n", mask_arg);
        return 1;
    }
    
    uint64_t ml_len = ml.getLen();
    
    uint64_t start_idx = 0;
    uint64_t end_idx = ml_len; // after the last word
    
    if (options.m_job_set) {
        // create our staring position and the number of word to generate
        // from a job spec
        // the remainder is distributed on the first jobs
        uint64_t q = ml_len / options.m_job_total;
        uint64_t r = ml_len - q * options.m_job_total;
        
        uint64_t todo = q;
        start_idx = q * (options.m_job_number - 1);
        if (r != 0) {
            start_idx += std::min((uint64_t) options.m_job_number - 1, r);
            todo += options.m_job_number <= r ? 1 : 0;
        }
        end_idx = start_idx + todo;
    }
    else {
        if (options.m_start_word_set) {
            start_idx = options.m_start_word;
        }
        if (options.m_end_word_set) {
            end_idx = options.m_end_word + 1;
        }
        
        if (end_idx - 1 < start_idx || end_idx > ml_len) {
            fprintf(stderr, "Error: the last word number is not valid\n");
            return 1;
        }
    }
    
    if (options.m_print_size) {
        printf("%" PRId64 "\n", end_idx - start_idx);
        return 0;
    }
    
    typename Helper::Printer printer;
    std::vector<T> buffer(8192);
    T * buffer_p = buffer.data();
    const T * buffer_end = buffer.data() + buffer.size();
    std::vector<T> word(ml.getMaxWidth() + 1);
    if (word.size() > buffer.size()) {
        fprintf(stderr, "Error: do you reallly intend to generate words of length over %zu ?\n", buffer.size());
        return 1;
    }
    
    // at last create the output file if needed now that we're not supposed to fail anymore
    int fdout = STDOUT_FILENO;
    if (!options.m_output_file.empty()) {
#if defined(__WINDOWS__)
        fdout = open(options.m_output_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, S_IRUSR|S_IWUSR);
#else
        fdout = open(options.m_output_file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR|S_IWUSR);
#endif
        if (fdout < 0) {
            fprintf(stderr, "Error: can't open the output file : %m\n");
            return 1;
        }
    }
    
    
    T delim = options.m_zero_delim ? '\0' : '\n';
    int delim_width = options.m_no_delim ? 0 : 1;
    uint64_t todo = end_idx - start_idx;
    ml.setPosition(start_idx);
    
    size_t w;
    // first word, use MaskList::getFirstWord
    if (todo >= 1) {
        ml.getFirstWord(word.data(), &w);
        word[w] = delim;
        if (w + delim_width > size_t(buffer_end - buffer_p)) {
            printer.print(buffer.data(), buffer_p - buffer.data(), fdout);
            buffer_p = buffer.data();
        }
        
        memcpy(buffer_p, word.data(), sizeof(T) * (w + delim_width));
        buffer_p += w + delim_width;
    }
    // following words, use MaskList::getNext
    for (uint64_t i = 1; i < todo; i++) {
        ml.getNext(word.data(), &w);
        word[w] = delim;
        if (w + delim_width > size_t(buffer_end - buffer_p)) {
            printer.print(buffer.data(), buffer_p - buffer.data(), fdout);
            buffer_p = buffer.data();
        }
        
        memcpy(buffer_p, word.data(), sizeof(T) * (w + delim_width));
        buffer_p += w + delim_width;
    }
    
    printer.print(buffer.data(), buffer_p - buffer.data(), fdout);
    if (fdout != STDOUT_FILENO) {
        close(fdout);
    }
    return 0;
}

int real_main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    Options options;
    
    struct option longopts[] = {
        {"unicode", no_argument, NULL, 'u'},
        {"job", required_argument, NULL, 'j'},
        {"begin", required_argument, NULL, 'b'},
        {"end", required_argument, NULL, 'e'},
        {"output", required_argument, NULL, 'o'},
        {"zero", no_argument, NULL, 'z'},
        {"no-delim", no_argument, NULL, 'n'},
        {"size", no_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"custom-charset1", required_argument, NULL, '1'},
        {"custom-charset2", required_argument, NULL, '2'},
        {"custom-charset3", required_argument, NULL, '3'},
        {"custom-charset4", required_argument, NULL, '4'},
        {"charset", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}
    };
    const char *shortopt = "uj:b:e:o:znsh1:2:3:4:c:";
    
    int opt;
    while ((opt = getopt_long(argc, argv, shortopt, longopts, 0)) >= 0) {
        switch (opt) {
            case 'u':
                options.m_unicode = true;
                break;
            case 'j':
            {
                int r = sscanf(optarg, "%" PRIu64 "/%" PRIu64, &options.m_job_number, &options.m_job_total);
                if (r != 2 || options.m_job_number > options.m_job_total || options.m_job_number == 0) {
                    fprintf(stderr, "Error: wrong job number specification (%s)\n", optarg);
                    return 1;
                }
                options.m_job_set = true;
            }
                break;
            case 'b':
            {
                int r = sscanf(optarg, "%" PRIu64, &options.m_start_word);
                if (r != 1) {
                    fprintf(stderr, "Error: wrong starting word number specification (%s)\n", optarg);
                    return 1;
                }
                options.m_start_word_set = true;
            }
                break;
            case 'e':
            {
                int r = sscanf(optarg, "%" PRIu64, &options.m_end_word);
                if (r != 1) {
                    fprintf(stderr, "Error: wrong last word number specification (%s)\n", optarg);
                    return 1;
                }
                options.m_end_word_set = true;
            }
                break;
            case 'o':
                options.m_output_file = std::string(optarg);
                break;
            case 'z':
                options.m_zero_delim = true;
                break;
            case 'n':
                options.m_no_delim = true;
                break;
            case 's':
                options.m_print_size = true;
                break;
            case 'h':
                usage();
                return 0;
            case 'V':
                fprintf(stdout, "Maskgen version %s\n", MASKGEN_VERSION_STRING);
                fprintf(stdout, "This sofware is distributed under the Apache License version 2.0\n");
                return 0;
            case '1':
            case '2':
            case '3':
            case '4':
                options.m_charsets_short_defs[opt] = std::string(optarg);
                break;
            case 'c':
                options.m_charsets_long_defs.emplace_back(optarg);
                break;
            default:
                usage();
                return 1;
        }
    }
    
    argc -= optind;
    argv += optind;
    
    if (argc != 1) {
        fprintf(stderr, "Error: wrong number of arguments\n");
        return 1;
    }
    
    const char *mask_arg = argv[0];
    
    if (!options.m_unicode) {
        int r = work<char>(options, mask_arg);
        return r;
    }
    else {
        int r = work<uint32_t>(options, mask_arg);
        return r;
    }

    return 0;
}

#if defined(__WINDOWS__)
#include <windows.h>
#include <io.h>

static int restore_output_cp = CP_UTF8;
/*
 * Restore the original console output codepage on exit
 * Note that this will not be called on abort() so we may still trash the console
 */
static void exit_restore_output_cp() {
    SetConsoleOutputCP(restore_output_cp);
}

/* For the Windows version:
 * - set the console output codepage to UTF-8
 * - set stdout as binary to disable the destroying of \n into \r\n
 * - call the standard main
 */
int main(int argc, char **argv) {
    restore_output_cp = GetConsoleOutputCP();
    if (restore_output_cp != CP_UTF8) {
        SetConsoleOutputCP(CP_UTF8);
        atexit(&exit_restore_output_cp);
    }
    _setmode(_fileno(stdout), _O_BINARY);

    int r = real_main(argc, argv);

    return r;
}

#if defined(UNICODE)

/* For the unicode Windows version:
 * - set the console output codepage to UTF-8
 * - set stdout as binary to disable the destroying of \n into \r\n
 * - convert the UTF-16 arguments into UTF-8
 * - call the standard main
 */
int wmain(int argc, wchar_t **argv) {
    restore_output_cp = GetConsoleOutputCP();
    if (restore_output_cp != CP_UTF8) {
        SetConsoleOutputCP(CP_UTF8);
        atexit(&exit_restore_output_cp);
    }
    _setmode(_fileno(stdout), _O_BINARY);

    char ** argv_utf8 = (char **) malloc((argc + 1) * sizeof(char *));
    for (int i = 0; i < argc; i++) {
        argv_utf8[i] = NULL;
    }
    argv_utf8[argc] = NULL;

    int r = 0;

    for (int i = 0; i < argc; i++) {
        int conv_size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
        if (conv_size == 0) {
            fprintf(stderr, "Error: can't decode the input parameters (reading Windows console UTF-16)\n");
            r = 1;
            goto clean_exit;
        }
        argv_utf8[i] = (char *) malloc(conv_size);
        conv_size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argv_utf8[i], conv_size, NULL, NULL);
        if (conv_size == 0) {
            fprintf(stderr, "Error: can't decode the input parameters (reading Windows console UTF-16)\n");
            r = 1;
            goto clean_exit;
        }
    }

    r = real_main(argc, argv_utf8);

clean_exit:
    for (int i = 0; i < argc; i++) {
        free(argv_utf8[i]);
    }
    free(argv_utf8);

    return r;
}

#endif /* UNICODE */

#else /* !WINDOWS */

int main(int argc, char **argv) {
    return real_main(argc, argv);
}

#endif /* WINDOWS */
