// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {argh::FromArgs, ffx_core::ffx_command};

#[ffx_command()]
#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "sdk", description = "Modify or query the installed SDKs")]
pub struct SdkCommand {
    #[argh(subcommand)]
    pub sub: SubCommand,
}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand)]
pub enum SubCommand {
    Version(VersionCommand),
}

#[derive(FromArgs, Debug, PartialEq)]
#[argh(subcommand, name = "version", description = "Retrieve the version of the current SDK")]
pub struct VersionCommand {}
