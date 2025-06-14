/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/listener_manager_impl.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

namespace libp2p::network {

  namespace {
    log::Logger log() {
      static log::Logger logger = log::createLogger("ListenerManager");
      return logger;
    }
  }  // namespace

  ListenerManagerImpl::ListenerManagerImpl(
      std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<TransportManager> tmgr,
      std::shared_ptr<ConnectionManager> cmgr)
      : multiselect_(std::move(multiselect)),
        router_(std::move(router)),
        tmgr_(std::move(tmgr)),
        cmgr_(std::move(cmgr)) {
    BOOST_ASSERT(multiselect_ != nullptr);
    BOOST_ASSERT(router_ != nullptr);
    BOOST_ASSERT(tmgr_ != nullptr);
    BOOST_ASSERT(cmgr_ != nullptr);
  }

  bool ListenerManagerImpl::isStarted() const {
    return started;
  }

  outcome::result<void> ListenerManagerImpl::closeListener(
      const multi::Multiaddress &ma) {
    // we can find multiaddress directly
    auto it = listeners_.find(ma);
    if (it != listeners_.end()) {
      auto listener = it->second;
      listeners_.erase(it);
      if (!listener->isClosed()) {
        return listener->close();
      }

      return outcome::success();
    }

    // we did not find direct multiaddress
    // lets try to search across interface addresses
    for (auto &&entry : listeners_) {
      auto r = entry.second->getListenMultiaddr();
      if (!r) {
        // ignore error
        continue;
      }

      auto &&addr = r.value();
      if (addr == ma) {
        // found. close listener.
        auto listener = entry.second;
        listeners_.erase(it);
        if (!listener->isClosed()) {
          return listener->close();
        }

        return outcome::success();
      }
    }

    return std::errc::invalid_argument;
  }

  outcome::result<void> ListenerManagerImpl::removeListener(
      const multi::Multiaddress &ma) {
    auto it = listeners_.find(ma);
    if (it != listeners_.end()) {
      listeners_.erase(it);
      return outcome::success();
    }

    return std::errc::invalid_argument;
  };

  // starts listening on all provided multiaddresses
  void ListenerManagerImpl::start() {
    if (started) {
      return;
    }

    auto begin = listeners_.begin();
    auto end = listeners_.end();
    for (auto it = begin; it != end;) {
      auto r = it->second->listen(it->first);
      if (!r) {
        // can not start listening on this multiaddr, remove listener
        it = listeners_.erase(it);
      } else {
        ++it;
      }
    }

    started = true;
  }

  // stops listening on all multiaddresses
  void ListenerManagerImpl::stop() {
    if (!started) {
      return;
    }

    auto begin = listeners_.begin();
    auto end = listeners_.end();
    for (auto it = begin; it != end;) {
      auto r = it->second->close();
      if (!r) {
        // error while stopping listener, remove it
        it = listeners_.erase(it);
      } else {
        ++it;
      }
    }

    started = false;
  }

  outcome::result<void> ListenerManagerImpl::listen(
      const multi::Multiaddress &ma) {
    auto tr = this->tmgr_->findBest(ma);
    if (tr == nullptr) {
      // can not listen on this address
      return std::errc::address_family_not_supported;
    }

    auto it = listeners_.find(ma);
    if (it != listeners_.end()) {
      // this address is already used
      return std::errc::address_in_use;
    }

    auto listener = tr->createListener(
        [this](auto &&r) { this->onConnection(std::forward<decltype(r)>(r)); });

    listeners_.insert({ma, std::move(listener)});

    return outcome::success();
  }

  std::vector<multi::Multiaddress> ListenerManagerImpl::getListenAddresses()
      const {
    std::vector<multi::Multiaddress> mas;
    mas.reserve(listeners_.size());

    for (auto &&e : listeners_) {
      mas.push_back(e.first);
    }

    return mas;
  }

  std::vector<multi::Multiaddress>
  ListenerManagerImpl::getListenAddressesInterfaces() const {
    std::vector<multi::Multiaddress> mas;
    mas.reserve(listeners_.size());

    for (auto &&e : listeners_) {
      auto addr = e.second->getListenMultiaddr();
      // ignore failed sockets
      if (addr) {
        mas.push_back(std::move(addr.value()));
      }
    }

    return mas;
  }

  void ListenerManagerImpl::onConnection(
      outcome::result<std::shared_ptr<connection::CapableConnection>> rconn) {
    if (!rconn) {
      log()->warn("can not accept valid connection, {}", rconn.error());
      return;  // ignore
    }
    auto &&conn = rconn.value();

    auto rid = conn->remotePeer();
    if (!rid) {
      log()->warn("can not get remote peer id, {}", rid.error());
      return;  // ignore
    }
    auto &&id = rid.value();

    // set onStream handler function
    conn->onStream(
        [this](outcome::result<std::shared_ptr<connection::Stream>> rstream) {
          if (!rstream) {
            log()->warn("can not accept stream, {}", rstream.error());
            return;  // ignore
          }
          auto &&stream = rstream.value();

          auto protocols = this->router_->getSupportedProtocols();
          if (protocols.empty()) {
            log()->warn("no protocols are served, resetting inbound stream");
            stream->reset();
            return;
          }

          // negotiate protocols
          this->multiselect_->selectOneOf(
              this->router_->getSupportedProtocols(),
              stream,
              false /* not initiator */,
              true /* need to negotiate multistream itself - SPEC ???*/,
              [this, stream](outcome::result<peer::ProtocolName> rproto) {
                bool success = true;

                if (!rproto) {
                  log()->warn("can not negotiate protocols, {}",
                              rproto.error());
                  success = false;
                } else {
                  auto &&proto = rproto.value();

                  auto rhandle = this->router_->handle(proto, stream);
                  if (!rhandle) {
                    log()->warn("no protocol handler found, {}",
                                rhandle.error());
                    success = false;
                  }
                }

                if (!success) {
                  stream->reset();
                }
              });
        });

    // store connection
    if (auto yamux_conn = std::dynamic_pointer_cast<connection::YamuxedConnection>(conn)) {
      yamux_conn->markAsRegistered();
    }
    this->cmgr_->addConnectionToPeer(id, conn);
  }

  Router &ListenerManagerImpl::getRouter() {
    return *router_;
  }

}  // namespace libp2p::network
