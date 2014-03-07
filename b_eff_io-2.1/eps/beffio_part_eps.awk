/Summary of file I\/O bandwidth accumulated on .+ processes with .+ MByte\/PE/ {
  i++
  PEs[i] = $8
  system ("./beffio_part_eps " $8 " " substr(FILENAME, 0, length(FILENAME)-4))
}

/  set / {
  print > "ntmp_beffio_gnuloads"
}

/.* PEs      total-write .* .* .* .* .*/ {
  gsub("PEs  ", "PEs " i)
  print > "ntmp_beffio_write"
}

/.* PEs      total-rewrite .* .* .* .* .*/ {
  gsub("PEs  ", "PEs " i)
  print > "ntmp_beffio_rewrite"
}

/.* PEs      total-read .* .* .* .* .*/ {
  gsub("PEs  ", "PEs " i)
  print > "ntmp_beffio_read"
}

END {
  printf("%s", "set xtics ( ") > "ntmp_beffio_gnuloads"
  komma = 0
  for (i in PEs) {
    if (komma==1)
      printf(", ") > "ntmp_beffio_gnuloads"
    printf("\"%s\" %d", PEs[i], i) > "ntmp_beffio_gnuloads"
    komma=1
  }
  printf("%s\t\n", " )") > "ntmp_beffio_gnuloads"
}
