#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <eosio/eosio.hpp>
#include <math.h>

using namespace std;
using namespace eosio;

namespace utility {

    // trim from left (in place)
    static inline void ltrim(string& s) {
        s.erase( s.begin(), find_if( s.begin(), s.end(), [](int ch) {
            return !isspace(ch);
        }));
    }

    // trim from right (in place)
    static inline void rtrim(string& s) {
        s.erase( find_if( s.rbegin(), s.rend(), [](int ch) {
            return !isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline string trim(string s) {
        ltrim(s);
        rtrim(s);
        return s;
    }

    tuple<uint64_t, name> parsememo(const string& memo) {
        auto dot_pos = memo.find(',');
        string errormsg = "malformed memo: must have game_id,player_acc";
        check ( dot_pos != string::npos, errormsg.c_str() );
        if ( dot_pos != string::npos ) {
            check( ( dot_pos != memo.size() - 1 ), errormsg.c_str() );
        }
        // will abort if stoull throws error since wasm no error checking
        uint64_t game_id = stoull( trim( memo.substr( 0, dot_pos ) ) );
        name player_acc = name( trim ( memo.substr( dot_pos + 1 ) ) );

        return make_tuple(game_id, player_acc);
    }

    float stof(std::string s, float def)
    {   
      if (s == "") return def;
      std::size_t i = s.find(".");
      int digits = s.length() - i - 1;
      s.erase(i, 1); 
      return atoi(s.c_str()) / pow(10, digits);
    }

}

