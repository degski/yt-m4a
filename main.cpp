
// MIT License
//
// Copyright (c) 2020 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <fcntl.h>
#include <io.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#include <sax/utf8conv.hpp>

#if 0
#    define SYSTEM std::wprintf
#else
#    define SYSTEM _wsystem
#endif

namespace fs = std::filesystem;

// Function declarations.

void print_exe_info ( wchar_t * name_, std::size_t size_ ) noexcept;

[[nodiscard]] std::vector<std::string> get_urls ( fs::path const & file_ );

template<typename Stream>
[[nodiscard]] std::string get_line ( Stream & in_ );

[[nodiscard]] std::string get_source ( std::string const & url_ );

[[nodiscard]] std::tuple<fs::path, fs::path> get_source_target ( std::string const & url_ ) noexcept;

[[nodiscard]] std::tuple<std::string, std::string> get_artist_title ( fs::path const & path_ ) noexcept;

// Main.

int wmain ( int argc_, wchar_t * argv_[], wchar_t *[] ) {
    std::vector<std::string> urls = get_urls ( fs::current_path ( ) / "download.txt" );
    print_exe_info ( argv_[ 0 ], urls.size ( ) );
    for ( std::string const & url : urls ) {
        auto [ source, target ] = get_source_target ( url );
        auto [ artist, title ]  = get_artist_title ( source );
        if ( fs::exists ( source ) )
            fs::remove ( source );

        // clang-format on

        SYSTEM ( sax::utf8_to_utf16 (
                     ( std::string{ "youtube-dl.exe --socket-timeout 10 --limit-rate 100K --buffer-size 16K --geo-bypass "
                                    "--extract-audio --audio-format best --audio-quality 0 --yes-playlist -o \"" } +
                       source.filename ( ).string ( ) + std::string{ "\" \"" } + url + std::string{ "\"" } ) )
                     .c_str ( ) );

        std::cout << nl;

        if ( not fs::exists ( target ) ) {

            SYSTEM (
                sax::utf8_to_utf16 (
                    ( std::string{ "opusdec.exe --rate 44100 \"" } + source.filename ( ).string ( ) + std::string{ "\" - | " } +
                      std::string{ "fdkaac.exe --bitrate-mode 5 --gapless-mode 1 --no-timestamp --artist \"" + artist +
                                   "\" --title \"" + title +
                                   "\" --comment \"down-sampled (sox-14.4.2) and re-encoded to m4a from lossy opus-source "
                                   "(fdkaac-1.0.0)\" - -o \"" +
                                   target.filename ( ).string ( ) + "\"" } ) )
                    .c_str ( ) );

            std::cout << nl;

            // clang-format on
        }
    }

    return EXIT_SUCCESS;
}

// Function definitions.

void print_exe_info ( wchar_t * name_, std::size_t size_ ) noexcept {
    _setmode ( _fileno ( stdout ), _O_U16TEXT );
    std::wstring exename = { name_ };
    if ( exename.size ( ) and L':' == exename[ 1 ] and
         std::string::npos != exename.find ( L"\\" ) ) // Is it actually a path in string-form? (doing some light checks)
        exename = fs::path{ exename }.filename ( );
    std::wcout << exename << L"-0.1 - copyright ©" << L" 2020 degski\ndownloading " << size_ << L" files started . . ." << nl;
}

[[nodiscard]] std::vector<std::string> get_urls ( fs::path const & file_ ) {
    std::vector<std::string> list;
    list.reserve ( 32 );
    list.resize ( 1 );
    std::ifstream in ( file_ );
    if ( not in )
        throw ( "cannot open the file : " + file_.string ( ) );
    while ( std::getline ( in, list.back ( ) ) ) {
        if ( list.back ( ).size ( ) )
            list.resize ( list.size ( ) + 1 );
    }
    in.close ( );
    list.resize ( list.size ( ) - list.back ( ).empty ( ) );
    return list;
}

template<typename Stream>
[[nodiscard]] std::string get_line ( Stream & in_ ) {
    std::string l;
    std::getline ( in_, l );
    if ( l.size ( ) )
        l.pop_back ( );
    return l;
}

[[nodiscard]] std::string get_source ( std::string const & url_ ) {
    _wsystem (
        sax::utf8_to_utf16 ( ( std::string ( "youtube-dl.exe --socket-timeout 5 --retries 20 --skip-download --get-title " ) +
                               url_ + std::string ( " > " ) + "c:\\tmp\\tmp.txt" ) )
            .c_str ( ) );
    std::ifstream stream{ fs::path{ "c:\\tmp\\tmp.txt" } };
    std::string filename = { get_line ( stream ) + ".opus" };
    stream.close ( );
    fs::remove ( "c:\\tmp\\tmp.txt" );
    return filename;
}

[[nodiscard]] std::tuple<fs::path, fs::path> get_source_target ( std::string const & url_ ) noexcept {
    fs::path opus_source = fs::current_path ( ) / get_source ( url_ ), m4a_local_target = opus_source.replace_extension ( ".m4a" ),
             m4a_player_target = fs::path{ "F:/Music" } / m4a_local_target.filename ( ),
             m4a_target        = fs::exists ( m4a_player_target.parent_path ( ) ) ? m4a_player_target : m4a_local_target;
    return { std::move ( opus_source ), std::move ( m4a_target ) };
}

[[nodiscard]] std::tuple<std::string, std::string> get_artist_title ( fs::path const & path_ ) noexcept {
    std::size_t pos = path_.stem ( ).string ( ).find ( "-" ), len = std::string::npos != pos;
    if ( std::string::npos != pos ) {
        if ( pos > 0 and ' ' == path_.string ( )[ pos - 1 ] )
            pos -= 1, len += 1;
        if ( pos < ( path_.stem ( ).string ( ).length ( ) - 1 ) - 1 and ' ' == path_.string ( )[ pos + 1 ] )
            len += 1;
    }
    std::string artist = std::string::npos != pos ? path_.stem ( ).string ( ).substr ( 0, pos ) : std::string{ },
                title  = std::string::npos != pos ? path_.stem ( ).string ( ).substr (
                                                       std::string::npos != pos ? pos + len : std::string::npos, std::string::npos )
                                                 : path_.stem ( ).string ( );
    return { std::move ( artist ), std::move ( title ) };
}
