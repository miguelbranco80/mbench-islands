#!/bin/sh
NMESSAGES=200000

sizes=("4" "16" "64" "256" "1024" "4096")
for size in "${sizes[@]}"; do

	cat > settings.cfg << EOF
$size $NMESSAGES
EOF

	echo -e "Message size (bytes)\t$size"

	tests=("fifos" "mq" "pipes" "shm" "sockets")
	for test in "${tests[@]}"; do
		cd $test
		cp ../settings.cfg .
		./run.sh
		rm settings.cfg
		cd ..
	done

	echo

done

rm settings.cfg

# Note: for MQ test, might need to increase /proc/sys/fs/mqueue/msgsize_max
