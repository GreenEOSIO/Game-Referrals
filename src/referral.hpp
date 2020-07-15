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

        ACTION configuregame(name contract,  name fee_account, string reward_per, uint8_t chain_length, uint8_t for_games, asset till_reward );

        ACTION referred(name to, name from, name gameAcct);

        ACTION transferfee(name from, name to, asset quantity, string memo);

        TABLE game {
            name contract;
            name fee_account;
            string reward_per;
            uint8_t chain_length;
            uint8_t for_games;
            asset till_reward;
            uint64_t primary_key() const { return contract.value; }
        };
        EOSLIB_SERIALIZE( game, (contract)(fee_account)(reward_per)
        (chain_length)(for_games)(till_reward))


        TABLE refer {
            name account;
            vector<name> refer_chain;
            uint8_t r_games;
            asset r_reward;
            uint64_t primary_key() const { return account.value; }
        };
        EOSLIB_SERIALIZE( refer, (account)(refer_chain)(r_games)(r_reward) )



        using games = multi_index< "game"_n, game>;
        using refers = multi_index< "refer"_n, refer>;


};