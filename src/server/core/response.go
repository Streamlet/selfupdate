package core

type UpgradeResult struct {
	PackageName       string            `json:"package_name"`
	HasNewVersion     bool              `json:"has_new_version"`
	PackageVersion    *string           `json:"package_version,omitempty"`
	ForceUpdate       *bool             `json:"force_update,omitempty"`
	PackageURL        *string           `json:"package_url,omitempty"`
	PackageSize       *uint             `json:"package_size,omitempty"`
	PackageFormat     *string           `json:"package_format,omitempty"`
	PackageHash       map[string]string `json:"package_hash,omitempty"`
	UpdateTitle       *string           `json:"update_title,omitempty"`
	UpdateDescription *string           `json:"update_description,omitempty"`
}
