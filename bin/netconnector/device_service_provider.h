// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>

#include "garnet/bin/netconnector/socket_address.h"
#include "lib/app/fidl/service_provider.fidl.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fxl/macros.h"

namespace netconnector {

class NetConnectorImpl;

// Provides services on a remote device.
class DeviceServiceProvider : public component::ServiceProvider {
 public:
  static std::unique_ptr<DeviceServiceProvider> Create(
      const std::string& device_name, const SocketAddress& address,
      f1dl::InterfaceRequest<component::ServiceProvider> request,
      NetConnectorImpl* owner);

  ~DeviceServiceProvider() override;

  void ConnectToService(const f1dl::StringPtr& service_name,
                        zx::channel channel) override;

 private:
  DeviceServiceProvider(
      const std::string& device_name, const SocketAddress& address,
      f1dl::InterfaceRequest<component::ServiceProvider> request,
      NetConnectorImpl* owner);

  std::string device_name_;
  SocketAddress address_;
  f1dl::Binding<component::ServiceProvider> binding_;
  NetConnectorImpl* owner_;

  FXL_DISALLOW_COPY_AND_ASSIGN(DeviceServiceProvider);
};

}  // namespace netconnector
