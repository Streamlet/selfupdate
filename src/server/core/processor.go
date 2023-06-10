package core

import (
	"selfupdate_server/config"
	"selfupdate_server/version"
)

type Processor struct {
	packageConfigMap map[string]config.PackageConfig
}

func NewProcessor(packageConfigMap map[string]config.PackageConfig) Processor {
	return Processor{packageConfigMap}
}

func (this Processor) process(packageName string, clientVersion *version.Version) *UpgradeResult {
	packageConfig, found := this.packageConfigMap[packageName]
	if !found {
		return nil
	}

	var result UpgradeResult
	result.PackageName = packageName
	result.HasNewVersion = false

	var matchedPolicy *config.UpgradePolicy
	for _, policy := range packageConfig.Policies {
		for _, versionRange := range policy.VersionRanges {
			if versionRange.Matches(clientVersion) {
				matchedPolicy = &policy
				break
			}
		}
		if matchedPolicy != nil {
			break
		}
	}

	if matchedPolicy == nil {
		return &result
	}

	packageInfo, found := packageConfig.Versions[matchedPolicy.Target]
	if !found {
		return &result
	}

	result.HasNewVersion = true
	result.PackageVersion = &matchedPolicy.Target
	result.ForceUpdate = matchedPolicy.Force
	result.PackageURL = &packageInfo.Url
	result.PackageSize = &packageInfo.Size
	result.PackageFormat = &packageInfo.Format
	result.PackageHash = packageInfo.Hash
	if matchedPolicy.Title != nil {
		result.UpdateTitle = matchedPolicy.Title
	} else {
		result.UpdateTitle = &packageInfo.Title
	}
	if matchedPolicy.Description != nil {
		result.UpdateDescription = matchedPolicy.Description
	} else {
		result.UpdateDescription = &packageInfo.Description
	}

	return &result
}
