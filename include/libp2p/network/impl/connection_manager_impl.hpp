/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>
#include <mutex>

#include <libp2p/event/bus.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/transport_manager.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::network {

  class ConnectionManagerImpl : public ConnectionManager {
   public:
    explicit ConnectionManagerImpl(std::shared_ptr<libp2p::event::Bus> bus);

    std::vector<ConnectionSPtr> getConnections() const override;

    std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const override;

    ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const override;

    void addConnectionToPeer(const peer::PeerId &p, ConnectionSPtr c) override;

    void collectGarbage() override;

    void closeConnectionsToPeer(const peer::PeerId &p) override;

    void onConnectionClosed(
        const peer::PeerId &peer_id,
        const std::shared_ptr<connection::CapableConnection> &conn) override;

   private:
    std::unordered_map<peer::PeerId, std::unordered_set<ConnectionSPtr>>
        connections_;

    std::shared_ptr<libp2p::event::Bus> bus_;

    /// Reentrancy resolver between closeConnectionsToPeer and
    /// onConnectionClosed
    boost::optional<peer::PeerId> closing_connections_to_peer_;
    
    /// Mutex to protect connection_is_closing_ set
    std::mutex connection_mutex_;
    
    /// Set of connections currently being closed to prevent double closing
    std::unordered_set<ConnectionSPtr> connection_is_closing_;
  };

}  // namespace libp2p::network
