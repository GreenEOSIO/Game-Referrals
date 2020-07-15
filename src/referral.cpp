#include <referral.hpp>

const name tokencontract = eosio::name("eosio.token");
const asset zero_wax=asset( 0, symbol("WAX", 8 ));

ACTION referral::configuregame(name contract,  name fee_account, string reward_per, uint8_t chain_length, uint8_t for_games, asset till_reward ) {
    require_auth( contract );
    check(chain_length<=10,"Chain length shouldn't be more than 10");

	//allow game account to be created
    games game_table( get_self(), get_self().value );
    auto g=game_table.find(contract.value);
    if(g!=game_table.end()){    
        //modify game rules
        game_table.modify( g, same_payer, [&]( auto& w ) {
            w.fee_account=fee_account;
            w.reward_per=reward_per;
            w.chain_length =chain_length;
            w.for_games=for_games;;
            w.till_reward=till_reward;
        });
    }else{
        //create rules
        game_table.emplace( contract, [&]( auto& w ) {
            w.contract = contract;
            w.fee_account=fee_account;
            w.reward_per=reward_per;
            w.chain_length =chain_length;
            w.for_games=for_games;;
            w.till_reward=till_reward;
        });
    }
}

// referral code is same as account name
// ACTION referral::referralCode(name from, name game){
// }

ACTION referral::referred(name to, name from, name gameAcct){
                          
    games game_table( get_self(), get_self().value );
    const auto& game = game_table.get(gameAcct.value , "game not configured yet" );

    require_auth( to );

    // if "to" plays, commission to "from and his chain"
    // add to table 
    // should be not more than chain_length


    refers refer_table( get_self(), gameAcct.value );
    auto r_to=refer_table.find(to.value);
    auto r_from=refer_table.find(from.value);

    check(r_to==refer_table.end(),"Already referred by someone else");

    vector<name> r_chain;

    if(r_from==refer_table.end()){
        r_chain.push_back(from);
        vector<name> empty_chain;

        refer_table.emplace( _self, [&]( auto& r ) {
                r.account = from;
                r.refer_chain=empty_chain;
                r.r_games=0;
                r.r_reward=zero_wax;
        });
    }
    else{
        r_chain= r_from->refer_chain;
        if(r_chain.size()<game.chain_length)
            // from should be pushed in front to get max reward, as direct advocate
            r_chain.emplace ( r_chain.begin(), from );
            // r_chain.push_back(from);
        else{
            // remove from end and then add
            r_chain.pop_back();
            // r_chain.push_back(from);
            r_chain.emplace ( r_chain.begin(), from );

        }  
    }
          // note this before sharing as service, RAM by contract
        refer_table.emplace( _self, [&]( auto& r ) {
                r.account = to;
                r.refer_chain=r_chain;
                r.r_games=0;
                r.r_reward=zero_wax;
        });
}


// from: game contract acct

//memo : gid,account that plays game
// check 

// if eligible for referral-> game fee distributed amongst chain
// else given to fee_account

// quantity: reward to be distributed

ACTION referral::transferfee (name from,
                              name to,
                              asset quantity,
                              string memo) {

    if ( to != get_self() ) return;
    if ( quantity.symbol != symbol( symbol_code("WAX"), 8) ) return;
    if ( memo.length() > 12 ) return;

    require_auth( from );
    check(from!=get_self(),"from account cannot be contract account");

    games game_table( get_self(), get_self().value );
    const auto& game = game_table.get(from.value , "game not configured yet" );

    uint64_t g_id;
    name player_acc;
    tie(g_id, player_acc) = parsememo(memo);

    refers refer_table( get_self(), from.value );
    auto r_acc=refer_table.find(player_acc.value);

    if(r_acc!=refer_table.end()){
        string string_precision = "precision of distribution share must be 8";
        // distribute money to chain
        vector<name> r_chain(r_acc->refer_chain);
        // float percent=quantity.amount*0.95;
        asset totalreferred=zero_wax;
        // percent.setprecision(8);
        asset share_i=asset( quantity.amount, symbol("WAX", 8 ));
        asset feeacctqty=zero_wax;

        // feeacctqty.amount=feeacctqty.amount+((share_i.amount)*0.90);
    

        for(int i=0;i<r_chain.size();i++){
            name acc_i=r_chain[i];

            // for next acct 5% of remaining
            share_i.amount=(quantity.amount-totalreferred.amount)*(1.00-(stof(game.reward_per,0.00)*0.01));
            // asset share_i=asset( percent, symbol("WAX", 8 ));
            totalreferred.amount=totalreferred.amount+share_i.amount;

            check(share_i.symbol == zero_wax.symbol, string_precision.c_str());

            auto r_i=refer_table.find(acc_i.value);

            if(r_i!=refer_table.end()){
                uint8_t games_earned=r_i->r_games+1;
                asset rewards_earned=r_i->r_reward+share_i;

                if(games_earned<=game.for_games && rewards_earned.amount<=game.till_reward.amount){

                    refer_table.modify( r_i, same_payer, [&]( auto& r ) {
                        r.r_games=games_earned;
                        r.r_reward=rewards_earned;
                    });
            
                    // totalreferred.amount=totalreferred.amount+share_i.amount;

                    action( permission_level{ get_self(), name("active") }, tokencontract, name("transfer"),
                        make_tuple( get_self(), acc_i, share_i , "referral reward, game id "+ to_string(g_id))).send();
                }
                else{
                    feeacctqty=feeacctqty+share_i;
                }
            }else{
                // remove this code block later
                vector<name> empty_chain;
                share_i.amount=(share_i.amount)*(1.00-(stof(game.reward_per,0.00)*0.01));

                refer_table.emplace( _self, [&]( auto& r ) {
                    r.account = acc_i;
                    r.refer_chain=empty_chain;
                    r.r_games=1;
                    r.r_reward=share_i;
                });

                action( permission_level{ get_self(), name("active") }, tokencontract, name("transfer"),
                    make_tuple( get_self(), acc_i, share_i , "referral reward, game id "+ to_string(g_id))).send();

            }    

        }

       
        feeacctqty.amount=quantity.amount-totalreferred.amount+feeacctqty.amount;
        if(feeacctqty.amount!=0){
            action( permission_level{ get_self(), name("active") }, tokencontract, name("transfer"),
                make_tuple( get_self(), game.fee_account, feeacctqty , "fee for game id "+ to_string(g_id))).send();
        }

    }
}




extern "C" {
    void apply (uint64_t receiver, uint64_t code, uint64_t action ) {
        auto self = receiver;

        if ( code == self ) {
            switch( action ) {
                EOSIO_DISPATCH_HELPER( referral, (configuregame)(referred)
                )
            }
        }
        else {
            if ( code == tokencontract.value && action == name("transfer").value ) {
                execute_action( name(receiver), name(code), &referral::transferfee );
            }
        }
    }
    
}