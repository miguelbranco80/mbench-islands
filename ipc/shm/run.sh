numactl -C 1 ./server /shm1 &
sleep 0.5
echo -e "Shared-memory with CAS same socket\t`numactl -C 2 ./client /shm1`"

sleep 1

numactl -C 1 ./server /shm1 &
sleep 0.5
echo -e "Shared-memory with CAS other socket\t`numactl -C 100 ./client /shm1`"

# The following is very slow: at 0.02 s per message!
#numactl -C 1 ./server /shm1 &
#sleep 0.5
#echo -e "Shared-memory+CAS same core\t`numactl -C 1 ./client /shm1`"

#sleep 1

