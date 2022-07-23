#!/bin/bash
STATUS=0

# Start the server
docker-compose up -d
sleep 10

# Run register_send
pushd "$(dirname "$(readlink -f "$0")")"
if [[ $STATUS == 0 ]]; then
    ./register_send.py
    STATUS=$?
    echo "Called register_send"
fi

# Run bad_login_send
if [[ $STATUS == 0 ]]; then
    ./bad_login_send.py
    STATUS=$?
    echo "Called bad_login_send"
fi


# Run good_login_send
if [[ $STATUS == 0 ]]; then
    ./good_login_send.py
    STATUS=$?
    echo "Called good_login_send"
fi

# Run game_invite_send
if [[ $STATUS == 0 ]]; then
    ./game_invite_send.py
    STATUS=$?
    echo "Called game_invite_send"
fi
popd



# Test basic loop functions

# Test a bunch of different timers (Note to self, check out time_heap_test.c and see if it works)

# Test sending each type of message

# Test getting every type of error that we can intentionally


# Take the server down
docker-compose down

# Report status
if [[ $STATUS != 0 ]]; then
    echo "Testing failed"
fi
exit $STATUS
