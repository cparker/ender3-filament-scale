#!/bin/bash

until /home/pi/.nvm/versions/node/v12.16.1/bin/node ./server.js; do
    echo "Server 'ender-filament-scale server.js' crashed with exit code $?.  Respawning.." >&2
    sleep 1
done
