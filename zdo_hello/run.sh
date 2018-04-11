#!/bin/sh

wait_for_start() {
    nm=$1
    s=''
    while [ A"$s" = A ]
    do
        sleep 1
        if [ -f core ]
        then
            echo 'Somebody has crashed (ns?)'
            killch
            exit 1
        fi
        s=`grep Device zdo_${nm}*.log`
    done
    if echo $s | grep OK
    then
        return
    else
        echo $s
        killch
        exit 1
    fi
}

killch() {
    kill $endPID $coordPID $PipePID
}

killch_ex() {
    killch
    echo Interrupted by user!
    exit 1
}

trap killch_ex TERM INT


rm -f *.log *.pcap *.dump core

echo "run ns"

../../devtools/network_simulator/network_simulator --nNode=2 --pipeName=/tmp/zt >ns.txt 2>&1 &
PipePID=$!
sleep 1

echo "run coordinator"
./zdo_start_zc /tmp/zt0.write /tmp/zt0.read &
coordPID=$!
wait_for_start zc

echo ZC STARTED OK

echo "run ze"
./zdo_start_ze /tmp/zt1.write /tmp/zt1.read &
endPID=$!
wait_for_start ze

echo ZE STARTED OK

sleep 20

if [ -f core ]
then
    echo 'Somebody has crashed'
fi

echo 'shutdown...'
killch


set - `ls *dump`
../../devtools/dump_converter/dump_converter -ns $1 zc.pcap
../../devtools/dump_converter/dump_converter -ns $2 ze.pcap

echo 'Now verify traffic dump, please!'

