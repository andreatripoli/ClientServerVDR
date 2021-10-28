#!/bin/bash

gcc /mds/mds.c $(mysql_config --libs) -o /mds/mds $(mysql_config --cflags) -pthread -lbinn -L/usr/local/opt/openssl/lib
gcc /vdr/vdr.c $(mysql_config --libs) -o /vdr/vdr $(mysql_config --cflags) -pthread -lbinn -L/usr/local/opt/openssl/lib
gcc /client/client.c -o /client/client -pthread -lcrypto -lbinn -lasound 


xterm -T "VDR" -hold -geometry 85x30-700+700 -e "cd vdr/; ./vdr; bash" &

sleep 1.5

xterm -T "MDS" -hold -geometry 85x30-1000+300 -e "cd mds/; ./mds; bash" &

sleep 1.5

xterm -T "Client" -hold -geometry 85x30-350+300 -e "cd client/; ./client; bash" &

