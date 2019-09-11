// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! WARNING: INSECURE CRYPTOGRAPHIC OPERATIONS.
//!
//! This module contains cryptographic operations which are considered insecure.
//! These operations should only be used for compatibility with legacy systems -
//! never in new systems!

#![deprecated(note = "insecure cryptographic operations")]

#[allow(deprecated)]
pub use hash::insecure_sha1_digest::InsecureSha1Digest;
#[allow(deprecated)]
pub use hmac::insecure_hmac_sha1::InsecureHmacSha1;

#[cfg(feature = "kdf")]
#[allow(deprecated)]
pub use kdf::insecure_pbkdf2_hmac_sha1::insecure_pbkdf2_hmac_sha1;
