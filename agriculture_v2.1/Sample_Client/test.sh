make clean
make
killall TestClient
rm -rf *.o *.out
nohup ./TestClient 1 2 3 4 >Test1.log &
nohup ./TestClient 5 6 >Test5.log &
nohup ./TestClient 7 8 9 >Test7.log &
nohup ./TestClient 10 11 12 13 14 15 16 17 18 19 >Test10.log & 
nohup ./TestClient 20 21 22 23 > Test20.log & 
ps -aux | grep TestClient

