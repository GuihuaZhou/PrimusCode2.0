import os


tcp_send_summary_file = open ("/home/guolab/LFS/NSDI/switch-TCP-time-summury/tcp-send.txt", "w+"); 
tcp_recv_summary_file = open ("/home/guolab/LFS/NSDI/switch-TCP-time-summury/tcp-recv.txt", "w+"); 

for ith_dir in range (1, 51):
	tcp_send_time_dir_path = "/home/guolab/LFS/NSDI/TCP-send/switch_result_TCP_send_elapsed_time_num-" + str (ith_dir * 200) + "/";
	tcp_recv_time_dir_path = "/home/guolab/LFS/NSDI/TCP-recv/switch_result_TCP_recv_elapsed_time_num-" + str (ith_dir * 200) + "/";
	print ith_dir
	send_time_sum = 0.0
	recv_time_sum = 0.0
	for ith_switch in range (0, ith_dir * 200):
		switch_file_send = open (tcp_send_time_dir_path + "switch-" + str (ith_switch) + ".txt", "r")
		switch_file_recv = open (tcp_recv_time_dir_path + "switch-" + str (ith_switch) + ".txt", "r")

		time_send = switch_file_send.readline ()
		time_recv = switch_file_recv.readline ()

		send_time_sum = send_time_sum + float (time_send)
		recv_time_sum = recv_time_sum + float (time_recv)

	tcp_send_summary_file.write (str (send_time_sum) + "\n")
	tcp_recv_summary_file.write (str (recv_time_sum) + "\n")
	tcp_send_summary_file.flush ()
	tcp_recv_summary_file.flush ()


