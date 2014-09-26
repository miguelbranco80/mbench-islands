numactl -C 1 ./server /queue &
sleep 1
echo -e "POSIX MQ same core\t`numactl -C 1 ./client /queue`"

sleep 2

numactl -C 1 ./server /queue &
sleep 1
echo -e "POSIX MQ same socket\t`numactl -C 2 ./client /queue`"

sleep 2

numactl -C 1 ./server /queue &
sleep 1
echo -e "POSIX MQ other socket\t`numactl -C 100 ./client /queue`"

