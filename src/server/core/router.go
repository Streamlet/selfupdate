package core

import (
	"encoding/json"
	"log"
	"net/http"
	"os"
	"path/filepath"
)

func NewRouter(processor Processor, root string) http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		packageName, clientVersion, err := parseRequestPath(r.URL.Path)
		if err != nil {
			if len(root) > 0 {
				path := filepath.Join(filepath.FromSlash(root), filepath.FromSlash(r.RequestURI))
				content, err := os.ReadFile(path)
				if err == nil {
					log.Printf("%s %s from %s returned raw file content: %s\n", r.Method, r.RequestURI, r.RemoteAddr, path)
					w.Header().Add("Content-Type", " application/octet-stream")
					w.WriteHeader(http.StatusOK)
					w.Write(content)
					return
				}
			}
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		result := processor.process(*packageName, clientVersion)
		if result == nil {
			log.Printf("%s %s from %s [package=%s version=%s] returned 404\n", r.Method, r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString)
			w.WriteHeader(http.StatusNotFound)
			return
		}

		body, err := json.Marshal(result)
		if err != nil {
			w.WriteHeader(http.StatusInternalServerError)
			return
		}
		if result.HasNewVersion {
			log.Printf("%s %s from %s [package=%s version=%s] returned [version=%s]\n", r.Method, r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString, *result.PackageVersion)
		} else {
			log.Printf("Request %s from %s [package=%s version=%s] returned [has_new_version=false]\n", r.RequestURI, r.RemoteAddr, *packageName, clientVersion.RawString)
		}

		w.Header().Add("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		w.Write(body)
	})

	return mux
}
