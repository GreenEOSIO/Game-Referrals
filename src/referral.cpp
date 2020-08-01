#include <referral.hpp>

const name tokencontract = eosio::name("eosio.token");
const asset zero_wax=asset( 0, symbol("WAX", 8 ));

ACTION referral::configuregame(string game_name, name contract,  name fee_account, string reward_per, uint8_t chain_length, uint8_t for_games, asset till_reward ) {
    require_auth( contract );
    check(chain_length<=10,"Chain length shouldn't be more than 10");

	//allow game account to be created
    gameaccs game_table( get_self(), get_self().value );
    auto g=game_table.find(contract.value);
    if(g!=game_table.end()){    
        //modify game rules
        game_table.modify( g, same_payer, [&]( auto& w ) {
            w.game_name=game_name;
            w.fee_account=fee_account;
            w.reward_per=reward_per;
            w.chain_length =chain_length;
            w.for_games=for_games;;
            w.till_reward=till_reward;
        });
    }else{
        //create rules
        game_table.emplace( contract, [&]( auto& w ) {
            w.game_name=game_name;
            w.contract = contract;
            w.fee_account=fee_account;
            w.reward_per=reward_per;
            w.chain_length =chain_length;
            w.for_games=for_games;;
            w.till_reward=till_reward;
            w.accs=0;
        });
    }
}


// if "to" plays, commission to "from and his chain"
// add to table 
// should be not more than chain_length

ACTION referral::referred(name to, name from, name gameAcct){
                          
    gameaccs game_table( get_self(), get_self().value );
    const auto& game = game_table.get(gameAcct.value , "game not configured yet" );

    require_auth( to );

    referaccs refer_table( get_self(), gameAcct.value );
    auto r_to=refer_table.find(to.value);
    auto r_from=refer_table.find(from.value);

    check(r_to==refer_table.end(),"Already referred by someone else");

    accountgs account_table( get_self(), get_self().value );
    auto acc=account_table.find(from.value);

    if(acc==account_table.end()){
            vector<string> v_g;
            v_g.push_back(game.game_name);
            vector<string> v_f;
            v_f.push_back(gameAcct.to_string());
            account_table.emplace( _self, [&]( auto& a ) {
            a.account=from;
            a.games = v_g;
            a.contracts=v_f;
            a.t_reward=zero_wax;
            a.t_plays=0;
        });
    }
    else{
        vector<string> cc=acc->contracts;
        if(std::find(cc.begin(),cc.end(),gameAcct.to_string())!=cc.end())
        {
            vector<string> gc=acc->games;
            gc.push_back(game.game_name);
            cc.push_back(gameAcct.to_string());
            account_table.modify( acc, same_payer, [&]( auto& a ) {
                a.games=gc;
                a.contracts=cc;
            });
        }
    }

    vector<name> r_chain;
    map<name,uint8_t> me_chains;
    map<uint32_t,asset> dates;

    if(r_from==refer_table.end()){
        r_chain.push_back(from);
        vector<name> empty_chain;
        me_chains[to]=1;

        refer_table.emplace( _self, [&]( auto& r ) {
                r.account = from;
                r.refer_chain=empty_chain;
                r.r_games=0;
                r.r_reward=zero_wax;
                r.me_chains=me_chains;
                r.dates=dates;
        });
    }
    else{

        map<name,uint8_t> me_chains_from;
        me_chains_from[to]=1;
        refer_table.modify( r_from, same_payer, [&]( auto& r ) {
            r.me_chains=me_chains_from;
        });


        r_chain= r_from->refer_chain;
        if(r_chain.size()<game.chain_length)
            // from should be pushed in front to get max reward, as direct advocate
            r_chain.emplace ( r_chain.begin(), from );
            
        else{
            // remove from end and then add
            r_chain.pop_back();
            r_chain.emplace ( r_chain.begin(), from );

        }  
    }

        map<name,uint8_t> empty_me_chains;

        refer_table.emplace( _self, [&]( auto& r ) {
                r.account = to;
                r.refer_chain=r_chain;
                r.r_games=0;
                r.r_reward=zero_wax;
                r.me_chains=empty_me_chains;
                r.dates=dates;
        });


        // me chain updates, the chains u are part of
        for(int i=0;i<r_chain.size();i++){
            name acc_i=r_chain[i];
            auto r_i=refer_table.find(acc_i.value);
            map<name,uint8_t> me_i_chain=r_i->me_chains;
            me_i_chain[to]=i+1;// added 1 for shift

            refer_table.modify( r_i, same_payer, [&]( auto& r ) {
                r.me_chains=me_i_chain;
                });
        }
        
        auto g=game_table.find(gameAcct.value);
        uint8_t accs=game.accs+1;
        game_table.modify( g, same_payer, [&]( auto& w ) {
            w.accs=accs;
        });
}


// from: game contract acct

//memo : gid,account that plays game

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

    gameaccs game_table( get_self(), get_self().value );
    const auto& game = game_table.get(from.value , "game not configured yet" );

    uint64_t g_id;
    name player_acc;
    tie(g_id, player_acc) = parsememo(memo);

    referaccs refer_table( get_self(), from.value );
    auto r_acc=refer_table.find(player_acc.value);
    
    accountgs account_table( get_self(), get_self().value );
    
    if(r_acc!=refer_table.end()){
        string string_precision = "precision of distribution share must be 8";
        // distribute money to chain
        vector<name> r_chain(r_acc->refer_chain);
        asset totalreferred=zero_wax;
        asset share_i=asset( quantity.amount, symbol("WAX", 8 ));
        asset feeacctqty=zero_wax;

        for(int i=0;i<r_chain.size();i++){
            name acc_i=r_chain[i];

            // for next acct x% of remaining
            share_i.amount=(quantity.amount-totalreferred.amount)*(1.00-(stof(game.reward_per,0.00)*0.01));
            totalreferred.amount=totalreferred.amount+share_i.amount;

            check(share_i.symbol == zero_wax.symbol, string_precision.c_str());

            auto r_i=refer_table.find(acc_i.value);

            if(r_i!=refer_table.end()){
                uint8_t games_earned=r_i->r_games+1;
                asset rewards_earned=r_i->r_reward+share_i;
                map<uint32_t,asset> dates=r_i->dates;
                uint32_t now=eosio::current_time_point().sec_since_epoch();


                dates[now]=share_i;

                if(games_earned<=game.for_games && rewards_earned.amount<=game.till_reward.amount){

                    refer_table.modify( r_i, same_payer, [&]( auto& r ) {
                        r.r_games=games_earned;
                        r.r_reward=rewards_earned;
                        r.dates=dates;
                    });
            

                    auto acc=account_table.find(acc_i.value);

                        if(acc==account_table.end()){
                            vector<string> v_g;
                            v_g.push_back(game.game_name);
                            vector<string> v_f;
                            v_f.push_back(from.to_string());
                                
                            account_table.emplace( _self, [&]( auto& a ) {
                                a.account=acc_i;
                                a.games = v_g;
                                a.contracts=v_f;
                                a.t_reward=share_i;
                                a.t_plays=1;
                            });
                        }
                        else{
                            vector<string> cc=acc->contracts;
                            vector<string> gc=acc->games;
                            if(std::find(cc.begin(),cc.end(),from.to_string())==cc.end())
                            {
                                gc.push_back(game.game_name);
                                cc.push_back(from.to_string());
                            }

                            asset t_rewards=acc->t_reward+share_i;
                            uint8_t t_plays=acc->t_plays+1;

                            account_table.modify( acc, same_payer, [&]( auto& a ) {
                                a.games=gc;
                                a.contracts=cc;
                                a.t_reward=t_rewards;
                                a.t_plays=t_plays;
                            });
                            
                        }


                    action( permission_level{ get_self(), name("active") }, tokencontract, name("transfer"),
                        make_tuple( get_self(), acc_i, share_i , "referral reward, game id "+ to_string(g_id))).send();
                }
                else{
                    feeacctqty=feeacctqty+share_i;
                }
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