# This is a gnuplot file
# It creates a picture of the current size of the CVS repository in
# gen/plot.pbm.
set xlabel "Date"
set ylabel "Size in MB" -1,0
set title "Size of AROS Sources"
set xdata time
set format x "%d.%m"
set format y "%6.2f"
set timefmt "%d.%m.%Y"
set grid
set terminal pbm small color
set size 0.5,0.5
set output "gen/plot.pbm"
set xrange ["01.04.1999":]
set xtics nomirror "01.03.1999", 1209600
#set ytics nomirror 28.0, 1.0
set nomxtics
plot '../../../CVSROOT/aros.size' using 1:($2/1024) title "" with lines
