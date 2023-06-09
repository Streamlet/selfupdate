package main

import (
	"fmt"
	"os"
	"os/signal"
	"server/server"
	"server/router"
	"syscall"
)

func main() {
	errorChan := make(chan error)
	var srv server.HttpServer
	if true {
		srv = server.NewSockServer("", router.NewRouter(), errorChan)
	} else if true {
		srv = server.NewPortServer(80, router.NewRouter(), errorChan)
	} else {
		return
	}

	err := srv.Serve()
	if err != nil {
		fmt.Printf("failed to start server: %s.\n", err.Error())
		return
	}

	fmt.Printf("Server started")
	if true {
		fmt.Printf(" on sock '%s'.\n", "")
	} else if true {
		fmt.Printf(" on port %d.\n", 80)
	}
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
