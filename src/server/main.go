package main

import (
	"flag"
	"fmt"
	"os"
	"os/signal"
	"selfupdate_server/config"
	"selfupdate_server/core"
	"selfupdate_server/http"
	"syscall"
)

type commandLineArgs struct {
	sock string
	port uint
	root string
}

func main() {
	var args commandLineArgs
	flag.StringVar(&args.sock, "sock", "", "unix sock file")
	flag.UintVar(&args.port, "port", 80, "listen port")
	flag.StringVar(&args.root, "root", "", "root directory")
	flag.Parse()

	packageConfigMap, err := config.ScanFolder(args.root)
	if err != nil {
		fmt.Printf("Failed to parse config: %s, %s.\n", args.root, err.Error())
		return
	}

	processor := core.NewProcessor(packageConfigMap)

	errorChan := make(chan error)
	var srv http.HttpServer
	if args.sock != "" {
		srv = http.NewSockServer("", core.NewRouter(processor), errorChan)
	} else if args.port > 0 {
		srv = http.NewPortServer(80, core.NewRouter(processor), errorChan)
	} else {
		return
	}

	err = srv.Serve()
	if err != nil {
		fmt.Printf("Failed to start server: %s.\n", err.Error())
		return
	}

	fmt.Printf("Server started")
	if args.sock != "" {
		fmt.Printf(" on sock '%s'.\n", "")
	} else if args.port > 0 {
		fmt.Printf(" on port %d.\n", 80)
	}
	fmt.Printf("Root directory: %s\n", args.root)
	fmt.Printf("\n")

	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM)

	for {
		select {
		case <-sig:
			fmt.Printf("Server shutdown.\n")
		case err := <-errorChan:
			fmt.Printf("Server error: %s\n", err.Error())
		}
		break
	}

	_ = srv.Shutdown()
}
