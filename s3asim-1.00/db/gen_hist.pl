#!/usr/bin/perl -w 

# Generates a histogram file that s3asim can use
# Requires the use of R, a statistical program

$input_file = "";
$R_file = "tmp.input.R";
$tmp_parse = "tmp.parse";
$tmp_parse_out = "tmp.parse.out";
$breaks = 20000;

# Parse command line arguments
if ($#ARGV == -1) {
    die "Usage: ./gen_hist.pl -i input_db -o output_file -b breaks";
}

while (@ARGV) {
    my $arg = shift @ARGV;
    if ($arg eq '-h') {
	die "Usage: ./gen_hist.pl -i input_db -o output_file -b breaks";
    }
    elsif ($arg eq '-i') {
	$input_file = shift @ARGV;
	if (-e $input_file == 0) {
	    die "input file $input_file does not exists\n";
	}
    }
    elsif ($arg eq '-o') {
	$output_file = shift @ARGV;
	if (-e $output_file) {
	    die "output_file $output_file already exists\n";
	}
    }
    elsif ($arg eq '-b') {
	$breaks = shift @ARGV;
	if ($breaks <= 0 || $breaks > 50000) {
	    die "$breaks breaks is impossible\n";
	}
    }
    else {
        die "Invalid argument: $arg\n";
    }
}

if ($input_file eq "" | $output_file eq "") {
    die "input_file $input_file or output_file $output_file invalid\n";
}

# Find the lengths of the all queries
open INPUT_FH, "< $input_file" or die "Couldn't open file $input_file\n";
open OUTPUT_FH, "> $tmp_parse" or die "Couldn't open file $tmp_parse\n";
$mean = 0;
$query_count = -1;
$cur_query_len = 0;
while (<INPUT_FH>) {
    chomp $_;
    if ($_ =~ m/^>/) {
	if ($query_count == -1) {
	    $query_count = 0;
	}
	else {
	    $mean += $cur_query_len;
	    print OUTPUT_FH "$cur_query_len\n";
	    $query_count++;
	    $cur_query_len = 0;
	}
    }
    else {
	$cur_query_len += length($_);
    }
}
$mean += $cur_query_len;
print OUTPUT_FH "$cur_query_len\n";
$query_count++;

close INPUT_FH;
close OUTPUT_FH;

# Create the R file for histogram generation
open R_FH, "> $R_file" or die "Couldn't open file $R_file\n";
print R_FH "X <- scan(\"$tmp_parse\")\n";
print R_FH "sink(\"$tmp_parse_out\")\n";
print R_FH "hist(X, breaks=$breaks,plot=FALSE)\n";
close R_FH;

`R --save < $R_file`;

# Generate the correct formating
open INPUT_FH, "< $tmp_parse_out" or die "Couldn't open file $tmp_parse_out";
open OUTPUT_FH, "> $output_file" or die "Couldn't open file $output_file";
my $type = "";
my @breaks_arr = ();
my @counts_arr = ();
my $breaks_count = 0;
my $counts_count = 0;
while (<INPUT_FH>) {
    chomp $_;
    if ($_ =~ m/\$breaks/) {
	$type = "breaks";
    }
    elsif ($_ =~ m/\$counts/) {
	$type = "counts";
    }
    elsif ($_ =~ m/\$intensities/) {
        last;
    }
    elsif ($_ =~ m/\[/) {
        my @data_arr = split(/\]/, $_);
        my @actual_arr = split(/\s+/, $data_arr[1]);

        for (my $i = 0; $i <= $#actual_arr; $i++) {
            if ($actual_arr[$i] =~ m/\d+/) {
		if ($type eq "breaks") {
		    push @breaks_arr, $actual_arr[$i];
		    $breaks_count++;
		}
		elsif ($type eq "counts") {
		    push @counts_arr, $actual_arr[$i];
		    $counts_count++;
		}
		else {
		    die "Impossible type $type\n";
		}
            }
        }
    }
}

if ($breaks_count != $counts_count + 1) {
    die "breaks_count $breaks_count not equal to counts_count $counts_count\n";
}
$breaks_count--;    
print OUTPUT_FH "$breaks_count\n";
for ($i = 1; $i < $breaks_count; $i++) {
    print OUTPUT_FH "$breaks_arr[$i] $counts_arr[$i-1]\n";
}

close INPUT_FH;
close OUTPUT_FH;
