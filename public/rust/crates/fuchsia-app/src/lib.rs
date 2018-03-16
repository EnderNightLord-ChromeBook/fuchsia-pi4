//! Connect to or provide Fuchsia services.

// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#![deny(warnings)]
#![deny(missing_docs)]

extern crate fuchsia_async as async;
extern crate fuchsia_zircon as zx;
extern crate mxruntime;
extern crate fdio;
#[macro_use] extern crate failure;
extern crate fidl;
extern crate futures;

// Generated FIDL bindings
extern crate garnet_public_lib_app_fidl;

use garnet_public_lib_app_fidl::{
    ApplicationController,
    ApplicationLauncher,
    ApplicationLaunchInfo,
};
use fidl::FidlService;

#[allow(unused_imports)]
use failure::{Error, ResultExt, Fail};
use futures::prelude::*;
use futures::stream::FuturesUnordered;

/// Tools for starting or connecting to existing Fuchsia applications and services.
pub mod client {
    use super::*;

    #[inline]
    /// Connect to a FIDL service using the application root namespace.
    pub fn connect_to_service<Service: FidlService>()
        -> Result<Service::Proxy, Error>
    {
        let (proxy, server)  = Service::new_pair()?;

        let service_path = format!("/svc/{}", Service::NAME);
        fdio::service_connect(&service_path, server.into_channel())?;

        Ok(proxy)
    }

    /// Launcher launches Fuchsia applications.
    pub struct Launcher {
        app_launcher: ApplicationLauncher::Proxy,
    }

    impl Launcher {
        #[inline]
        /// Create a new application launcher.
        pub fn new() -> Result<Self, Error> {
            let app_launcher = connect_to_service::<ApplicationLauncher::Service>()?;
            Ok(Launcher { app_launcher })
        }

        /// Launch an application at the specified URL.
        pub fn launch(
            &self,
            url: String,
            arguments: Option<Vec<String>>,
        ) -> Result<App, Error>
        {

            let (app_controller, controller_server_end) = ApplicationController::Service::new_pair()?;
            let (directory_request, directory_server_chan) = zx::Channel::create()?;

            let launch_info = ApplicationLaunchInfo {
                url,
                arguments,
                out: None,
                err: None,
                directory_request: Some(directory_server_chan),
                flat_namespace: None,
                additional_services: None,
            };


            self.app_launcher
                .create_application(launch_info, Some(controller_server_end))
                .context("Failed to start a new Fuchsia application.")?;

            Ok(App { directory_request, app_controller })
        }
    }

    /// `App` represents a launched application.
    pub struct App {
        // directory_request is a directory protocol channel
        directory_request: zx::Channel,

        // TODO: use somehow?
        #[allow(dead_code)]
        app_controller: ApplicationController::Proxy,
    }

    impl App {
        #[inline]
        /// Connect to a service provided by the `App`.
        pub fn connect_to_service<Service: FidlService>(&self)
            -> Result<Service::Proxy, Error>
        {
            let (client_channel, server_channel) = zx::Channel::create()?;
            fdio::service_connect_at(&self.directory_request, Service::NAME, server_channel)?;
            Ok(Service::new_proxy(fidl::ClientEnd::new(client_channel))?)
        }

        /// Connect `channel` to a service called `service_name` provided by the `App`.
        #[inline]
        pub fn connect_to_service_raw(&self, channel: zx::Channel, service_name: &str)
            -> Result<(), Error>
        {
            fdio::service_connect_at(&self.directory_request, service_name, channel)?;
            Ok(())
        }
    }
}

/// Tools for providing Fuchsia services.
pub mod server {
    use super::*;
    use futures::{Future, Poll};

    use self::errors::*;
    /// New root-level errors that may occur when using the `fuchsia_component::server` module.
    /// Note that these are not the only kinds of errors that may occur: errors the module
    /// may also be caused by `fidl::Error` or `zircon::Status`.
    pub mod errors {
        /// The startup handle on which the FIDL server attempted to start was missing.
        #[derive(Debug, Fail)]
        #[fail(display = "The startup handle on which the FIDL server attempted to start was missing.")]
        pub struct MissingStartupHandle;
    }

    /// A heterogeneous list.
    pub struct HCons<Head, Tail> {
        head: Head,
        tail: Tail,
    }

    /// The "empty" tail of a heterogeneous list.
    pub struct HNil;

    /// `ServiceFactory` lazily creates instances of services.
    ///
    /// Note that this trait is implemented by `FnMut` closures like `|| MyService { ... }`.
    pub trait ServiceFactory {

        /// The concrete type of the `fidl::Stub` service created by this `ServiceFactory`.
        type Stub: fidl::Stub + 'static;

        /// Create a `fidl::Stub` service.
        // TODO(cramertj): allow `create` calls to fail.
        fn create(&mut self) -> Self::Stub;
    }

    impl<F, S> ServiceFactory for F
        where F: FnMut() -> S,
            S: fidl::Stub + 'static
    {
        type Stub = S;

        #[inline]
        fn create(&mut self) -> Self::Stub {
            (self)()
        }
    }

    /// A collection of `ServiceFactory`s.
    pub trait ServiceFactories {
        /// Spawn a service of type `service_name` on `channel`.
        fn spawn_service(&mut self, service_name: String, channel: async::Channel);
    }

    impl ServiceFactories for HNil {
        #[inline]
        fn spawn_service(&mut self, service_name: String, _: async::Channel) {
            // TODO: proper logging
            eprintln!("No service found with name \"{}\"", service_name);
        }
    }

    impl<Factory, Tail> ServiceFactories for HCons<Factory, Tail>
        where Factory: ServiceFactory,
            Tail: ServiceFactories
    {
        #[inline]
        fn spawn_service(&mut self, service_name: String, channel: async::Channel) {
            if service_name == <Factory::Stub as fidl::Stub>::Service::NAME {
                match fidl::Server::new(self.head.create(), channel) {
                    Ok(server) => {
                        async::spawn(
                            server.recover(|e|
                                // TODO: proper logging
                                eprintln!("Error running server: {:?}", e)
                            ));
                    }
                    Err(e) => {
                        // TODO: proper logging
                        eprintln!("Error starting service \"{}\": {:?}", service_name, e);
                    }
                }
            } else {
                self.tail.spawn_service(service_name, channel);
            }
        }
    }

    /// `ServicesServer` is a server which manufactures service instances of varying types on demand.
    /// To run a `ServicesServer`, use `Server::new`.
    pub struct ServicesServer<Services: ServiceFactories> {
        services: Services,
    }

    impl ServicesServer<HNil> {
        /// Create a new `ServicesServer` which doesn't provide any services.
        pub fn new() -> Self {
            ServicesServer {
                services: HNil,
            }
        }
        /// Spawn a service instance
        pub fn spawn_service(&mut self, service_name: String, channel: async::Channel) {
            self.services.spawn_service(service_name, channel)
        }
    }

    impl<Services: ServiceFactories> ServicesServer<Services> {
        /// Create a new `ServicesServer` with an existing `ServiceFactories`.
        pub fn new_with_factories(services: Services) -> Self {
            ServicesServer { services, }
        }

        /// Add a service to the `ServicesServer`.
        pub fn add_service<S: ServiceFactory>(self, service_factory: S) -> ServicesServer<HCons<S, Services>> {
            ServicesServer {
                services: HCons {
                    head: service_factory,
                    tail: self.services,
                }
            }
        }

        /// Start serving directory protocol service requests on the process PA_DIRECTORY_REQUEST handle
        pub fn start(self) -> Result<FdioServer<Services>, Error> {
            let fdio_handle = mxruntime::get_startup_handle(mxruntime::HandleType::DirectoryRequest)
                .ok_or(MissingStartupHandle)?;

            let fdio_channel = async::Channel::from_channel(fdio_handle.into())?;

            let mut server = FdioServer{
                readers: FuturesUnordered::new(),
                factories: self.services,
            };

            server.serve_channel(fdio_channel);

            Ok(server)
        }
    }

    /// `FdioServer` is a very basic vfs directory server that only responds to
    /// OPEN and CLONE messages. OPEN always connects the client channel to a
    /// newly spawned fidl service produced by the factory F.
    #[must_use = "futures must be polled"]
    pub struct FdioServer<F: ServiceFactories + 'static> {
        readers: FuturesUnordered<async::RecvMsg<zx::MessageBuf>>,
        factories: F,
    }

    impl<F: ServiceFactories + 'static> FdioServer<F> {
        fn dispatch(&mut self, chan: &async::Channel, buf: zx::MessageBuf) -> zx::MessageBuf {
            // TODO(raggi): provide an alternative to the into() here so that we
            // don't need to pass the buf in owned back and forward.
            let mut msg: fdio::rio::Message = buf.into();

            // open & clone use a different reply channel
            //
            // Note: msg.validate() ensures that open must have exactly one
            // handle, but the message may yet be invalid.
            let reply_channel = match msg.op() {
                fdio::fdio_sys::ZXRIO_OPEN |
                fdio::fdio_sys::ZXRIO_CLONE => {
                    msg.take_handle(0).map(zx::Channel::from)
                }
                _ => None,
            };

            let validation = msg.validate();
            if validation.is_err() ||
                (
                    msg.op() != fdio::fdio_sys::ZXRIO_OPEN &&
                    msg.op() != fdio::fdio_sys::ZXRIO_CLONE
                ) ||
                msg.is_describe() ||
                !reply_channel.is_some()
            {
                eprintln!(
                    "service request channel received invalid/unsupported zxrio request: {:?}",
                    &msg
                );

                if msg.is_describe() {
                    let reply_channel = reply_channel.as_ref().unwrap_or(chan.as_ref());
                    let reply_err = validation.err().unwrap_or(zx::Status::NOT_SUPPORTED);
                    fdio::rio::write_object(reply_channel, reply_err, 0, &[], &mut vec![])
                        .unwrap_or_else(|e| {
                            eprintln!("service request reply write failed with {:?}", e)
                        });
                }

                return msg.into();
            }

            if msg.op() == fdio::fdio_sys::ZXRIO_CLONE {
                if let Some(c) = reply_channel {
                    if let Ok(fdio_chan) = async::Channel::from_channel(c) {
                        self.serve_channel(fdio_chan);
                    }
                }
                return msg.into();
            }

            let service_channel = reply_channel.unwrap();
            let service_channel = async::Channel::from_channel(service_channel).unwrap();

            // TODO(raggi): re-arrange things to avoid the copy here
            let path = std::str::from_utf8(msg.data()).unwrap().to_owned();

            println!(
                "service request channel received open request for path: {:?}",
                &path
            );

            self.factories.spawn_service(path, service_channel);
            msg.into()
        }

        fn serve_channel(&mut self, chan: async::Channel) {
            let rmsg = chan.recv_msg(zx::MessageBuf::new());
            self.readers.push(rmsg);
        }

    }

    impl<F: ServiceFactories + 'static> Future for FdioServer<F> {
        type Item = ();
        type Error = Error;

        fn poll(&mut self, cx: &mut task::Context) -> Poll<Self::Item, Self::Error> {
            loop {
                match self.readers.poll_next(cx) {
                    Ok(Async::Ready(Some((chan, buf)))) => {
                        let buf = self.dispatch(&chan, buf);
                        self.readers.push(chan.recv_msg(buf));
                    },
                    Ok(Async::Ready(None)) | Ok(Async::Pending) => return Ok(Async::Pending),
                    Err(_) => {
                        // errors are ignored, as we assume that the channel should still be read from.
                    },
                }
            }
        }
    }
}
