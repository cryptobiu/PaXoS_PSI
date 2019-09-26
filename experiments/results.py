
# ****** ATTENTION *****
# Always run the command with -r 0 first (this is the server) because we measure the time of the client, and we don't want the client to wait for the server, this adds time to the run
# **********************

## bandwidth
#lan1: (n.virginia-n.virginia)
#[ ID] Interval       Transfer     Bandwidth
# [  3]  0.0-10.0 sec  5.79 GBytes  4.97 Gbits/sec

#wan2: (n.virginia-sydney)
# [ ID] Interval       Transfer     Bandwidth
# [  3]  0.0-10.0 sec  65.9 MBytes  55.1 Mbits/sec

#wan1 (n.virginia-oregon)
# [ ID] Interval       Transfer     Bandwidth
# [  3]  0.0-10.0 sec   185 MBytes   155 Mbits/sec


# The versions are:
results = {}
# ECDH with 2 curves versions
results['ecdh-c0'] = {}
results['ecdh-c1'] = {}
results['kkrt'] = {}
results['spot-low'] = {}
results['spot-fast'] = {}
results['rrv1'] = {}
results['rrv2'] = {}
results['paxos-ours-sh'] = {}
results['paxos-ours-mal'] = {}
results['paxos-ni-sh'] = {}
results['paxos-ni-mal'] = {}

results['sec2x'] = {}
# results['sec3x'] = {}
# results['sec4x'] = {}
results['sec5x'] = {}

for c in ['ecdh-c0', 'ecdh-c1', 'kkrt', 'spot-low', 'spot-fast', 'rrv1', 'rrv2', 'paxos-ours-sh', 'paxos-ours-mal', 'paxos-ni-sh', 'paxos-ni-mal', 'sec2x', 'sec5x']:
	results[c]['lan'] = {}
	results[c]['wan1'] = {}
	results[c]['wan2'] = {}


# ECDH
# cd ~/Ni/SpOT-PSI
# server:
# ./bin/frontend.exe -r 0 -echd -c 0 -n 12 -ip 172.31.15.222:1212
# client:
# ./bin/frontend.exe -r 1 -echd -c 0 -n 12 -ip 172.31.15.222:1212
# parameters -c {0,1} -n {12,16,20}
# *****   -c 0  ******
results['ecdh-c0']['lan'][12] = 4597.4
results['ecdh-c0']['lan'][16] = 73511.8
results['ecdh-c0']['lan'][20] = 0.0 # more than 3:47 minutes
results['ecdh-c0']['wan1'][12] = 6529.1
results['ecdh-c0']['wan1'][16] = 75839.5
results['ecdh-c0']['wan1'][20] = 0.0 # more than 3:47 minutes
results['ecdh-c0']['wan2'][12] = 36979.1
results['ecdh-c0']['wan2'][16] = 80775.974
results['ecdh-c0']['wan2'][20] = 0.0 # more than 3:47 minutes
# *****   -c 1  ******
results['ecdh-c1']['lan'][12] = 8797.2
results['ecdh-c1']['lan'][16] = 140507.2
results['ecdh-c1']['lan'][20] = 0.0 # more than 3:47 minutes
results['ecdh-c1']['wan1'][12] = 12558.8
results['ecdh-c1']['wan1'][16] = 142922.5
results['ecdh-c1']['wan1'][20] = 0.0 # more than 3:47 minutes
results['ecdh-c1']['wan2'][12] = 17641.965
results['ecdh-c1']['wan2'][16] = 150703.494
results['ecdh-c1']['wan2'][20] = 0.0 # more than 3:47 minutes


# KKRT
# cd ~/BaRK-OPRF/Release
# server:
# ./bOPRFmain.exe -r 0  -ip 0.0.0.0:1212
# client:
# ./bOPRFmain.exe -r 1  -ip 172.31.15.222:1212
# no parameters
results['kkrt']['lan'][12] = 177.0
results['kkrt']['lan'][16] = 339.0
results['kkrt']['lan'][20] = 4551
results['kkrt']['lan'][24] = 0.0  # doesn't run
results['kkrt']['wan1'][12] = 586
results['kkrt']['wan1'][16] = 1361
results['kkrt']['wan1'][20] = 9809
results['kkrt']['wan1'][24] = 0.0 # doesn't run
results['kkrt']['wan2'][12] = 932
results['kkrt']['wan2'][16] = 1983
results['kkrt']['wan2'][20] = 22887
results['kkrt']['wan2'][24] = 0.0 # doesn't run


# SpOT
# cd ~/Ni/SpOT-PSI
# server:
# ./bin/frontend.exe -r 0 -n 12 -t 1 -p 1 -ip 172.31.15.222:1212
# client:
# ./bin/frontend.exe -r 1 -n 12 -t 1 -p 1 -ip 172.31.15.222:1212
# parameters: -p {0,1}
# spot-low:  if -p 1: -n {12,16,20}
# spot-fast: if -p 0: -n {12,16,20, 24}
results['spot-low']['lan'][12] = 898.5
results['spot-low']['lan'][16] = 10173.3
results['spot-low']['lan'][20] = 0.0 # more than 200 secs
results['spot-low']['wan1'][12] = 3693.5
results['spot-low']['wan1'][16] = 18068.6
results['spot-low']['wan1'][20] = 0.0 # doesn't run
results['spot-low']['wan2'][12] = 5425.4
results['spot-low']['wan2'][16] = 22152.8
results['spot-low']['wan2'][20] = 0.0 # more than 200 secs

results['spot-fast']['lan'][12] = 460.7
results['spot-fast']['lan'][16] = 1964.6
results['spot-fast']['lan'][20] = 24442.5
results['spot-fast']['lan'][24] = 0.0 # doesn't run
results['spot-fast']['wan1'][12] = 6464.5
results['spot-fast']['wan1'][16] = 11602.9
results['spot-fast']['wan1'][20] = 31944.4
results['spot-fast']['wan1'][24] = 0.0 # doesn't run
results['spot-fast']['wan2'][12] = 9565.7
results['spot-fast']['wan2'][16] = 11088.2
results['spot-fast']['wan2'][20] = 36187.8
results['spot-fast']['wan2'][24] = 0.0 # doesn't run

# RR17b {rrv1,rrv2}
# cd ~/Ni/libPSI
# ***** NOTE ************************************
# * The IPs are hard coded in the source coded 	*
# *	 right now the server's IP encoded IPs 		*
# *	 172.31.10.226 								*
# *	 so either change it and re-compile 		*
# *	 or change only the client's location		*
#	NOTE that the roles -r 1, -r 0 might be then*
#		opposite. (client and server switch)	*
# ***********************************************
# In RR - take run times from the server side.
# server:
# ./bin/frontend.exe -rr17b -r 1 -nn 20 -s 4
# client:
# ./bin/frontend.exe -rr17b -r 0 -nn 20 -s 4
# parameters: -nn {12,16,20,24}
results['rrv1']['lan'][12] = 144.0
results['rrv1']['lan'][16] = 828.0
results['rrv1']['lan'][20] = 13996.0
results['rrv1']['lan'][24] = 0.0 # doesn't run
results['rrv1']['wan1'][12] = 1723
results['rrv1']['wan1'][16] = 5061
results['rrv1']['wan1'][20] = 69003
results['rrv1']['wan1'][24] = 0.0 # doesn't run
results['rrv1']['wan2'][12] = 4320
results['rrv1']['wan2'][16] = 12979
results['rrv1']['wan2'][20] = 161632
results['rrv1']['wan2'][24] = 0.0 # doesn't run
# server:
# ./bin/frontend.exe -rr17b-sm -r 1 -nn 16 -s 4 -b 64
# client:
# ./bin/frontend.exe -rr17b-sm -r 0 -nn 16 -s 4 -b 64
# parameters: -nn {12,16}
results['rrv2']['lan'][12] = 596.0
results['rrv2']['lan'][16] = 7330
results['rrv2']['lan'][20] = 0.0 # doesn't run
results['rrv2']['wan1'][12] = 6190
results['rrv2']['wan1'][16] = 67310
results['rrv2']['wan1'][20] = 0.0 # doesn't run
results['rrv2']['wan2'][12] = 14867
results['rrv2']['wan2'][16] = 161868
results['rrv2']['wan2'][20] = 0.0 # doesn't run


# Paxos ours semi-honest
# cd ~/PaXoS_PSI
# NOTE: always set -malicious 0 (because -malicious 1 has a bug)
# NOTE2: set the partyID as it appears in the bin/Parties.txt!
# server:
# ./bin/frontend.exe -partyID 1 -hashSize x  -fieldSize y -partiesFile bin/Parties.txt -reportStatistics 0 -internalIterationsNumber 1 -malicious 0
# client:
# ./bin/frontend.exe -partyID 0 -hashSize x  -fieldSize y -partiesFile bin/Parties.txt -reportStatistics 0 -internalIterationsNumber 1 -malicious 0
#
# parameters: when -hashSize is x[i] then -fieldSize is y[i]
# x = [ 4096, 65536, 1048576, 16777216 ], y = [ 72, 72, 84, 90 ]
#
results['paxos-ours-sh']['lan'][12] = 16
results['paxos-ours-sh']['lan'][16] = 235
results['paxos-ours-sh']['lan'][20] = 5378
results['paxos-ours-sh']['lan'][24] = 0 # doesn't run
results['paxos-ours-sh']['wan1'][12] = 641
results['paxos-ours-sh']['wan1'][16] = 1664
results['paxos-ours-sh']['wan1'][20] = 17628
results['paxos-ours-sh']['wan1'][24] = 0.0
results['paxos-ours-sh']['wan2'][12] = 1592
results['paxos-ours-sh']['wan2'][16] = 4140
results['paxos-ours-sh']['wan2'][20] = 40524
results['paxos-ours-sh']['wan2'][24] = 0.0

# Paxos ours malicious
# same commands as semi-honest, but with the following parameters
#
# parameters: when -hashSize is x[i] then -fieldSize is y[i]
# x = [ 4096, 65536, 1048576, 16777216 ], y = [ 132, 132, 132, 132 ]
#
results['paxos-ours-mal']['lan'][12] = 20
results['paxos-ours-mal']['lan'][16] = 257
results['paxos-ours-mal']['lan'][20] = 5598
results['paxos-ours-mal']['lan'][24] = 0.0 # doesn't run
results['paxos-ours-mal']['wan1'][12] = 644
results['paxos-ours-mal']['wan1'][16] = 1800
results['paxos-ours-mal']['wan1'][20] = 18621
results['paxos-ours-mal']['wan1'][24] = 0.0
results['paxos-ours-mal']['wan2'][12] = 1770
results['paxos-ours-mal']['wan2'][16] = 4546
results['paxos-ours-mal']['wan2'][20] = 40810
results['paxos-ours-mal']['wan2'][24] = 0.0



# Paxos Ni
# cd ~/Ni/dfs
# NOTE: always set -malicious 0 (because -malicious 1 has a bug)
# NOTE2: set IP accordingly
# server:
# ./bin/frontend.exe -r 0 -n 12 -s 0 -ip 172.31.22.179:1212
# client:
# ./bin/frontend.exe -r 1 -n 12 -s 0 -ip 172.31.22.179:1212
#
# parameters: -n {12,16,20,24} -s {0,1}
# -s 1 is malicious

results['paxos-ni-sh']['lan'][12] = 94.7
results['paxos-ni-sh']['lan'][16] = 838.8
results['paxos-ni-sh']['lan'][20] = 17140.3
results['paxos-ni-sh']['lan'][24] = 0.0 # doesn't run
results['paxos-ni-sh']['wan1'][12] = 790.8
results['paxos-ni-sh']['wan1'][16] = 1968.3
results['paxos-ni-sh']['wan1'][20] = 33707.1
results['paxos-ni-sh']['wan1'][24] = 0.0
results['paxos-ni-sh']['wan2'][12] = 1805.5
results['paxos-ni-sh']['wan2'][16] = 19265.4
results['paxos-ni-sh']['wan2'][20] = 50218.3
results['paxos-ni-sh']['wan2'][24] = 0.0

results['paxos-ni-mal']['lan'][12] = 98.8
results['paxos-ni-mal']['lan'][16] = 913.7
results['paxos-ni-mal']['lan'][20] = 17903.6
results['paxos-ni-mal']['lan'][24] = 0.0
results['paxos-ni-mal']['wan1'][12] = 881.4
results['paxos-ni-mal']['wan1'][16] = 2351.1
results['paxos-ni-mal']['wan1'][20] = 29078.0
results['paxos-ni-mal']['wan1'][24] = 0.0
results['paxos-ni-mal']['wan2'][12] = 2329.4
results['paxos-ni-mal']['wan2'][16] = 8660.2
results['paxos-ni-mal']['wan2'][20] = 31737.5
results['paxos-ni-mal']['wan2'][24] = 0.0





# Paxos security tradeoff - 
# ***  sec2x   ****
# cd ~/PaXoS_PSI
# NOTE: always set -malicious 0 (because -malicious 1 has a bug)
# server:
# ./bin/frontend.exe -partyID 1 -hashSize x  -fieldSize y -partiesFile bin/Parties.txt -reportStatistics 0 -internalIterationsNumber 1 -malicious 0
# client:
# ./bin/frontend.exe -partyID 0 -hashSize x  -fieldSize y -partiesFile bin/Parties.txt -reportStatistics 0 -internalIterationsNumber 1 -malicious 0
#
# parameters: when -hashSize is x[i] then -fieldSize is y[i]
# x = [ 4096, 65536, 1048576, 16777216 ], y = [ 238, 231, 217, 210 ]
#
results['sec2x']['lan'][12] = 0.0
results['sec2x']['lan'][16] = 0.0
results['sec2x']['lan'][20] = 0.0
results['sec2x']['lan'][24] = 0.0
results['sec2x']['wan1'][12] = 0.0
results['sec2x']['wan1'][16] = 0.0
results['sec2x']['wan1'][20] = 0.0
results['sec2x']['wan1'][24] = 0.0
results['sec2x']['wan2'][12] = 0.0
results['sec2x']['wan2'][16] = 0.0
results['sec2x']['wan2'][20] = 0.0
results['sec2x']['wan2'][24] = 0.0

# ***  sec5x   ****
#
# parameters: when -hashSize is x[i] then -fieldSize is y[i]
# x = [ 4096, 65536, 1048576, 16777216 ], y = [ 144, 138, 138, 132 ]
#
# this is the same as paxos-ours-mal so not need to run again
#
results['sec5x']['lan'][12] = results['paxos-ours-mal']['lan'][12]
results['sec5x']['lan'][16] = results['paxos-ours-mal']['lan'][16]
results['sec5x']['lan'][20] = results['paxos-ours-mal']['lan'][20]
results['sec5x']['lan'][24] = results['paxos-ours-mal']['lan'][24]
results['sec5x']['wan1'][12] = results['paxos-ours-mal']['wan1'][12]
results['sec5x']['wan1'][16] = results['paxos-ours-mal']['wan1'][16]
results['sec5x']['wan1'][20] = results['paxos-ours-mal']['wan1'][20]
results['sec5x']['wan1'][24] = results['paxos-ours-mal']['wan1'][24]
results['sec5x']['wan2'][12] = results['paxos-ours-mal']['wan2'][12]
results['sec5x']['wan2'][16] = results['paxos-ours-mal']['wan2'][16]
results['sec5x']['wan2'][20] = results['paxos-ours-mal']['wan2'][20]
results['sec5x']['wan2'][24] = results['paxos-ours-mal']['wan2'][24]








