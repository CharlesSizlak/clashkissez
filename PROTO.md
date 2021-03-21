info we need to handle

client -> server
- login information
    - username/password combo
        - hash password using ??????
        - only the hash goes over the wire
        - username is ascii
        - password comes as a fixed number of bytes
client <- server
- pass or fail
    - if pass, get a session token too
    - pass/fail response comes as status boolean
    - if pass, further info of token comes as a fixed byte length

- registration info
    - username/hashed password combo to add to a database automagically
    - basically the same conversation as above but with a further check with our database to see if we're allowing what they send us

things to do once you've got a session token
client -> server
- pull online or offline player usernames
    - get usernames message is sent to server with some flags
        - incl online/offline
        - elo range
        - friends
        - in game/out of game
client <- server
- big ass mess of bytes of information
    - each players cache of information is a fixed length
    - number of players that matched
    - elo comes in as an int
    - all other info comes as a boolean

invite to game
client -> server
- invite a player to a game
    - send the server a message containing a username and game settings
        - username is plain ol' ascii
        - time controls includes four ints, starting time for both players and their increment
        - enum of whether the inviting player is white/black/randomly generated
client <- server
- pass/fail
    - fail if the player(s) are offline or in game, success if the players are on and the rules are legal
other guy <- server
- invite
    - info containing all the above game rules
    - ascii of the inviting players name
    - invite token
other guy -> server
- accept/refusal
    - bool
    - the invite token

handle game
both guys <- server
- game token
    - hand both players a game token
    - send both players the full game information
both guys -> server
- moves
    - player sends the server two bytes, origin square and destination square of a piece
the moving guy <- server
- pass/fail
    - if it's a legal move, the server moves to the next step
    - if it's an illegal move, the server sends back a fail bool
both guys assuming pass <- server
- update of the game state
    - two ints for performing the move clientside on their programs
    - two floats with remaining time for both players
client -> server
- check last move
    - just a single field asking nicely for the last move
client <- server
- the same message as updating the game state, but only to this guy with updated time
client -> server
- get entire board
    - single field asking nicely for the full game state, history, and time
client <- server
- game history and time
    - couple floats containing times for players
    - int containing number of moves that have occurred in the canonical game state
    - a series of ints in pairs that have origin/dest squares for the client to reconstruct the game
client <- server
- heartbeat
    - happens pretty often
    - contains two floats with time
client -> server
- heartbeat response
    - really just a blank packet to let the server know the client is still connected
client -> server
- draw offer
    - really just the info that this is a draw offer
other client <- server
- draw info
    - informs the other client that the other player has offered a draw. again, really just the type of message
other client -> server
- draw accept/decline
    - pretty much just a bool
client <- server
- draw decline
    - pretty much the above
both clients <- server
- draw accept
    - really just politely informing the client that the connections are all closing
client -> server
- resign
    - really just the info that this is a resign
both clients <- server
- resign confirmation
    - really just politely informing the client that the connections are all closing
both clients <- server
- checkmate
    - politely informs both clients that connections are closing
both clients <- server
- flag
    - two floats containing both players times
    - politely informs both clients that connections are closing



add friend
client -> server
- friendo requesto
    - contains ascii of the friend you want to add
client <- server
- pass/fail
    - just a bool

check friend requests
client -> server
- hey do I have any friendos
client <- server
- pending friend requests
    - contains an int with number of incoming requests
    - each individual request contains
        - ascii of name
        - on/offline status
        - in/not in game status
        - elo
client -> server
- accept friend quests
    - ascii of the newly added players name


