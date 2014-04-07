+	Title:  Profiling `HPIO` benchmark  
+	Author: Antonis Kougkas  
+	Date:   April 6th, 2014  
+	
+	# Official `HPIO` website
+	
+	Website: http://users.eecs.northwestern.edu/~aching/research_webpage/hpio.html
+	
+	# Building `HPIO`
+	
+	    $ make -f Makefile.linux
+	    
+	Check the Makefile to make sure you use the right `mpicc`.
+	
+	# Building `HPIO` with IOSIG
+	
+	    $ make -f Makefile.iosig
+	
+	Check `Makefile.iosig` for more details on enabling IOSIG.
+	
+	#List of options to use
+	
+	    $ mpirun -np 1 ./hpio -h
+	
+	Read README for more detailed test parameters
+	
+	# Running the benchmark
+	
+	    $ mpirun -np <number of processes> ./hpio-small -n 1111 -m 11 -b 111
 

