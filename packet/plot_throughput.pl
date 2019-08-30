#!/usr/bin/perl -w

# my $intialTime=140405.503983;
my $argc = @ARGV;
if($argc!=2)
{
	print STDOUT "usage: fileName timeBin(s)\n";
	exit(1);
}

$fileName=$ARGV[0];
$timeBin=$ARGV[1];
# @legendName=split(/\./, $fileName);;

open(fTraffic,$fileName);
open(fTmp,">$fileName.tr");
$binBytes=0;
my $counter=0;
while($line=<fTraffic>)
{
	$counter++;
	chomp $line;
	@trafficInfo=split(" ", $line);
	$arriveTime=$trafficInfo[1];
	$dataBytes=$trafficInfo[0];
	if($counter==1)
	{
		$currentTime=$arriveTime;
	}
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

# my $PLOT;
# open ($PLOT, "|gnuplot") or die "error: gnuplot not found!";

# #mytics
# #set mytics 5
# print $PLOT <<EOPLOT;

# set terminal postscript color
# set output "$fileName.ps"
# set grid ytics 
# set xlabel "Time (us)"
# set ylabel "Goodput (B/s)"
# plot "$fileName.tr" using 1:2 with line linewidth 3 title "$legendName[0]"
# EOPLOT

# close $PLOT;