set terminal png size 640,480
set output "cwnd.png"
plot "cwnd.dat" using 1:2 title 'Congestion Window,no-WS & cwnd=4' with linespoints
exit
