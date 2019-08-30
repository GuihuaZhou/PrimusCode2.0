#!/usr/bin/perl -w

my $argc = @ARGV;
if($argc!=2)
{
	print STDOUT "usage: fileName timeBin(us)\n";
	exit(1);
}

$fileName=$ARGV[0];
$timeBin=$ARGV[1];
@legendName=split(/\./, $fileName);;

open(fTraffic,$fileName);
open(fTmp,">$fileName.tr");
$binBytes=0;
$currentTime=$timeBin;
while($line=<fTraffic>)
{
	chomp $line;
	@trafficInfo=split(" ", $line);
	$arriveTime=$trafficInfo[0];
	$dataBytes=$trafficInfo[1];
	while($currentTime<=$arriveTime)
	{
		$currentThroughput=$binBytes/$timeBin;
		printf fTmp "$currentTime $currentThroughput\n";
		$currentTime=$currentTime+$timeBin;
		$binBytes=0;
	}
	$binBytes=$binBytes+$dataBytes;
}
close($fileName);

my $PLOT;
open ($PLOT, "|gnuplot") or die "error: gnuplot not found!";

#mytics
#set mytics 5
print $PLOT <<EOPLOT;

set terminal postscript color
set output "$fileName.ps"
set grid ytics 
set xlabel "Time (us)"
set ylabel "Goodput (MB/s)"
plot "$fileName.tr" using 1:2 with line linewidth 3 title "$legendName[0]"
EOPLOT

close $PLOT;