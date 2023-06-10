package config

import (
	"io/fs"
	"os"
	"path/filepath"
	"selfupdate_server/version"

	"gopkg.in/yaml.v3"
)

type PackageConfig struct {
	Package  string                 `yaml:"package"`
	Versions map[string]PackageInfo `yaml:"versions"`
	Policies []UpgradePolicy        `yaml:"policies"`
}

type PackageInfo struct {
	Url         string            `yaml:"url"`
	Size        uint              `yaml:"size"`
	Format      string            `yaml:"format"`
	Hash        map[string]string `yaml:"hash"`
	Title       string            `yaml:"title"`
	Description string            `yaml:"description"`
}

type UpgradePolicy struct {
	Matches       []string `yaml:"matches"`
	VersionRanges []version.VersionRange
	Target        string  `yaml:"target"`
	Force         *bool   `yaml:"force,omitempty"`
	Title         *string `yaml:"title,omitempty"`
	Description   *string `yaml:"description,omitempty"`
}

func Load(yamlPath string) (*PackageConfig, error) {
	content, err := os.ReadFile(yamlPath)
	if err != nil {
		return nil, err
	}

	var packageConfig PackageConfig
	err = yaml.Unmarshal(content, &packageConfig)
	if err != nil {
		return nil, err
	}
	for i, policy := range packageConfig.Policies {
		policy.VersionRanges = []version.VersionRange{}
		for _, match := range policy.Matches {
			versionRange, err := version.ParseVersionRange(match)
			if err != nil {
				return nil, err
			}
			packageConfig.Policies[i].VersionRanges = append(packageConfig.Policies[i].VersionRanges, *versionRange)
		}
	}
	return &packageConfig, nil
}

func ScanFolder(yamlFolder string) (map[string]PackageConfig, error) {
	packageConfigMap := map[string]PackageConfig{}
	err := filepath.Walk(yamlFolder, func(path string, info fs.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if info.IsDir() || (filepath.Ext(path) != ".yaml" && filepath.Ext(path) != ".yml") {
			return nil
		}
		packageConfig, err := Load(path)
		if err != nil {
			return err
		}
		packageConfigMap[packageConfig.Package] = *packageConfig
		return nil
	})
	if err != nil {
		return nil, err
	}
	return packageConfigMap, nil
}
