## Game Referral

This is an open source smart contract for managing the referrals and pay the referral reward directly to the players via this smart contract.

The contracts is currently deployed on:

<b>WAX mainnet</b> -> [gamereferral](https://wax.bloks.io/account/gamereferral?loadContract=true&tab=Tables&account=gamereferral&limit=100)              

### ARCHITECTURE
We use the pyramid model to give rewards to both direct as well as indirect referrals. 

Consider chain:  Alice->Bob->Charlie (Alice refers Bob, Bob refers Charlie)

In this case, if Charlie plays a game, Bob (direct referrer) receives reward (x%) as well as Charlie recieves (x% of the remaining). For 100 WAX referral reward means, Bob receives 95 WAX and Charlie receives 5 WAX.

There can be no overdrawn balance, the x% is adjusted by the next immediate chain account only (in above case by Bob). 

### ACTIONS:

* `configuregame`: callable by game account once or later to change game referral rules. 
* `referred` : should be called once by referred account `to` stating the account name that referred `from` and the game account `gameAcct`. This shall be integrated in the frontend of your dapp.
* `transferfee` : should be called by the game account sending the game fee to be used for giving referral rewards. Memo shoud contain the `game_id,account name` to which referral reward is to be given. This shall be integrated in your game smart contract where you transfer the game fee to your account. 

#### configuregame
   
```
ACTION configuregame (name contract, 
                      name fee_account,
                      string reward_per, 
                      uint8_t chain_length, 
                      uint8_t for_games, 
                      asset till_reward )
```
* `contract`: account name of the game contract
* `fee_account`: the account where fee is collected in case referred account reaches threshold.
* `reward_per`: Reward percentage eg. 5.00% reward means each subsequent account gets 5% of remaining reward. Should be in format `X.YZ` format.
* `chain_length`: the length of the pyramid chain, must be <=10.
* `for_games`: the total no. of games for which account can gain referral reward.
* `till_reward`: the total max reward that the player account can gain.


#### referred

```
ACTION referred (name to, 
                 name from, 
                 name gameAcct)
```
* `from`: the account name that refers
* `to`: the account that is referred.
* `gameAcct`: game account name 


#### transferfee

```
ACTION transferfee (name from,
                    name to,
                    asset quantity,
                    string memo)
```
* `from`: the account name that refers
* `to`: should be `gamereferral` for WAX-mainnet
* `quantity`: amount to be used for referral rewards
* `memo`: should be in format `GAME_ID, PLAYER_ACCOUNT`. Note this should be sent in correct format.


### TABLES

#### game
```
TABLE game {
    name contract;
    name fee_account;
    string reward_per;
    uint8_t chain_length;
    uint8_t for_games;
    asset till_reward;
};
```
Scope is _self.

#### refer
```
TABLE refer {
    name account;
    vector<name> refer_chain;
    uint8_t r_games;
    asset r_reward;
};
```
Scope is your game account name.
  
