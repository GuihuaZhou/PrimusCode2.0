Currently, thorough performance tests have been done on 9.1(master) and 10.1(switch). Both machine turn off hyper-threading, and use "powersave" CPUfreq governer.
Master use 10 send thread and 2 recv thread, all bind to a different CPU core

All the time below is the average per link-state. We don't count failure detection time in all the experiments (cause we cannot).
NOTE: We have compared the detection time for NIC change between epoll (used by us) and busy poll loop, in a dedicated test. Results show that epoll detect NIC change about 250us faster even than busy while(1) loop to poll interface status using getifaddrs(). Please use script "rtnetlinkTest.sh" to conduct this test. Example usage: "sudo ./rtnetlinkTest.sh lo 100 1"


All the following tests are conducted using script "SwitchMasterTest.sh". Example usage: "bash SwitchMasterTest.sh 5000 10000 10 2 1 0"
Note: Some tests below need to change the source code of master and switch. For example, we need to add usleep in "generateAFakeLSReport()" to insert 2s LS generation interval. Current source code version is the ready version for run in normal status, without incurring any test delay.

*******
In overall, from the test results, we expect less than 16.7ms*2 round-trip delay for completely finish processing a single LS, under the worst-case, in a 10K switch network. (If we have enough switch machines to have one switch client running on one machine, this number is expected to be very lower, but we cannot test it). Please read the details below carefully.
*******

Overall issues:   
    1. If change NIC too quickly, the delay for each LS processing soars-up (the whole processing for each LS takes ~110ms), because each switch only has one thread and will continuously process received NIC events and cannot deal with other processing. Specifically, netlink cannot change route (cannot recv kernel ACK) due to continuously receiving excessive NIC change events in kernel.
    2. NIC change mode sometimes will miss some NIC change events, so in the script we change NIC 10 more times to ensure master and switch exit gracefully.
    3. Path table update for case 3 need to iterate and modify most path entries, which becomes one of the most performance impactor.
    4. When too many switch processes run on a single machine, all cache miss becomes inevitable and performance drops due to process contention.

For each worst-case LS 
(LS hits case 3 in path table, iterate and change about 160K path table entries and change 1 kernel route)
	When 1 single switch process runs on a single machine
	    1. When continuously generate LS as fast as can, but with only 1 outbounding (cache all hit):
			1) Path table update takes about 620us;
			2) Kernel route change takes about 20us;
			3) LS delivery at master in TCP send takes about 3us per switch;
			4) LS response at master in TCP recv takes about 7us per switch;
			5) The whole LS report/deliver/response/ACK takes about 750us;
			6) For NIC change case (cannot ensure only 1 outbounding LS): 1) 1.5ms, 2) 55us, 3) 5.2us, 4) 6.8us, 5) 1.7ms
	    2. When wait 2s for each LS generation (cache all miss):
			1) Path table update takes about 1.5ms;
			2) Kernel route change takes about 50us;
			3) LS delivery at master in TCP send takes about 15us per switch;
			4) LS response at master in TCP recv takes about 11us per switch;
			5) The whole LS report/deliver/response/ACK takes about 1.7ms;
			6) For NIC change case: 1) 1ms, 2) 40us, 3) 11us, 4) 7us, 5) 1.2ms 
	When 5k switch processes run on a single machine
	    1. When continuously generate LS as fast as can, but with only 1 outbounding (cache still all miss due to process switch):
			1) Path table update takes about 2.9ms;
			2) Kernel route change takes about 700us;
			3) LS delivery at master in TCP send takes about 4us per switch (20ms in total, takes ~2ms for 10 send threads);
			4) LS response at master in TCP recv takes about 1.7us per switch (8.5ms in total, takes ~4.3ms for 2 recv threads);
			5) The whole LS report/deliver/response/ACK takes about 12ms;
			6) For NIC change case (cannot ensure only 1 outbounding LS): 1) 100ms, 2) 100ms, 3) 4.2us, 4) 4us, 5) 123ms
	    2. When wait 2s for each LS generation (cache all miss):
			1) Path table update takes about 2.6ms;
			2) Kernel route change takes about 550us;
			3) LS delivery at master in TCP send takes about 4.9us per switch (25ms in total, takes ~2.5ms for 10 send threads);
			4) LS response at master in TCP recv takes about 2.9us per switch (15ms in total, takes ~7.3ms for 2 recv threads);
			5) The whole LS report/deliver/response/ACK takes about 16.7ms;
			6) For NIC change case: 1) 2.5ms, 2) 370us, 3) 4.9us, 4) 3us, 5) 16ms
 
For each randomly generated LS
(can be both case 1/2/3/4 in path table)
	When 1 single switch process runs on a single machine
	    1. When continuously generate LS as fast as can, but with only 1 outbounding (cache all hit):
			1) Path table update takes about 4us;
			2) Kernel route change takes about 2us;
			3) LS delivery at master in TCP send takes about 3us per switch;
			4) LS response at master in TCP recv takes about 2.3us per switch;
			5) The whole LS report/deliver/response/ACK takes about 50us;
			6) For NIC change case (cannot ensure only 1 outbounding LS): 1) 16us, 2) 10us, 3) 5.2us, 4) 6.9us, 5) 170us
	    2. When wait 2s for each LS generation (cache all miss):
			1) Path table update takes about 23us;
			2) Kernel route change takes about 19us;
			3) LS delivery at master in TCP send takes about 15.8us per switch;
			4) LS response at master in TCP recv takes about 9us per switch;
			5) The whole LS report/deliver/response/ACK takes about 240us;
			6) For NIC change case: 1) 21us, 2) 18us, 3) 14.3us, 4) 7.5us, 5) 198us
	When 5k switch processes run on a single machine
	    1. When continuously generate LS as fast as can, but with only 1 outbounding (cache still all miss due to process switch):
			1) Path table update takes about 320us;
			2) Kernel route change takes about 240us;
			3) LS delivery at master in TCP send takes about 4us per switch (20ms in total, takes ~2ms for 10 send threads);
			4) LS response at master in TCP recv takes about 1.7us per switch (8.5ms in total, takes ~4.3ms for 2 recv threads);
			5) The whole LS report/deliver/response/ACK takes about 10.5ms;
			6) For NIC change case (cannot ensure only 1 outbounding LS): 1) 7.3ms, 2) 7.3ms, 3) 4.2us, 4) 4.1us, 5) 44ms
	    2. When wait 2s for each LS generation (cache all miss):
			1) Path table update takes about 549us;
			2) Kernel route change takes about 515us;
			3) LS delivery at master in TCP send takes about 5.1us per switch (25ms in total, takes ~2.5ms for 10 send threads);
			4) LS response at master in TCP recv takes about 2.9us per switch (15ms in total, takes ~7.3ms for 2 recv threads);
			5) The whole LS report/deliver/response/ACK takes about 15.5ms;
			6) For NIC change case: 1) 168us, 2) 130us, 3) 4.8us, 4) 3us, 5) 15.8ms