// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Package pkg contains the in memory representations of a Package in the pm
// system and associated utilities.
package pkg

import (
	"errors"
	"strings"
)

// ErrInvalidPackageName occurs when a package name contains one of InvalidPackageChars or is any of the exact InvalidNames.
var ErrInvalidPackageName = errors.New("pkg: invalid package name")

// ErrInvalidPackageVersion occurs when a package version contains one of InvalidPackageChars or is any of the exact InvalidNames.
var ErrInvalidPackageVersion = errors.New("pkg: invalid package version")

// InvalidPackageChars contains a list of characters that are disallowed in package names
const InvalidPackageChars = "/="

// InvalidNames are package names & versions that are invalid
var InvalidNames = []string{".", ".."}

// Package is a representation of basic package metadata
type Package struct {
	Name    string `json:"name"`
	Version string `json:"version"`
}

// Validate returns an error if the package contains an invalid value in one of it's fields.
func (pkg *Package) Validate() error {
	if strings.ContainsAny(pkg.Name, InvalidPackageChars) {
		return ErrInvalidPackageName
	}
	if strings.ContainsAny(pkg.Version, InvalidPackageChars) {
		return ErrInvalidPackageVersion
	}
	for _, name := range InvalidNames {
		if pkg.Name == name {
			return ErrInvalidPackageName
		}
		if pkg.Version == name {
			return ErrInvalidPackageVersion
		}
	}
	return nil
}
