// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "garnet/bin/mdns/service/mdns.h"
#include "garnet/bin/media/util/fidl_publisher.h"
#include "lib/app/cpp/application_context.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fxl/macros.h"
#include "lib/mdns/fidl/mdns.fidl.h"

namespace mdns {

class MdnsServiceImpl : public MdnsService {
 public:
  MdnsServiceImpl(component::ApplicationContext* application_context);

  ~MdnsServiceImpl() override;

  // MdnsService implementation.
  void ResolveHostName(const f1dl::String& host_name,
                       uint32_t timeout_ms,
                       const ResolveHostNameCallback& callback) override;

  void SubscribeToService(const f1dl::String& service_name,
                          f1dl::InterfaceRequest<MdnsServiceSubscription>
                              subscription_request) override;

  void PublishServiceInstance(
      const f1dl::String& service_name,
      const f1dl::String& instance_name,
      uint16_t port,
      f1dl::Array<f1dl::String> text,
      const PublishServiceInstanceCallback& callback) override;

  void UnpublishServiceInstance(const f1dl::String& service_name,
                                const f1dl::String& instance_name) override;

  void AddResponder(
      const f1dl::String& service_name,
      const f1dl::String& instance_name,
      f1dl::InterfaceHandle<MdnsResponder> responder_handle) override;

  void SetSubtypes(const f1dl::String& service_name,
                   const f1dl::String& instance_name,
                   f1dl::Array<f1dl::String> subtypes) override;

  void ReannounceInstance(const f1dl::String& service_name,
                          const f1dl::String& instance_name) override;

  void SetVerbose(bool value) override;

 private:
  class Subscriber : public Mdns::Subscriber, public MdnsServiceSubscription {
   public:
    Subscriber(f1dl::InterfaceRequest<MdnsServiceSubscription> request,
               const fxl::Closure& deleter);

    ~Subscriber() override;

    // Mdns::Subscriber implementation:
    void InstanceDiscovered(const std::string& service,
                            const std::string& instance,
                            const SocketAddress& v4_address,
                            const SocketAddress& v6_address,
                            const std::vector<std::string>& text) override;

    void InstanceChanged(const std::string& service,
                         const std::string& instance,
                         const SocketAddress& v4_address,
                         const SocketAddress& v6_address,
                         const std::vector<std::string>& text) override;

    void InstanceLost(const std::string& service,
                      const std::string& instance) override;

    void UpdatesComplete() override;

    // MdnsServiceSubscription implementation.
    void GetInstances(uint64_t version_last_seen,
                      const GetInstancesCallback& callback) override;

   private:
    f1dl::Binding<MdnsServiceSubscription> binding_;
    media::FidlPublisher<GetInstancesCallback> instances_publisher_;
    std::unordered_map<std::string, MdnsServiceInstancePtr> instances_by_name_;

    FXL_DISALLOW_COPY_AND_ASSIGN(Subscriber);
  };

  // Publisher for PublishServiceInstance.
  class SimplePublisher : public Mdns::Publisher {
   public:
    SimplePublisher(IpPort port,
                    f1dl::Array<f1dl::String> text,
                    const PublishServiceInstanceCallback& callback);

   private:
    // Mdns::Publisher implementation.
    void ReportSuccess(bool success) override;

    void GetPublication(
        bool query,
        const std::string& subtype,
        const std::function<void(std::unique_ptr<Mdns::Publication>)>& callback)
        override;

    IpPort port_;
    std::vector<std::string> text_;
    PublishServiceInstanceCallback callback_;

    FXL_DISALLOW_COPY_AND_ASSIGN(SimplePublisher);
  };

  // Publisher for AddResponder.
  class ResponderPublisher : public Mdns::Publisher {
   public:
    ResponderPublisher(MdnsResponderPtr responder, const fxl::Closure& deleter);

    // Mdns::Publisher implementation.
    void ReportSuccess(bool success) override;

    void GetPublication(
        bool query,
        const std::string& subtype,
        const std::function<void(std::unique_ptr<Mdns::Publication>)>& callback)
        override;

    MdnsResponderPtr responder_;

    FXL_DISALLOW_COPY_AND_ASSIGN(ResponderPublisher);
  };

  // Starts the service.
  void Start();

  component::ApplicationContext* application_context_;
  f1dl::BindingSet<MdnsService> bindings_;
  mdns::Mdns mdns_;
  size_t next_subscriber_id_ = 0;
  std::unordered_map<size_t, std::unique_ptr<Subscriber>> subscribers_by_id_;
  std::unordered_map<std::string, std::unique_ptr<Mdns::Publisher>>
      publishers_by_instance_full_name_;

  FXL_DISALLOW_COPY_AND_ASSIGN(MdnsServiceImpl);
};

}  // namespace mdns
