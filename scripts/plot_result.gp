reset
set xlabel 'F(n)'
set ylabel 'time (ns)'
set title 'fib seq/doublig'
set term png enhanced font 'Verdana,10'
set output 'plot_statistic.png'
set grid
plot [0:92][0:300] \
'time_result' \
using 1:2 with linespoints linewidth 2 title "fib seq",\
'' using 1:3 with linespoints linewidth 2 title "fib doubling ",\