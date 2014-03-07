#
# Synopsis: gnuplot beffio_n_eps_rewrt.gnuplot
#
# Input:  ntmp_beffio_write, ntmp_beffio_rewrite, ntmp_beffio_read,
#	  ntmp_beffio_gnuloads
# Output: beffio_write_mono.eps
#         beffio_write_color.eps
#         beffio_write_color.png
#         beffio_rewrt_mono.eps
#         beffio_rewrt_color.eps
#         beffio_rewrt_color.png
#         beffio_read_mono.eps
#         beffio_read_color.eps
#         beffio_read_color.png

set label 5 "" at 10,0.50 right
set label 6 "" at 10,0.25 right
load "ntmp_beffio_gnuloads"

set size 0.73,1.00
set nologscale x
set xrange[0.5:10.5]
set xlabel "number of PEs"
set noxzeroaxis

set logscale y 10
set yrange[0.1:100000]
set ytics
set noyzeroaxis

set key 5.3,50000

set term postscript eps monochrome "Times-Roman" 22
set output "ntmp_beffio_write_mono.eps"
set ylabel "bandwidth  -  first write  [MB/s]"
plot '< cat ntmp_beffio_write' using 3:5 title "scatter - type 0" with linespoints 1 1,\
     '< cat ntmp_beffio_write' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_write' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_write' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_write' using 3:9 title "seg-coll - type 4" with linespoints 5 5
set output "ntmp_beffio_rewrt_mono.eps"
set ylabel "bandwidth  -  rewrite  [MB/s]"
plot '< cat ntmp_beffio_rewrite' using 3:5 title "scatter - type 0" with linespoints 1 1
set output "ntmp_beffio_read_mono.eps"
set ylabel "bandwidth  -  read  [MB/s]"
plot '< cat ntmp_beffio_read' using 3:5 title "scatter - type 0" with linespoints 1 1,\
     '< cat ntmp_beffio_read' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_read' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_read' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_read' using 3:9 title "seg-coll - type 4" with linespoints 5 5


set term postscript eps color solid "Helvetica" 20
load "ntmp_beffio_gnuloads"
set output "ntmp_beffio_write_color.eps"
set ylabel "bandwidth  -  first write  [MB/s]"
plot '< cat ntmp_beffio_write' using 3:5 title "scatter - type 0" with linespoints 1 1,\
     '< cat ntmp_beffio_write' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_write' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_write' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_write' using 3:9 title "seg-coll - type 4" with linespoints 5 5
set output "ntmp_beffio_rewrt_color.eps"
set ylabel "bandwidth  -  rewrite  [MB/s]"
plot '< cat ntmp_beffio_rewrite' using 3:5 title "scatter - type 0" with linespoints 1 1
set output "ntmp_beffio_read_color.eps"
set ylabel "bandwidth  -  read  [MB/s]"
plot '< cat ntmp_beffio_read' using 3:5 title "scatter - type 0" with linespoints 1 1,\
     '< cat ntmp_beffio_read' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_read' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_read' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_read' using 3:9 title "seg-coll - type 4" with linespoints 5 5


set term png small color
load "ntmp_beffio_gnuloads"
set output "ntmp_beffio_write_color.png"
set ylabel "bandwidth  -  first write  [MB/s]" 
plot '< cat ntmp_beffio_write' using 3:5 title "scatter - type 0" with linespoints lt 1 lw 200 pt 1,\
     '< cat ntmp_beffio_write' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_write' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_write' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_write' using 3:9 title "seg-coll - type 4" with linespoints 5 5
set output "ntmp_beffio_rewrt_color.png"
set ylabel "bandwidth  -  rewrite  [MB/s]"
plot '< cat ntmp_beffio_rewrite' using 3:5 title "scatter - type 0" with linespoints 1 1
set output "ntmp_beffio_read_color.png"
set ylabel "bandwidth  -  read  [MB/s]"
plot '< cat ntmp_beffio_read' using 3:5 title "scatter - type 0" with linespoints 1 1,\
     '< cat ntmp_beffio_read' using 3:6 title "shared - type 1" with linespoints 2 2,\
     '< cat ntmp_beffio_read' using 3:7 title "separate - type 2" with linespoints 3 3,\
     '< cat ntmp_beffio_read' using 3:8 title "segment - type 3" with linespoints 4 4,\
     '< cat ntmp_beffio_read' using 3:9 title "seg-coll - type 4" with linespoints 5 5
