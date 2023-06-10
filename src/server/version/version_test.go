package version

import (
	"testing"
)

func TestParseVersion(t *testing.T) {
	if v, e := ParseVersion(""); e != nil || v != nil {
		t.Error("ParseVersion ''")
	}
	if v, e := ParseVersion(" "); e != nil || v != nil {
		t.Error("ParseVersion ' '")
	}
	if v, e := ParseVersion("1"); e != nil || len(v.Parts) != 1 || v.Parts[0] != 1 {
		t.Error("ParseVersion '1'")
	}
	if v, e := ParseVersion(" 1 "); e != nil || len(v.Parts) != 1 || v.Parts[0] != 1 {
		t.Error("ParseVersion ' 1 '")
	}
	if v, e := ParseVersion("1.2"); e != nil || len(v.Parts) != 2 || v.Parts[0] != 1 || v.Parts[1] != 2 {
		t.Error("ParseVersion '1.2'")
	}
	if v, e := ParseVersion(" 1.2 "); e != nil || len(v.Parts) != 2 || v.Parts[0] != 1 || v.Parts[1] != 2 {
		t.Error("ParseVersion ' 1.2 '")
	}
	if v, e := ParseVersion("1.2.3"); e != nil || len(v.Parts) != 3 || v.Parts[0] != 1 || v.Parts[1] != 2 || v.Parts[2] != 3 {
		t.Error("ParseVersion '1.2.3'")
	}
	if v, e := ParseVersion(" 1.2.3 "); e != nil || len(v.Parts) != 3 || v.Parts[0] != 1 || v.Parts[1] != 2 || v.Parts[2] != 3 {
		t.Error("ParseVersion ' 1.2.3 '")
	}

	if v, e := ParseVersion("1 .2"); e == nil || v != nil {
		t.Error("ParseVersion '1 .2'")
	}
	if v, e := ParseVersion("1. 2"); e == nil || v != nil {
		t.Error("ParseVersion '1. 2'")
	}
	if v, e := ParseVersion("1 . 2"); e == nil || v != nil {
		t.Error("ParseVersion '1 . 2'")
	}
	if v, e := ParseVersion("1..2"); e == nil || v != nil {
		t.Error("ParseVersion '1..2'")
	}
	if v, e := ParseVersion(".1.2"); e == nil || v != nil {
		t.Error("ParseVersion '.1.2'")
	}
	if v, e := ParseVersion("1.2."); e == nil || v != nil {
		t.Error("ParseVersion '1.2.'")
	}
}

func TestParseVersionRange(t *testing.T) {
	if r, e := ParseVersionRange("[,]"); e != nil || r.Begin != nil || !r.IncludeBegin || r.End != nil || !r.IncludeEnd {
		t.Error("ParseVersionRange '[,]'")
	}
	if r, e := ParseVersionRange("[,)"); e != nil || r.Begin != nil || !r.IncludeBegin || r.End != nil || r.IncludeEnd {
		t.Error("ParseVersionRange '[,)'")
	}
	if r, e := ParseVersionRange("(,]"); e != nil || r.Begin != nil || r.IncludeBegin || r.End != nil || !r.IncludeEnd {
		t.Error("ParseVersionRange '(,]'")
	}
	if r, e := ParseVersionRange("(,)"); e != nil || r.Begin != nil || r.IncludeBegin || r.End != nil || r.IncludeEnd {
		t.Error("ParseVersionRange '(,)'")
	}

	if r, e := ParseVersionRange("[1,]"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || r.End != nil || !r.IncludeEnd {
		t.Error("ParseVersionRange '[1,]'")
	}
	if r, e := ParseVersionRange("[,1]"); e != nil || r.Begin != nil || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange '[,1]'")
	}
	if r, e := ParseVersionRange("[1,1]"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange '[1,1]'")
	}
	if r, e := ParseVersionRange("[1,)"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || r.End != nil || r.IncludeEnd {
		t.Error("ParseVersionRange '[1,)'")
	}
	if r, e := ParseVersionRange("[,1)"); e != nil || r.Begin != nil || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange '[,1)'")
	}
	if r, e := ParseVersionRange("[1,1)"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange '[1,1)'")
	}
	if r, e := ParseVersionRange("(1,]"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || r.End != nil || !r.IncludeEnd {
		t.Error("ParseVersionRange '(1,]'")
	}
	if r, e := ParseVersionRange("(,1]"); e != nil || r.Begin != nil || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange '(,1]'")
	}
	if r, e := ParseVersionRange("(1,1]"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange '(1,1]'")
	}
	if r, e := ParseVersionRange("(1,)"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || r.End != nil || r.IncludeEnd {
		t.Error("ParseVersionRange '(1,)'")
	}
	if r, e := ParseVersionRange("(,1)"); e != nil || r.Begin != nil || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange '(,1)'")
	}
	if r, e := ParseVersionRange("(1,1)"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange '(1,1)'")
	}

	if r, e := ParseVersionRange(" [ 1 , 1 ] "); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange ' [ 1 , 1 ] '")
	}
	if r, e := ParseVersionRange(" [ 1 , 1 ) "); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange ' [ 1 , 1 ) '")
	}
	if r, e := ParseVersionRange(" ( 1 , 1 ] "); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange ' ( 1 , 1 ] '")
	}
	if r, e := ParseVersionRange(" ( 1 , 1 ) "); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || r.IncludeEnd {
		t.Error("ParseVersionRange ' ( 1 , 1 ) '")
	}
	if r, e := ParseVersionRange("1"); e != nil || len(r.Begin.Parts) != 1 || r.Begin.Parts[0] != 1 || !r.IncludeBegin || len(r.End.Parts) != 1 || r.End.Parts[0] != 1 || !r.IncludeEnd {
		t.Error("ParseVersionRange '1'")
	}

	if v, e := ParseVersionRange(""); e == nil || v != nil {
		t.Error("ParseVersionRange ''")
	}
	if v, e := ParseVersionRange("["); e == nil || v != nil {
		t.Error("ParseVersionRange '[")
	}
	if v, e := ParseVersionRange("]"); e == nil || v != nil {
		t.Error("ParseVersionRange ']")
	}
	if v, e := ParseVersionRange("("); e == nil || v != nil {
		t.Error("ParseVersionRange '(")
	}
	if v, e := ParseVersionRange(")"); e == nil || v != nil {
		t.Error("ParseVersionRange ')")
	}
	if v, e := ParseVersionRange("[,,]"); e == nil || v != nil {
		t.Error("ParseVersionRange '[,,]'")
	}
}

func TestVersionCompare(t *testing.T) {
	v0, _ := ParseVersion("")
	v1, _ := ParseVersion("1")
	v2, _ := ParseVersion("1.2")
	v20, _ := ParseVersion("1.2.0")

	if !v0.Matches(v0) {
		t.Error("v0.Matches(v0)")
	}
	if !v0.Matches(v1) {
		t.Error("v0.Matches(v1)")
	}
	if !v0.Matches(v2) {
		t.Error("v0.Matches(v2)")
	}
	if !v1.Matches(v1) {
		t.Error("v1.Matches(v1)")
	}
	if !v1.Matches(v2) {
		t.Error("v1.Matches(v2)")
	}
	if !v2.Matches(v2) {
		t.Error("v2.Matches(v2)")
	}

	if !v2.Matches(v20) {
		t.Error("v2.Matches(v20)")
	}
	if !v20.Matches(v2) {
		t.Error("v20.Matches(v2)")
	}

	if !v0.Equals(v0) {
		t.Error("v0.Equals(v0)")
	}
	if v0.Equals(v1) {
		t.Error("v0.Equals(v1)")
	}
	if v0.Equals(v2) {
		t.Error("v0.Equals(v2)")
	}
	if !v1.Equals(v1) {
		t.Error("v1.Equals(v1)")
	}
	if v1.Equals(v2) {
		t.Error("v1.Equals(v2)")
	}
	if !v2.Equals(v2) {
		t.Error("v2.Equals(v2)")
	}
	if v2.Equals(v20) {
		t.Error("v2.Equals(v20)")
	}
	if v20.Equals(v2) {
		t.Error("v20.Equals(v2)")
	}

	if v0.LittleThan(v0) {
		t.Error("v0.LittleThan(v0)")
	}
	if !v0.LittleThan(v1) {
		t.Error("v0.LittleThan(v1)")
	}
	if !v0.LittleThan(v2) {
		t.Error("v0.LittleThan(v2)")
	}
	if v1.LittleThan(v1) {
		t.Error("v1.LittleThan(v1)")
	}
	if !v1.LittleThan(v2) {
		t.Error("v1.LittleThan(v2)")
	}
	if v2.LittleThan(v2) {
		t.Error("v2.LittleThan(v2)")
	}
	if v0.GreaterThan(v0) {
		t.Error("v0.GreaterThan(v0)")
	}
	if v0.GreaterThan(v1) {
		t.Error("v0.GreaterThan(v1)")
	}
	if v0.GreaterThan(v2) {
		t.Error("v0.GreaterThan(v2)")
	}
	if v1.GreaterThan(v1) {
		t.Error("v1.GreaterThan(v1)")
	}
	if v1.GreaterThan(v2) {
		t.Error("v1.GreaterThan(v2)")
	}
	if v2.GreaterThan(v2) {
		t.Error("v2.GreaterThan(v2)")
	}

	if v2.GreaterThan(v2) {
		t.Error("v2.GreaterThan(v2)")
	}
	if !v2.GreaterThan(v1) {
		t.Error("v2.GreaterThan(v1)")
	}
	if !v2.GreaterThan(v0) {
		t.Error("v2.GreaterThan(v0)")
	}
	if v1.GreaterThan(v1) {
		t.Error("v1.GreaterThan(v1)")
	}
	if !v1.GreaterThan(v0) {
		t.Error("v1.GreaterThan(v0)")
	}
	if v0.GreaterThan(v0) {
		t.Error("v0.GreaterThan(v0)")
	}
	if v2.LittleThan(v2) {
		t.Error("v2.LittleThan(v2)")
	}
	if v2.LittleThan(v1) {
		t.Error("v2.LittleThan(v1)")
	}
	if v2.LittleThan(v0) {
		t.Error("v2.LittleThan(v0)")
	}
	if v1.LittleThan(v1) {
		t.Error("v1.LittleThan(v1)")
	}
	if v1.LittleThan(v0) {
		t.Error("v1.LittleThan(v0)")
	}
	if v0.LittleThan(v0) {
		t.Error("v0.LittleThan(v0)")
	}

	v3, _ := ParseVersion("3")

	if v2.Matches(v3) {
		t.Error("v2.Matches(v3)")
	}
	if v2.Equals(v3) {
		t.Error("v2.Equals(v3)")
	}
	if !v2.LittleThan(v3) {
		t.Error("v2.LittleThan(v3)")
	}
	if v2.GreaterThan(v3) {
		t.Error("v2.GreaterThan(v3)")
	}
	if v3.LittleThan(v2) {
		t.Error("v3.LittleThan(v2)")
	}
	if !v3.GreaterThan(v2) {
		t.Error("v3.GreaterThan(v2)")
	}

	if !v2.Matches(v20) {
		t.Error("v2.Matches(v20)")
	}
	if v2.Equals(v20) {
		t.Error("v2.Equals(v20)")
	}
	if v2.LittleThan(v20) {
		t.Error("v2.LittleThan(v20)")
	}
	if v2.GreaterThan(v20) {
		t.Error("v2.GreaterThan(v20)")
	}
	if v20.LittleThan(v2) {
		t.Error("v20.LittleThan(v2)")
	}
	if v20.GreaterThan(v2) {
		t.Error("v20.GreaterThan(v2)")
	}
}

func TestVersionRangeMatch(t *testing.T) {
	v0, _ := ParseVersion("")
	v1, _ := ParseVersion("1")
	v2, _ := ParseVersion("1.2")

	r_kk, _ := ParseVersionRange("[,]")
	r_ko, _ := ParseVersionRange("[,)")
	r_ok, _ := ParseVersionRange("(,]")
	r_oo, _ := ParseVersionRange("(,)")

	if !r_kk.Matches(v0) {
		t.Error("r_kk.Matches(v0)")
	}
	if !r_kk.Matches(v1) {
		t.Error("r_kk.Matches(v1)")
	}
	if !r_kk.Matches(v2) {
		t.Error("r_kk.Matches(v2)")
	}

	if !r_ko.Matches(v0) {
		t.Error("r_ko.Matches(v0)")
	}
	if !r_ko.Matches(v1) {
		t.Error("r_ko.Matches(v1)")
	}
	if !r_ko.Matches(v2) {
		t.Error("r_ko.Matches(v2)")
	}

	if !r_ok.Matches(v0) {
		t.Error("r_ok.Matches(v0)")
	}
	if !r_ok.Matches(v1) {
		t.Error("r_ok.Matches(v1)")
	}
	if !r_ok.Matches(v2) {
		t.Error("r_ok.Matches(v2)")
	}

	if !r_oo.Matches(v0) {
		t.Error("r_oo.Matches(v0)")
	}
	if !r_oo.Matches(v1) {
		t.Error("r_oo.Matches(v1)")
	}
	if !r_oo.Matches(v2) {
		t.Error("r_oo.Matches(v2)")
	}

	r_kk_l, _ := ParseVersionRange("[1.2,]")
	r_ko_l, _ := ParseVersionRange("[1.2,)")
	r_ok_l, _ := ParseVersionRange("(1.2,]")
	r_oo_l, _ := ParseVersionRange("(1.2,)")

	if !r_kk_l.Matches(v2) {
		t.Error("r_kk_l.Matches(v2)")
	}
	if !r_ko_l.Matches(v2) {
		t.Error("r_ko_l.Matches(v2)")
	}
	if r_ok_l.Matches(v2) {
		t.Error("r_ok_l.Matches(v2)")
	}
	if r_oo_l.Matches(v2) {
		t.Error("r_oo_l.Matches(v2)")
	}

	r_kk_r, _ := ParseVersionRange("[,1.2]")
	r_ko_r, _ := ParseVersionRange("[,1.2)")
	r_ok_r, _ := ParseVersionRange("(,1.2]")
	r_oo_r, _ := ParseVersionRange("(,1.2)")

	if !r_kk_r.Matches(v2) {
		t.Error("r_kk_r.Matches(v2)")
	}
	if r_ko_r.Matches(v2) {
		t.Error("r_ko_r.Matches(v2)")
	}
	if !r_ok_r.Matches(v2) {
		t.Error("r_ok_r.Matches(v2)")
	}
	if r_oo_r.Matches(v2) {
		t.Error("r_oo_r.Matches(v2)")
	}

	r_kk_lr, _ := ParseVersionRange("[1.2,1.2]")
	r_ko_lr, _ := ParseVersionRange("[1.2,1.2)")
	r_ok_lr, _ := ParseVersionRange("(1.2,1.2]")
	r_oo_lr, _ := ParseVersionRange("(1.2,1.2)")

	if !r_kk_lr.Matches(v2) {
		t.Error("r_kk_lr.Matches(v2)")
	}
	if r_ko_lr.Matches(v2) {
		t.Error("r_ko_lr.Matches(v2)")
	}
	if r_ok_lr.Matches(v2) {
		t.Error("r_ok_lr.Matches(v2)")
	}
	if r_oo_lr.Matches(v2) {
		t.Error("r_oo_lr.Matches(v2)")
	}

	if r, _ := ParseVersionRange("(,1.1)"); r.Matches(v2) {
		t.Error("(,1.1) Matches 1.2")
	}
	if r, _ := ParseVersionRange("(,1.2.1)"); !r.Matches(v2) {
		t.Error("(,1.2.1) Matches 1.2")
	}
	if r, _ := ParseVersionRange("(,1.3)"); !r.Matches(v2) {
		t.Error("(,1.3) Matches 1.2")
	}
	if r, _ := ParseVersionRange("(1.1.9,)"); !r.Matches(v2) {
		t.Error("(1.1.9,) Matches 1.2")
	}
	if r, _ := ParseVersionRange("1.2"); !r.Matches(v2) {
		t.Error("1.2 Matches 1.2")
	}
	if r, _ := ParseVersionRange("1"); !r.Matches(v2) {
		t.Error("1 Matches 1.2")
	}
	if r, _ := ParseVersionRange("1.2.1"); r.Matches(v2) {
		t.Error("1.2.1 Matches 1.2")
	}
}
