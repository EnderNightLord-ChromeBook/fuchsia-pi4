// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/ledger/src/cloud_sync/impl/user_sync_impl.h"

#include "apps/ledger/src/cloud_sync/impl/ledger_sync_impl.h"
#include "lib/ftl/logging.h"

namespace cloud_sync {

UserSyncImpl::UserSyncImpl(ledger::Environment* environment,
                           UserConfig user_config)
    : environment_(environment), user_config_(std::move(user_config)) {}

UserSyncImpl::~UserSyncImpl() {
  FTL_DCHECK(active_ledger_syncs_.empty());
}

const UserConfig& UserSyncImpl::GetUserConfig() {
  return user_config_;
}

std::unique_ptr<LedgerSync> UserSyncImpl::CreateLedgerSync(
    ftl::StringView app_id) {
  if (!user_config_.use_sync) {
    return nullptr;
  }

  auto result =
      std::make_unique<LedgerSyncImpl>(environment_, &user_config_, app_id);
  result->set_on_delete([ this, ledger_sync = result.get() ]() {
    active_ledger_syncs_.erase(ledger_sync);
  });
  active_ledger_syncs_.insert(result.get());
  return result;
}

}  // namespace cloud_sync
