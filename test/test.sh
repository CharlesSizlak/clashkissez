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
popd

# Take the server down
docker-compose down

# Report status
if [[ $STATUS != 0 ]]; then
    echo "Testing failed"
fi
exit $STATUS
