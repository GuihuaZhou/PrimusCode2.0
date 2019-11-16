// ############# Master connects with a node by tcp #############
// // Master
// {
// 	if the destNode does not exist in the directNodeTable then 
// 		updateDirectNodeTable(add,destNode,nextHop is the destNode);
// 		for i 0 to AllMaster.size() by 1 do // send the node's information to other Masters
// 			send(AllMaster[i]);
// 		end
// 		for i 0 to AllDirectNode.size() by 1 do // send the node's information to all directly controlled nodes
// 			send(AllDirectNode[i]);
// 		end
// 	else
// 		updateDirectNodeTable(modify,destNode,nextHop is the destNode);
// }
// // other Masters
// {
// 	receiveNodeFromMaster();// receive a node's information from other master
// 	updateIndirectNodeTable(add,destNode,nextHop is a Master);// add an indirectly controlled node,and the nexthop from the master to this node
// }
// // Node
// {
// 	receiveNodeFromMaster();
// 	if the node is my neighbor then
// 		updatePathToMaster(add);// add a new path that the node can reach the master through this neighbor
// 	else 
// 		doNothing;
// }

// ############# Master detects the connection failure with a directly controlled node ############# 
// {
// 	// Either the Node is failed or the link is failed.
// 	Dijkstra(Master,destNode);// calculate a shortest path from the master to the destNode
// 	sendHello();// send a Hello to the destNode via the new path
// 	if reply then
// 		// link is failed
// 		updateDirectNodeTable(modify,destNode,nextHop is an other Node or a Master);// 直接控制Node的下一跳更新为通过其他Node或Master到达
// 	else
// 		// Node is failed
// 		updateDirectNodeTable(del,destNode);
// 		for i 0 to AllMaster.size() by 1 do // notify other masters
// 		{
// 			sendLinkMessage(AllMaster[i]);
// 		}
// 		for i 0 to AllDirectNode.size() by 1 do //
// 		{
// 			if affected then // notify the affected nodes
// 				sendLinkMessage(AllDirectNode[i]);
// 			else
// 				doNothing;
// 		} 
// 	updateKernelRoute();

// 	// 可能存在其他直接控制的Node，原本Master需要通过该失效Node才能和它们通信	
// 	for i 0 to AllDirectNode by 1 do 
// 	{
// 		if nextHop is the failed Node then
// 			Dijkstra(Master,AllDirectNode[i]);
// 			updateDirectNodeTable(modify,AllDirectNode[i],nextHop is an other Node or a Mater);
// 			updateKernelRoute();
// 		else
// 			doNothing;
// 	} 

// 	// master receive a topology update message from other master
// 	if a node is up or down then
// 		updateIndirectNodeTable();

// 	for i 0 to AllDirectNode.size() by 1 do //
// 	{
// 		if affected then // notify the affected node
// 			sendLinkMessage(AllDirectNode[i]);
// 		else
// 			doNothing;
// 	} 
// }

// ############# Node detects the connection failure with the master ############# 
// {
// 	updatePathToMaster(del);
// 	updateKernelRoute();
// }

// ############# Master sends a message to a master or a node ############# 
// {
// 	// Master send a message
// 	if there is a route then
// 		send;
// 	else
// 	{
// 		Dijkstra(Master,destNode);
// 		encapsulate the path in the message;
// 		send the message to the master or the node that is the transfer station;
// 	}	

// 	// Master receive a message
// 	parse the message;
// 	if the master is the destination then
// 		deal with the message;
// 	else if the next transfer node is a directly controlled node or a direct connected master then
// 		send the message to the node or the master;
// 	else 
// 		error;
// 		reply;

// 	// Node receive a message
// 	parse the message;
// 	if the node is the destination then
// 		deal with the message;
// 	else if there is a route to the next transfer node then
// 		send;
// 	else 
// 		error;
// 		reply;
// }

// ############# Node sends a message to a master ############# 
// {
// 	// Node send a message to a master
// 	send the message to a master or a neighbor;

// 	// Node receive a message 
// 	if there is a route to the master then
// 		send;
// 	else 
// 		send to a neighbor;

// 	if the message is received from a spine node and there is not a route to the master then
// 		send' message to a ToR;
// }


############# Master ############# 

void ReceiveMessage()
	// first
	Normal processing message
	// 这个报文是转发的，
	if message is forward then 
		if node is direct then
			AddDirConNodeForThisNode()//bianli
			send NodeList to other master
		else if node is indirect then
			if it is a new other node then
				ChangeDirConNodeForThisNode()//bianli
				send NodeList to other master
			else 
				doNothing
	else 
		......
	send NodeList to node 

void SendToNode()
	for i <- 0 to NodeList.size() by 1 do
		if destination == NodeList[i].address then
			if it is directly connected then
				send()
				break
			else
				while (1)
					j <- Hash()
					send(NodeList[i].indirNode[j])
					waiting for reply
					if time out then
						remove NodeList[i].indirNode[j]
					if NodeList[i] indirNode is NULL then
						for counter <- 0 to 10 by 1 do 
							k <- Hash()
							send(NodeList[k].address)
							waiting for reply
							if time out or can not reach then 
								continue
							else 
								sending successfully
								break
						end
						if sending successfully then 
							NodeList[i].indirNode <- NodeList[k].address
							break
						else
							if 
							// 查找该Node的直连链路情况，如果链路全挂了，或者正常直连链路的邻居的直连链路全挂了，则不会启动BGP
							else
								go back to BGP
					else 
						continue
					break
	end
						


############# Node ############# 

int AddOtherDirConNode(int i)
	for j <- 0 to PathTable.size() by 1 do
		if PathTable[j] DirConFlag is true then
			OtherDirConNodeList[i] <- PathTable[j]
			return j
	end 
	return -1

void SendToMaster()
	if is connected with master then
		send(Master)
	else
		i <- Hash()
		if OtherDirConNodeList[i].DirConFlag is true then
			send(OtherDirConNodeList[i])
		else 
			temp <- AddOtherDirConNode(i)
			if temp==-1 then 
				node can not connect with master
			else
				send(OtherDirConNodeList[i],new otherNode)
		waiting for reply......

		LOOP:if time out then 
			PathTable[j].DirConFlag <- false
			temp <- AddOtherDirConNode(i)
			if temp is false then
				node can not connect with master
			else 
				send(OtherDirConNodeList[i],new otherNode)
			goto LOOP
		else
			......

void ReceiveMessage()
	if it is a reply message then 
		finish the SendToMaster thread
	else if destination is master then
		send(Master,forward)
		reply
		// parse this message 
		sourceAddress <- GetSource()
		for i <- 0 to PathTable.size() by 1 do
			if PathTable[i].address = sourceAddress then
				PathTable[i].DirConFlag <- false
				break
		end
	else if destination is a node and source is master then
		if destination is reachable then 
			send(other node)
		reply to master
	else if it is a table that master could directly connect node then
		for i <- 0 to Table.size() by 1 do
			for j <- 0 to PathTable.size() by 1 do
				if Table[i].address == PathTable[j].address then
					if Table[i].DirConFlag != PathTable[j].DirConFlag then
						PathTable[j].DirConFlag <- Table[i].DirConFlag
			end
		end
	else
		......

