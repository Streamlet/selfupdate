package core

import (
	"encoding/json"
	"log"
	"net/http"
)

func NewRouter(processor Processor) http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		packageName, clientVersion, err := parseRequestPath(r.RequestURI)
		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		result := processor.process(*packageName, clientVersion)
		if result == nil {
			log.Printf("Request %s from %s [package=%s version=%s] returned 404\n", r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString)
			w.WriteHeader(http.StatusNotFound)
			return
		}

		body, err := json.Marshal(result)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			return
		}
		if result.HasNewVersion {
			log.Printf("Request %s from %s [package=%s version=%s] returned [version=%s]\n", r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString, *result.PackageVersion)
		} else {
			log.Printf("Request %s from %s [package=%s version=%s] returned [has_new_version=false]\n", r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString)
		}

		w.WriteHeader(http.StatusOK)
		w.Header().Add("Content-Type", "application/json")
		w.Write(body)
	})

	return mux
}
