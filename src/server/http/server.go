package http

import (
	"context"
	"fmt"
	"net"
	"net/http"
	"os"
)

type HttpServer interface {
	Serve() error
	Shutdown() error
}

func NewPortServer(port uint, handler http.Handler, errorChan chan error) HttpServer {
	server := new(portServer)
	server.Handler = handler
	server.port = port
	server.errorChan = errorChan
	return server
}

func NewSockServer(sock string, handler http.Handler, errorChan chan error) HttpServer {
	server := new(sockServer)
	server.Handler = handler
	server.sock = sock
	server.errorChan = errorChan
	return server
}

type portServer struct {
	http.Server
	port      uint
	errorChan chan error
}

type sockServer struct {
	http.Server
	sock      string
	errorChan chan error
}

func serve(s *http.Server, l net.Listener, errorChan chan error) {
	go func() {
		err := s.Serve(l)
		if err != http.ErrServerClosed {
			errorChan <- err
		}
	}()
}

func shutdown(s *http.Server) error {
	return s.Shutdown(context.Background())
}

func (s portServer) Serve() error {
	l, err := net.Listen("tcp", fmt.Sprintf(":%d", s.port))
	if err != nil {
		return err
	}
	serve(&s.Server, l, s.errorChan)
	return nil
}

func (s portServer) Shutdown() error {
	return shutdown(&s.Server)
}

func (s sockServer) Serve() error {
	_ = os.Remove(s.sock)
	l, err := net.Listen("unix", s.sock)
	if err != nil {
		return err
	}
	serve(&s.Server, l, s.errorChan)
	return nil
}

func (s sockServer) Shutdown() error {
	err := shutdown(&s.Server)
	_ = os.Remove(s.sock)
	return err
}
