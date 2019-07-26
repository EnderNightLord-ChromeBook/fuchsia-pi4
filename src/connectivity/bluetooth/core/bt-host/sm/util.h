// Copyright 2018 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_SM_UTIL_H_
#define SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_SM_UTIL_H_

#include "src/connectivity/bluetooth/core/bt-host/common/device_address.h"
#include "src/connectivity/bluetooth/core/bt-host/common/uint128.h"
#include "src/connectivity/bluetooth/core/bt-host/hci/hci_constants.h"
#include "src/connectivity/bluetooth/core/bt-host/sm/smp.h"

namespace bt {
namespace sm {
namespace util {

// Returns a string representation of a given pairing method.
std::string PairingMethodToString(PairingMethod method);

// Returns a string representation of a given IOCapability.
std::string IOCapabilityToString(IOCapability capability);

// Returns the HCI version of an SMP IOCapability. Returns
// hci::IOCapability::kNoInputNoOutput for values not in sm::IOCapability.
hci::IOCapability IOCapabilityForHci(IOCapability capability);

// Used to select the key generation method as described in Vol 3, Part H,
// 2.3.5.1 based on local and peer authentication parameters:
//   - |secure_connections|: True if Secure Connections pairing is used. False
//     means Legacy Pairing.
//   - |local_oob|: Local OOB auth data is available.
//   - |peer_oob|: Peer OOB auth data is available.
//   - |mitm_required|: True means at least one of the devices requires MITM
//     protection.
//   - |local_ioc|, |peer_ioc|: Local and peer IO capabilities.
//   - |local_initiator|: True means that the local device is the initiator and
//     |local_ioc| represents the initiator's I/O capabilities.
PairingMethod SelectPairingMethod(bool secure_connections, bool local_oob, bool peer_oob,
                                  bool mitm_required, IOCapability local_ioc, IOCapability peer_ioc,
                                  bool local_initiator);

// Implements the "Security Function 'e'" defined in Vol 3, Part H, 2.2.1.
void Encrypt(const UInt128& key, const UInt128& plaintext_data, UInt128* out_encrypted_data);

// Implements the "Confirm Value Generation" or "c1" function for LE Legacy
// Pairing described in Vol 3, Part H, 2.2.3.
//
//   |tk|: 128-bit TK value
//   |rand|: 128-bit random number
//   |preq|: 56-bit SMP "Pairing Request" PDU
//   |pres|: 56-bit SMP "Pairing Response" PDU
//   |initiator_addr|: Device address of the initiator used while establishing
//                     the connection.
//   |responder_addr|: Device address of the responder used while establishing
//                     the connection.
//
// The generated confirm value will be returned in |out_confirm_value|.
void C1(const UInt128& tk, const UInt128& rand, const ByteBuffer& preq, const ByteBuffer& pres,
        const DeviceAddress& initiator_addr, const DeviceAddress& responder_addr,
        UInt128* out_confirm_value);

// Implements the "Key Generation Function s1" to generate the STK for LE Legacy
// Pairing described in Vol 3, Part H, 2.2.4.
//
//   |tk|: 128-bit TK value
//   |r1|: 128-bit random value generated by the responder.
//   |r2|: 128-bit random value generated by the initiator.
void S1(const UInt128& tk, const UInt128& r1, const UInt128& r2, UInt128* out_stk);

// Implements the "Random Address Hash Function ah" to resolve RPAs. Described
// in Vol 3, Part H, 222.
//
//   |k|: 128-bit IRK value
//   |r|: 24-bit random part of a RPA.
//
// Returns 24 bit hash value.
uint32_t Ah(const UInt128& k, uint32_t r);

// Returns true if the given |irk| can resolve the given |rpa| using the method
// described in Vol 6, Part B, 1.3.2.3.
bool IrkCanResolveRpa(const UInt128& irk, const DeviceAddress& rpa);

// Generates a RPA using the given IRK based on the method described in Vol 6,
// Part B, 1.3.2.2.
DeviceAddress GenerateRpa(const UInt128& irk);

// Generates a static or non-resolvable private random device address.
DeviceAddress GenerateRandomAddress(bool is_static);

}  // namespace util
}  // namespace sm
}  // namespace bt

#endif  // SRC_CONNECTIVITY_BLUETOOTH_CORE_BT_HOST_SM_UTIL_H_
