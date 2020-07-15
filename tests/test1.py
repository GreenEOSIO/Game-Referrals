import sys
from eosfactory.eosf import *
verbosity([Verbosity.INFO, Verbosity.OUT, Verbosity.DEBUG])

CONTRACT_WORKSPACE = sys.path[0] + "/../src/"
TOKEN_CONTRACT_WORKSPACE = sys.path[0] + "/../../eosio.token/"


def test():
    SCENARIO('''
    Execute all actions.
    ''')
    reset()
    create_master_account("eosio")

    #######################################################################################################
    
    COMMENT('''
    Create test accounts:
    ''')

    # test accounts and accounts where the smart contracts will be hosted
    create_account("alice", eosio, account_name="alice")
    create_account("gamereferral", eosio, account_name="gamereferral")
    create_account("eosiotoken", eosio, account_name="eosio.token")
    create_account("bob", eosio, account_name="bob")
    create_account("thirdacc", eosio, account_name="thirdacc")

    create_account("gameaccount", eosio, account_name="gameaccount")
    create_account("feeaccount", eosio, account_name="feeaccount")


    ########################################################################################################
    
    COMMENT('''
    Build and deploy token contract:
    ''')

    # creating token contract
    token_contract = Contract(eosiotoken, TOKEN_CONTRACT_WORKSPACE)
    token_contract.build(force=True)
    token_contract.deploy()

    ########################################################################################################
    
    COMMENT('''
    Build and deploy gamereferral contract:
    ''')

    # creating gamereferral contract
    contract = Contract(gamereferral, CONTRACT_WORKSPACE)
    contract.build(force=True)
    contract.deploy()

    ########################################################################################################
    
    COMMENT('''
    Create WAX tokens 
    ''')

    token_contract.push_action(
        "create",
        {
            "issuer": eosio,
            "maximum_supply": "1000000000.00000000 WAX"
        },
        [eosiotoken]
    )


    ########################################################################################################

    COMMENT('''
    Issue WAX tokens to eosio 
    ''')

    token_contract.push_action(
        "issue",
        {
            "to": eosio,
            "quantity": "10000000.00000000 WAX",
            "memo": "issued tokens to eosio"
        },
        [eosio]
    )

    
    COMMENT('''
    Transfer WAX tokens to gameaccount 
    ''')

    token_contract.push_action(
        "transfer",
        {
            "from":eosio,
            "to": gameaccount,
            "quantity": "10000.00000000 WAX",
            "memo": "issued tokens to alice"
        },
        [eosio]
    )

  
    
    ########################################################################################################
    COMMENT('''
    Config the gamereferral
    ''')

        # ACTION configuregame(name contract,  name fee_account, string reward_per, uint8_t chain_length, uint8_t for_games, asset till_reward );

    contract.push_action(
        "configuregame",
        {
            "contract": gameaccount,
            "fee_account": feeaccount,
            "reward_per":"5.00",
            "chain_length":"2",
            "for_games":"2",
            "till_reward":"100.00000000 WAX"

        },
        [gameaccount]
    )

    COMMENT('''
    refer alice by bob
    ''')
        # ACTION referred(name to, name from, name gameAcct);

    contract.push_action(
        "referred",
        {
            "to": alice,
            "from": bob,
            "gameAcct":gameaccount
        },
        [alice]
    )

    COMMENT('''
        refer thirdacc by alice
    ''')
        # ACTION referred(name to, name from, name gameAcct);

    contract.push_action(
        "referred",
        {
            "to": thirdacc,
            "from": alice,
            "gameAcct":gameaccount
        },
        [thirdacc]
    )

    # COMMENT('''
    # refer bob by alice
    # ''')
    #     # ACTION referred(name to, name from, name gameAcct);

    # contract.push_action(
    #     "referred",
    #     {
    #         "to": bob,
    #         "from": alice,
    #         "gameAcct":gameaccount
    #     },
    #     [bob]
    # )
    ########################################################################################################

   

    # ########################################################################################################
    
    # set eosio.code permission to the contract
    gamereferral.set_account_permission(
        Permission.ACTIVE,
        {
            "threshold": 1,
            "keys": [
                {
                    "key": gamereferral.active(),
                    "weight": 1
                }
            ],
            "accounts":
            [
                {
                    "permission":
                        {
                            "actor": gamereferral,
                            "permission": "eosio.code"
                        },
                    "weight": 1
                }
            ]
        },
        Permission.OWNER,
        (gamereferral, Permission.OWNER)
    )

  ##################################
    # COMMENT('''
    # Transfer from gameaccount to gamereferral (all gamefee)
    # ''')
    # token_contract.push_action(
    #     "transfer",
    #     {
    #         "from": gameaccount,
    #         "to": gamereferral,
    #         "quantity":"100.00000000 WAX",
    #         "memo":"1,bob"
    #     },
    #     [gameaccount]
    # )
    gamereferral.table("game", gamereferral)
    gamereferral.table("refer", gameaccount)

    COMMENT('''
        Transfer from gameaccount to gamereferral (all gamefee)
    ''')
    token_contract.push_action(
        "transfer",
        {
            "from": gameaccount,
            "to": gamereferral,
            "quantity":"100.00000000 WAX",
            "memo":"1,thirdacc"
        },
        [gameaccount]
    )

    # token_contract.push_action(
    #     "transfer",
    #     {
    #         "from": gameaccount,
    #         "to": gamereferral,
    #         "quantity":"100.00000000 WAX",
    #         "memo":"1,alice"
    #     },
    #     [gameaccount]
    # )

    gamereferral.table("game", gamereferral)
    gamereferral.table("refer", gameaccount)


    COMMENT('''Expected: 9900 WAX''')
    eosiotoken.table("accounts", "gameaccount")
    COMMENT('''Expected: 1 WAX''')
    eosiotoken.table("accounts", "feeaccount")

    COMMENT('''Expected: 9 WAX''')
    eosiotoken.table("accounts", "bob")

    COMMENT('''Expected: 90 WAX''')
    eosiotoken.table("accounts", "alice")


    stop()

if __name__ == "__main__":
    test()
