package main

import (
	"flag"
	"fmt"
	"github.com/swiftstack/ProxyFS/consensus"
	"os"
	"time"
)

func setupConnection() (cs *consensus.EtcdConn) {

	// TODO - endpoints should be an option or grab from configuration
	// file of etcd.conf
	endpoints := []string{"192.168.60.10:2379", "192.168.60.11:2379", "192.168.60.12:2379"}

	// Create an etcd client - our current etcd setup does not listen on
	// localhost.  Therefore, we pass the IP addresses used by etcd.
	hostName, _ := os.Hostname()
	cs, err := consensus.New(endpoints, hostName, 2*time.Second)
	if err != nil {
		fmt.Printf("Register() returned err: %v\n", err)
		os.Exit(-1)
	}

	// Setup to do administrative operations
	cs.CLI()

	return cs
}

func teardownConnection(cs *consensus.EtcdConn) {

	// Unregister from the etcd cluster
	cs.Close()
}

func listNode(cs *consensus.EtcdConn, node string, nodeInfo consensus.AllNodeInfo) {

	if node != "" {
		fmt.Printf("Node: %v State: %v Last HB: %v\n", node, nodeInfo.NodesState[node],
			nodeInfo.NodesHb[node])
		return
	}
	fmt.Printf("Node Information\n")
	for n := range nodeInfo.NodesState {
		fmt.Printf("\tNode: %v State: %v Last HB: %v\n", n, nodeInfo.NodesState[n], nodeInfo.NodesHb[n])
	}

}

func listVg(cs *consensus.EtcdConn, vgName string, vgInfo *consensus.VgInfo) {

	fmt.Printf("\tVG: %v State: '%v' Node: '%v' Ipaddr: '%v' Netmask: '%v' Nic: '%v'\n",
		vgName, vgInfo.VgState, vgInfo.VgNode, vgInfo.VgIpAddr, vgInfo.VgNetmask, vgInfo.VgNic)
	fmt.Printf("\t\tAutofail: %v Enabled: %v\n", vgInfo.VgAutofail, vgInfo.VgEnabled)

	fmt.Printf("\tVolumes:\n")
	for _, v := range vgInfo.VgVolumeList {
		fmt.Printf("\t\t%v\n", v)
	}
	fmt.Printf("\n")
}

// listOp handles a listing
func listOp(nodeName string, vgName string) {

	cs := setupConnection()

	allVgInfo := cs.ListVg()
	allNodeInfo := cs.ListNode()

	if vgName != "" {
		if allVgInfo[vgName] != nil {
			listVg(cs, vgName, allVgInfo[vgName])
		} else {
			fmt.Printf("vg: %v does not exist\n", vgName)
		}
	} else if nodeName != "" {
		if allNodeInfo.NodesState[nodeName] != "" {
			listNode(cs, nodeName, allNodeInfo)
		} else {
			fmt.Printf("Node: %v does not exist\n", nodeName)
		}
	} else {
		listNode(cs, "", allNodeInfo)

		fmt.Printf("\nVolume Group Information\n")
		for name, vgInfo := range allVgInfo {
			listVg(cs, name, vgInfo)
		}
	}

	teardownConnection(cs)
}

func main() {

	// Setup Subcommands
	listCommand := flag.NewFlagSet("list", flag.ExitOnError)
	offlineCommand := flag.NewFlagSet("offline", flag.ExitOnError)
	onlineCommand := flag.NewFlagSet("online", flag.ExitOnError)
	stopCommand := flag.NewFlagSet("stop", flag.ExitOnError)
	/*
		  TODO - add watch commands to watch for changes for
		  example a VG, etc.   Also, add command to make it easier
		  for controller to configure, learn keys, etc.  Supportability
		  requirements?

			watchCommand := flag.NewFlagSet("watch", flag.ExitOnError)
	*/

	// List subcommand flag pointers
	listNodePtr := listCommand.String("node", "", "node to list - list all if empty")
	listVgPtr := listCommand.String("vg", "", "volume group to list - list all if empty")

	// Offline subcommand flag pointers
	offlineVgPtr := offlineCommand.String("vg", "", "volume group to offline (required)")

	// Online subcommand flag pointers
	onlineNodePtr := onlineCommand.String("node", "", "node where to online VG (required)")
	onlineVgPtr := onlineCommand.String("vg", "", "volume group to online (required)")

	// Stop subcommand
	stopNodePtr := stopCommand.String("node", "", "node to be stopped (required)")

	// Verify that a subcommand has been provided
	// os.Arg[0] is the main command
	// os.Arg[1] will be the subcommand
	if len(os.Args) < 2 {
		fmt.Println("list, offline, online or stop subcommand is required")
		flag.PrintDefaults()
		os.Exit(1)
	}

	// Switch on the subcommand
	// Parse the flags for appropriate FlagSet
	// FlagSet.Parse() requires a set of arguments to parse as input
	// os.Args[2:] will be all arguments starting after the subcommand at os.Args[1]
	switch os.Args[1] {
	case "list":
		listCommand.Parse(os.Args[2:])
	case "offline":
		offlineCommand.Parse(os.Args[2:])
	case "online":
		onlineCommand.Parse(os.Args[2:])
	case "stop":
		stopCommand.Parse(os.Args[2:])
	default:
		// TODO - fix this error message
		fmt.Println("invalid subcommand - valid subcommands are list, offline, online or stop")
		flag.PrintDefaults()
		os.Exit(1)
	}

	// Check which subcommand was Parsed for list
	if listCommand.Parsed() {
		if (*listNodePtr != "") && (*listVgPtr != "") {
			listCommand.PrintDefaults()
			os.Exit(1)
		}
		listOp(*listNodePtr, *listVgPtr)
	}

	// Check which subcommand was Parsed for offline
	if offlineCommand.Parsed() {

		if *offlineVgPtr == "" {
			offlineCommand.PrintDefaults()
			os.Exit(1)
		}

		cs := setupConnection()

		// Offline the VG
		err := cs.CLIOfflineVg(*offlineVgPtr)
		if err != nil {
			fmt.Printf("offline failed with error: %v\n", err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	// Check which subcommand was Parsed for online
	if onlineCommand.Parsed() {

		if (*onlineNodePtr == "") || (*onlineVgPtr == "") {
			onlineCommand.PrintDefaults()
			os.Exit(1)
		}

		cs := setupConnection()

		// Online the VG
		err := cs.CLIOnlineVg(*onlineVgPtr, *onlineNodePtr)
		if err != nil {
			fmt.Printf("online failed with error: %v\n", err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	// Check which subcommand was Parsed for stop
	if stopCommand.Parsed() {

		if *stopNodePtr == "" {
			stopCommand.PrintDefaults()
			os.Exit(1)
		}

		cs := setupConnection()

		// Stop the node and wait for it to reach DEAD state
		// Offline all VGs on the node and stop the node
		if *stopNodePtr != "" {
			err := cs.CLIStopNode(*stopNodePtr)
			if err != nil {
				fmt.Printf("Stop failed with error: %v\n", err)
				os.Exit(1)
			}
			os.Exit(0)
		}
	}
}
