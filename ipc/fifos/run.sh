numactl -C 1 ./server fifo &
sleep 0.5
echo -e "FIFO same core\t`numactl -C 1 ./client fifo`"

sleep 1

numactl -C 1 ./server fifo &
sleep 0.5
echo -e "FIFO same socket\t`numactl -C 2 ./client fifo`"

sleep 1

numactl -C 1 ./server fifo &
sleep 0.5
echo -e "FIFO other socket\t`numactl -C 100 ./client fifo`"

