# 
# Synopsis: gnuplot beffio_part_eps_rewrt.gnuplot 
# 
#--  # Input:  file     b_eff_io.sum
#--  #         argument " 4 proc"
#--  #
#--  ! rm -f xyztmp_beffio_gnuloads xyztmp_beffio_prot
#--  ! sed -n -e '/Sum.* 4 proc/,/these.* 4 proc/p' b_eff_io.sum | sed -e 's/PEs 7/PEs 10/' -e 's/PEs 6/PEs 8/' -e 's/PEs 5/PEs 7/' -e 's/PEs 4/PEs 5/' -e 's/PEs 3/PEs 4/' > xyztmp_beffio_prot 
#--  ! grep "  set " xyztmp_beffio_prot > xyztmp_beffio_gnuloads
# 
# Input:  xyztmp_beffio_prot, xyztmp_beffio_gnuloads
# Output: xyztmp_beffio_write_mono.eps
#         xyztmp_beffio_write_color.eps
#         xyztmp_beffio_write_color.png
#         xyztmp_beffio_rewrt_mono.eps
#         xyztmp_beffio_rewrt_color.eps
#         xyztmp_beffio_rewrt_color.png
#         xyztmp_beffio_read_mono.eps
#         xyztmp_beffio_read_color.eps
#         xyztmp_beffio_read_color.png
#         xyztmp_beffio_type0_sca_mono.eps
#         xyztmp_beffio_type0_sca_color.eps
#         xyztmp_beffio_type0_sca_color.png
#         xyztmp_beffio_type1_sha_mono.eps
#         xyztmp_beffio_type1_sha_color.eps
#         xyztmp_beffio_type1_sha_color.png
#         xyztmp_beffio_type2_sep_mono.eps
#         xyztmp_beffio_type2_sep_color.eps
#         xyztmp_beffio_type2_sep_color.png
#         xyztmp_beffio_type3_seg_mono.eps
#         xyztmp_beffio_type3_seg_color.eps
#         xyztmp_beffio_type3_seg_color.png
#         xyztmp_beffio_type4_sgc_mono.eps
#         xyztmp_beffio_type4_sgc_color.eps
#         xyztmp_beffio_type4_sgc_color.png
# 
# default:
set label 5 "" at 10,0.50 right
set label 6 "" at 10,0.25 right
#  
load "xyztmp_beffio_gnuloads" 
# reload for colored pictures to restore label 5 and 6 
# 
# set title "my title" 
# set label 1 "hww T3E900-512" at 10,50000 right 
# set xtics ( "1k" 1, "+8" 2, "32k" 4, "+8" 5, "1M" 7, "+8" 8, "M_PART" 10) 
# 
set size 0.73,1.00 
set nologscale x 
set xrange[0.5:10.5]
set xlabel "contiguous chunks on disk  [bytes]" 
set noxzeroaxis
# 
set logscale y 10
set yrange[0.1:100000] 
set ytics 
set noyzeroaxis
#
set key 5.3,50000 
# 
#
# ======================================================================
# 
set term postscript eps monochrome  "Times-Roman" 22
#
# ----------------------------------------------------------------------
#
# 
set output "xyztmp_beffio_write_mono.eps"
set ylabel "bandwidth  -  first write  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " write  " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " write  " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " write  " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " write  " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_rewrt_mono.eps"
set ylabel "bandwidth  -  rewrite  [MB/s]" 
plot '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_read_mono.eps"
set ylabel "bandwidth  -  read  [MB/s]" 
plot '< grep " read   " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_type1_sha_mono.eps"
set ylabel "bandwidth  -  shared/type 1  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:7  title   "write - shared" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "rewrite - shared" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title    "read - shared" with linespoints 3 3
#
set label 5 "" at 10,0.50 right
set label 6 "" at 10,0.25 right
#
set output "xyztmp_beffio_type0_sca_mono.eps"
set ylabel "bandwidth  -  scatter/type 0  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title   "write - scatter" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "rewrite - scatter" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:6  title    "read - scatter" with linespoints 3 3
#
set output "xyztmp_beffio_type2_sep_mono.eps"
set ylabel "bandwidth  -  separate/type 2  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:8  title   "write - separate" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "rewrite - separate" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title    "read - separate" with linespoints 3 3
#
set output "xyztmp_beffio_type3_seg_mono.eps"
set ylabel "bandwidth  -  segmented/type 3  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:9  title   "write - segment" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "rewrite - segment" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title    "read - segment" with linespoints 3 3
#
set output "xyztmp_beffio_type4_sgc_mono.eps"
set ylabel "bandwidth  -  seg-coll/type 4  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:10 title   "write - seg-coll" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "rewrite - seg-coll" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title    "read - seg-coll" with linespoints 3 3
# 
#
# ======================================================================
# 
set term postscript eps color solid "Helvetica" 20
# 
# ----------------------------------------------------------------------
# 
# reload for colored pictures to restore label 5 and 6 
load "xyztmp_beffio_gnuloads" 
# 
set output "xyztmp_beffio_write_color.eps"
set ylabel "bandwidth  -  first write  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " write  " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " write  " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " write  " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " write  " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_rewrt_color.eps"
set ylabel "bandwidth  -  rewrite  [MB/s]" 
plot '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_read_color.eps"
set ylabel "bandwidth  -  read  [MB/s]" 
plot '< grep " read   " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_type1_sha_color.eps"
set ylabel "bandwidth  -  shared/type 1  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:7  title   "write - shared" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "rewrite - shared" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title    "read - shared" with linespoints 3 3
#
set label 5 "" at 10,0.50 right
set label 6 "" at 10,0.25 right
#
set output "xyztmp_beffio_type0_sca_color.eps"
set ylabel "bandwidth  -  scatter/type 0  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title   "write - scatter" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "rewrite - scatter" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:6  title    "read - scatter" with linespoints 3 3
#
set output "xyztmp_beffio_type2_sep_color.eps"
set ylabel "bandwidth  -  separate/type 2  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:8  title   "write - separate" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "rewrite - separate" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title    "read - separate" with linespoints 3 3
#
set output "xyztmp_beffio_type3_seg_color.eps"
set ylabel "bandwidth  -  segmented/type 3  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:9  title   "write - segment" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "rewrite - segment" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title    "read - segment" with linespoints 3 3
#
set output "xyztmp_beffio_type4_sgc_color.eps"
set ylabel "bandwidth  -  seg-coll/type 4  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:10 title   "write - seg-coll" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "rewrite - seg-coll" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title    "read - seg-coll" with linespoints 3 3
# 
# ======================================================================
# 
#set terminal png medium size 640,480 \
#                  xffffff x000000 x404040 \
#                  xff0000 xffa500 x66cdaa xcdb5cd \
#                  xadd8e6 x0000ff xdda0dd x9500d3    # defaults 
#set terminal png medium size 540,440 \
#                  xffffff x000000 x404040 \
#                  xff0000 xffa500 x0000ff x00ffff \
#                  xcc0094 x946600 xff6600 x009999    # defaults 
#set size 540,440
set terminal png small color
#		  xffffff x000000 x404040 \
#		  x00ee00 x0000ff xff0000 xcc0094 \
#		  x00eeee xffa500 xff6600 x946600    # defaults 
# 
# ----------------------------------------------------------------------
# 
# reload for colored pictures to restore label 5 and 6 
load "xyztmp_beffio_gnuloads" 
# 
set output "xyztmp_beffio_write_color.png"
set ylabel "bandwidth  -  first write  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints lt 1 lw 200 pt 1,\
     '< grep " write  " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " write  " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " write  " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " write  " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_rewrt_color.png"
set ylabel "bandwidth  -  rewrite  [MB/s]" 
plot '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_read_color.png"
set ylabel "bandwidth  -  read  [MB/s]" 
plot '< grep " read   " xyztmp_beffio_prot' using 3:6  title "scatter - type 0" with linespoints 1 1,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title "shared - type 1" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title "separate - type 2" with linespoints 3 3,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title "segment - type 3" with linespoints 4 4,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title "seg-coll - type 4" with linespoints 5 5
#
set output "xyztmp_beffio_type1_sha_color.png"
set ylabel "bandwidth  -  shared/type 1  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:7  title   "write - shared" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:7  title "rewrite - shared" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:7  title    "read - shared" with linespoints 3 3
#
set label 5 "" at 10,0.50 right
set label 6 "" at 10,0.25 right
#
set output "xyztmp_beffio_type0_sca_color.png"
set ylabel "bandwidth  -  scatter/type 0  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:6  title   "write - scatter" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:6  title "rewrite - scatter" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:6  title    "read - scatter" with linespoints 3 3
#
set output "xyztmp_beffio_type2_sep_color.png"
set ylabel "bandwidth  -  separate/type 2  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:8  title   "write - separate" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:8  title "rewrite - separate" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:8  title    "read - separate" with linespoints 3 3
#
set output "xyztmp_beffio_type3_seg_color.png"
set ylabel "bandwidth  -  segmented/type 3  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:9  title   "write - segment" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:9  title "rewrite - segment" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:9  title    "read - segment" with linespoints 3 3
#
set output "xyztmp_beffio_type4_sgc_color.png"
set ylabel "bandwidth  -  seg-coll/type 4  [MB/s]" 
plot '< grep " write  " xyztmp_beffio_prot' using 3:10 title   "write - seg-coll" with linespoints 1 1,\
     '< grep " rewrite" xyztmp_beffio_prot' using 3:10 title "rewrite - seg-coll" with linespoints 2 2,\
     '< grep " read   " xyztmp_beffio_prot' using 3:10 title    "read - seg-coll" with linespoints 3 3
# 
# ======================================================================
# 
#--  ! rm -f xyztmp_beffio_gnuloads xyztmp_beffio_prot
