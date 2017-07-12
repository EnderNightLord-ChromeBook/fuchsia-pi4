// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MODULAR_SRC_DEVICE_RUNNER_USER_CONTROLLER_IMPL_H_
#define APPS_MODULAR_SRC_DEVICE_RUNNER_USER_CONTROLLER_IMPL_H_

#include "application/lib/app/application_context.h"
#include "application/services/application_environment.fidl.h"
#include "apps/modular/lib/fidl/scope.h"
#include "apps/modular/services/auth/account/account.fidl.h"
#include "apps/modular/services/auth/account_provider.fidl.h"
#include "apps/modular/services/config/config.fidl.h"
#include "apps/modular/services/device/user_provider.fidl.h"
#include "apps/modular/services/user/user_runner.fidl.h"
#include "apps/mozart/services/views/view_token.fidl.h"
#include "lib/fidl/cpp/bindings/array.h"
#include "lib/fidl/cpp/bindings/binding.h"
#include "lib/fidl/cpp/bindings/interface_handle.h"
#include "lib/fidl/cpp/bindings/interface_ptr_set.h"
#include "lib/fidl/cpp/bindings/interface_request.h"
#include "lib/ftl/macros.h"

namespace modular {

// |UserControllerImpl| starts and manages a UserRunner. The life time of a
// UserRunner is bound to this class.  |UserControllerImpl| is not self-owned,
// but still drives its own deletion: On logout, it signals its
// owner (DeviceRunnerApp) to delete it.
class UserControllerImpl : UserController, UserContext {
 public:
  // After perfoming logout, to signal our completion (and deletion of our
  // instance) to our owner, we do it using a callback supplied to us in our
  // constructor. (The alternative is to take in a DeviceRunnerApp*, which seems
  // a little specific and overscoped).
  using DoneCallback = std::function<void(UserControllerImpl*)>;

  UserControllerImpl(
      std::shared_ptr<app::ApplicationContext> app_context,
      AppConfigPtr user_shell,
      const AppConfig& story_shell,
      fidl::InterfaceHandle<auth::TokenProviderFactory> token_provider_factory,
      auth::AccountPtr account,
      fidl::InterfaceRequest<mozart::ViewOwner> view_owner_request,
      fidl::InterfaceRequest<UserController> user_controller,
      DoneCallback done);

  // This will effectively tear down the entire instance by calling |done|.
  // |UserController|
  void Logout(const LogoutCallback& done) override;

 private:
  // |UserController|
  void Watch(fidl::InterfaceHandle<UserWatcher> watcher) override;

  // |UserContext|
  void Logout() override;

  std::unique_ptr<Scope> user_runner_scope_;
  app::ApplicationControllerPtr user_runner_controller_;
  UserRunnerPtr user_runner_;

  fidl::Binding<UserContext> user_context_binding_;
  fidl::Binding<UserController> user_controller_binding_;

  fidl::InterfacePtrSet<modular::UserWatcher> user_watchers_;

  std::vector<LogoutCallback> logout_response_callbacks_;
  DoneCallback done_;

  FTL_DISALLOW_COPY_AND_ASSIGN(UserControllerImpl);
};

}  // namespace modular

#endif  // APPS_MODULAR_SRC_DEVICE_RUNNER_USER_CONTROLLER_IMPL_H_
