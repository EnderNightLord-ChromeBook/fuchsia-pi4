// Copyright 2021 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! This module contains the top level AT command library methods.  It contains a
//! trait for serialization and deserialization, which is implemented by the high level
//! generated Command and Response types by composing together the parsing and
//! unparsing methods for the low level ASTs with the the generated methods
//! for raising and lowering from and to the low level ASTs and high level generated tyoes.

pub use crate::parser::common::ParseError;
use {
    crate::{
        generated,
        lowlevel::{arguments::Arguments, command::Command, response::Response, write_to::WriteTo},
        parser::{command_parser, response_parser},
    },
    pest,
    std::io,
    thiserror::Error,
};

/// Errors that can occur while deserializing AT commands into high level generated AT command
/// and response types.
#[derive(Debug, Error)]
pub enum DeserializeError {
    // Just store the parse errors as Strings so as to not require clients to know about the
    // pest parser types.
    #[error("Parse error: {0:?}")]
    ParseError(String),
    #[error("IO error: {0:?}")]
    IoError(io::Error),
    #[error("Parsed unknown command: {0:?}")]
    UnknownCommand(Command),
    #[error("Parsed unknown response: {0:?}")]
    UnknownResponse(Response),
    #[error("Parsed arguments do not match argument definition: {0:?}")]
    UnknownArguments(Arguments),
}

impl<RT: pest::RuleType> From<ParseError<RT>> for DeserializeError {
    fn from(parse_error: ParseError<RT>) -> DeserializeError {
        let string = format!("{:?}", parse_error);
        DeserializeError::ParseError(string)
    }
}

impl From<io::Error> for DeserializeError {
    fn from(io_error: io::Error) -> DeserializeError {
        DeserializeError::IoError(io_error)
    }
}

#[derive(Debug, Error)]
pub enum SerializeError {
    #[error("IO error: {0:?}")]
    IoError(io::Error),
}

impl From<io::Error> for SerializeError {
    fn from(io_error: io::Error) -> SerializeError {
        SerializeError::IoError(io_error)
    }
}

/// Trait implemented for the generated high level AT command and response types
/// to convert back and forth between those types and byte streams.
pub trait SerDe {
    fn deserialize<R: io::Read>(source: &mut R) -> Result<Self, DeserializeError>
    where
        Self: Sized;
    fn serialize<W: io::Write>(&self, sink: &mut W) -> Result<(), SerializeError>;
}

impl SerDe for generated::types::Command {
    fn deserialize<R: io::Read>(source: &mut R) -> Result<Self, DeserializeError> {
        //TODO(fxb/66041) Remove this intermediate String and parse directly from the Read.
        let mut string: String = String::new();
        source.read_to_string(&mut string)?;
        let lowlevel = command_parser::parse(&string)?;
        let highlevel = generated::translate::raise_command(&lowlevel)?;

        Ok(highlevel)
    }

    fn serialize<W: io::Write>(&self, sink: &mut W) -> Result<(), SerializeError> {
        let lowlevel = generated::translate::lower_command(self);
        lowlevel.write_to(sink)?;

        Ok(())
    }
}

impl SerDe for generated::types::Response {
    fn deserialize<R: io::Read>(source: &mut R) -> Result<Self, DeserializeError> {
        //TODO(fxb/66041) Remove this intermediate String and parse directly from the Read.
        let mut string: String = String::new();
        source.read_to_string(&mut string)?;
        let lowlevel = response_parser::parse(&string)?;
        let highlevel = generated::translate::raise_response(&lowlevel)?;

        Ok(highlevel)
    }

    fn serialize<W: io::Write>(&self, sink: &mut W) -> Result<(), SerializeError> {
        let lowlevel = generated::translate::lower_response(self);
        lowlevel.write_to(sink)?;

        Ok(())
    }
}
