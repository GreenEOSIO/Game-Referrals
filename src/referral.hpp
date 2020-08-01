#pragma once

#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/singleton.hpp>
#include<vector>
#include "utility.hpp"

using namespace std;
using namespace eosio;
using namespace utility;

CONTRACT referral: public contract {
    public:
        using contract::contract;
        
        referral(name receiver, name code, datastream<const char*> ds)
            : contract(receiver, code, ds) {}

        ACTION configuregame(string game_name, name contract,  name fee_account, string reward_per, uint8_t chain_length, uint8_t for_games, asset till_reward );

        ACTION referred(name to, name from, name gameAcct);

        ACTION transferfee(name from, name to, asset quantity, string memo);

        TABLE gameacc {
            string game_name;// for frontend
            name contract;
            name fee_account;
            string reward_per;
            uint8_t chain_length;
            uint8_t for_games;
            asset till_reward;
            uint8_t accs;// total accs that have acc
            uint64_t primary_key() const { return contract.value; }
        };
        EOSLIB_SERIALIZE( gameacc, (game_name)(contract)(fee_account)(reward_per)
        (chain_length)(for_games)(till_reward)(accs))


        TABLE referacc{
            name account;
            vector<name> refer_chain;
            uint8_t r_games;
            asset r_reward;
            map<name,uint8_t> me_chains;
            map<uint32_t,asset> dates;
            uint64_t primary_key() const { return account.value; }
        };
        EOSLIB_SERIALIZE( referacc, (account)(refer_chain)(r_games)(r_reward)(me_chains)(dates) )


        TABLE accountg {
            name account;
            vector<string> games;
            vector<string> contracts;
            asset t_reward;
            uint8_t t_plays;// referred games played
            uint64_t primary_key() const { return account.value; }
        };
        EOSLIB_SERIALIZE( accountg, (account)(games)(contracts)(t_reward)(t_plays) )


        using gameaccs = multi_index< "gameacc"_n, gameacc>;
        using referaccs = multi_index< "referacc"_n, referacc>;
        using accountgs=multi_index< "accountg"_n, accountg>;


};