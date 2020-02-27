#!/bin/bash

prgm="./lab4b"

echo 'Beginning smoke test on lab4b ...'
printf "SCALE=F\nLOG hello!\nOFF\n" | $prgm --log=log.txt
RC="$?"

if [[ $RC -ne 0 ]]; then
    echo "ERROR in program; expected exit code 0, got $RC" >&2
    exit 1
fi

if [[ ! -e log.txt ]]; then
    echo 'Program was unable to write log to file.' >&2
    exit 1
fi

if ! ( cat log.txt | grep -E "[0-9]{2}:[0-9]{2}:[0-9]{2} [0-9]+\.[0-9]" ); then
    echo 'Incorrect logging format to log file.' >&2
    exit 1
fi

if ! ( cat log.txt | grep -E "SCALE=F$" && cat log.txt | grep -E "LOG hello!$" ); then
    echo 'Input commands were not logged to file.' >&2
    exit 1
fi

echo "Smoke check passed!"
