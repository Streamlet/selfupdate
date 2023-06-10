package core

import (
	"errors"
	"net/url"
	"selfupdate_server/version"
	"strings"
)

func parseRequestPath(urlPath string) (*string, *version.Version, error) {
	normalizedurlPath, err := url.PathUnescape(urlPath)
	if err != nil {
		return nil, nil, err
	}
	// /packageName/clientVersion
	urlParts := strings.Split(strings.Trim(normalizedurlPath, "/"), "/")
	if len(urlParts) != 2 {
		return nil, nil, errors.New("invalid request path")
	}
	packageName := urlParts[0]
	clientVersion := urlParts[1]
	parsedVersion, err := version.ParseVersion(clientVersion)
	if err != nil {
		return nil, nil, err
	}

	return &packageName, parsedVersion, nil
}
