package version

import (
	"errors"
	"strconv"
	"strings"
)

type Version struct {
	RawString string
	Parts     []uint
}

func ParseVersion(version string) (*Version, error) {
	version = strings.TrimSpace(version)
	versionNumbers := []uint{}
	if len(version) == 0 {
		return nil, nil
	}
	versionPart := strings.Split(version, ".")
	for _, versionString := range versionPart {
		versionNumber, err := strconv.ParseUint(versionString, 10, 0)
		if err != nil {
			return nil, err
		}
		versionNumbers = append(versionNumbers, uint(versionNumber))
	}
	return &Version{version, versionNumbers}, nil
}

func (this *Version) Matches(that *Version) bool {
	if this == nil {
		return true
	}
	if that == nil {
		return true
	}
	minParts := len(this.Parts)
	if minParts > len(that.Parts) {
		minParts = len(that.Parts)
	}
	for i := 0; i < minParts; i++ {
		if this.Parts[i] != that.Parts[i] {
			return false
		}
	}
	if len(this.Parts) <= len(that.Parts) {
		return true
	}
	for _, v := range this.Parts[minParts:] {
		if v > 0 {
			return false
		}
	}
	return true
}

func (this *Version) Equals(that *Version) bool {
	if this == nil {
		return that == nil
	}
	if that == nil {
		return this == nil
	}
	if len(this.Parts) != len(that.Parts) {
		return false
	}
	for i := 0; i < len(this.Parts); i++ {
		if this.Parts[i] != that.Parts[i] {
			return false
		}
	}
	return true
}

func (this *Version) LittleThan(that *Version) bool {
	if this == nil {
		return that != nil
	}
	if that == nil {
		return false
	}
	minParts := len(this.Parts)
	if minParts > len(that.Parts) {
		minParts = len(that.Parts)
	}
	for i := 0; i < minParts; i++ {
		if this.Parts[i] < that.Parts[i] {
			return true
		}
		if this.Parts[i] > that.Parts[i] {
			return false
		}
	}
	if len(this.Parts) >= len(that.Parts) {
		return false
	}
	for _, v := range that.Parts[minParts:] {
		if v > 0 {
			return true
		}
	}
	return false
}

func (this *Version) GreaterThan(that *Version) bool {
	if this == nil {
		return false
	}
	if that == nil {
		return this != nil
	}
	minParts := len(this.Parts)
	if minParts > len(that.Parts) {
		minParts = len(that.Parts)
	}
	for i := 0; i < minParts; i++ {
		if this.Parts[i] > that.Parts[i] {
			return true
		}
		if this.Parts[i] < that.Parts[i] {
			return false
		}
	}
	if len(this.Parts) <= len(that.Parts) {
		return false
	}
	for _, v := range this.Parts[minParts:] {
		if v > 0 {
			return true
		}
	}
	return false
}

type VersionRange struct {
	Begin        *Version
	IncludeBegin bool
	End          *Version
	IncludeEnd   bool
}

func ParseVersionRange(versionRange string) (*VersionRange, error) {
	versionRange = strings.TrimSpace(versionRange)
	if (len(versionRange)) == 0 {
		return nil, errors.New("invalid version range expression: should not be empty")
	}
	beginChar := versionRange[0]
	endChar := versionRange[len(versionRange)-1]
	if (beginChar != '[' && beginChar != '(') || (endChar != ')' && endChar != ']') {
		version, err := ParseVersion(versionRange)
		if err != nil {
			return nil, errors.New("invalid version range expression: should begin with '[' or '(', end with ']' or ')'")
		}
		return &VersionRange{version, true, version, true}, nil
	}
	versions := strings.Split(versionRange[1:len(versionRange)-1], ",")
	if len(versions) != 2 {
		return nil, errors.New("invalid version range expression: should include two version strings separated by ','")
	}
	beginVersion, err := ParseVersion(versions[0])
	if err != nil {
		return nil, err
	}
	endVersion, err := ParseVersion(versions[1])
	if err != nil {
		return nil, err
	}
	return &VersionRange{beginVersion, beginChar == '[', endVersion, endChar == ']'}, nil
}

func (this *VersionRange) Matches(version *Version) bool {
	if this.Begin != nil {
		if !((this.IncludeBegin && this.Begin.Matches(version)) || this.Begin.LittleThan(version)) {
			return false
		}
	}
	if this.End != nil {
		if !((this.IncludeEnd && this.End.Matches(version)) || this.End.GreaterThan(version)) {
			return false
		}
	}
	return true
}
