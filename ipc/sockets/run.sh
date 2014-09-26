numactl -C 1 ./tcp-server 8100 &
sleep 0.5
echo -e "TCP same core\t`numactl -C 1 ./tcp-client localhost 8100`"

numactl -C 1 ./tcp-server 8101 &
sleep 0.5
echo -e "TCP same socket\t`numactl -C 2 ./tcp-client localhost 8101`"

numactl -C 1 ./tcp-server 8102 &
sleep 0.5
echo -e "TCP other socket\t`numactl -C 100 ./tcp-client localhost 8102`"

rm -f socket1 socket2
numactl -C 1 ./unix-server socket1 &
sleep 0.5
echo -e "UNIX same core\t`numactl -C 1 ./unix-client socket1`"

rm -f socket1 socket2
numactl -C 1 ./unix-server socket1 &
sleep 0.5
echo -e "UNIX same socket\t`numactl -C 2 ./unix-client socket1`"

rm -f socket1 socket2
numactl -C 1 ./unix-server socket2 &
sleep 0.5
echo -e "UNIX other cores\t`numactl -C 100 ./unix-client socket2`"
