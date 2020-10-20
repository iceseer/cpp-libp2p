/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify.hpp>
#include <libp2p/protocol/ping.hpp>

#include <vector>
#include <chrono>
#include <thread>

#include <generated/protocol/identify/protobuf/identify.pb.h>

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/network.hpp>
#include <libp2p/basic/message_read_writer_uvarint.hpp>
#include <libp2p/basic/message_read_writer_bigendian.hpp>
#include <libp2p/basic/protobuf_message_read_writer.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/common/hexutil.hpp>

#include <generated/protocol/identify/protobuf/block_request.pb.h>
//#include <generated/protocol/identify/protobuf/>
//#include <>

class IdentifyTest : public testing::Test {
 public:
  void SetUp() override {
    // create a Protobuf message, which is to be "read" or written
  }

};

inline void tr(std::string_view prefix, std::shared_ptr<libp2p::connection::Stream> stream) {
  if (auto peer_res = stream->remotePeerId()) {
    std::cerr << prefix << ":" << std::endl
              << "\t" << peer_res.value().toHex() << std::endl
              << "\t"
              << "SUCCESS" << std::endl;
  } else {
    std::cerr << prefix << ":" << std::endl
              << "\t"
              << "-------" << std::endl
              << "\t"
              << "SUCCESS" << std::endl;
  }
}

inline void tr_error(std::string_view prefix, std::shared_ptr<libp2p::connection::Stream> stream, std::string_view error) {
  if (stream) {
    if (auto peer_res = stream->remotePeerId()) {
      std::cerr << prefix << ":" << std::endl
                << "\t" << peer_res.value().toHex() << std::endl
                << "\t" << error << std::endl;
      return;
    }
  }
  std::cerr << prefix << ":" << std::endl
            << "\t"
            << "-------" << std::endl
            << "\t" << error << std::endl;
}

enum {
  kReadCount = 1
};

TEST_F(IdentifyTest, RealConnect) {
  auto injector = libp2p::injector::makeHostInjector();
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  auto identify =
      injector.create<std::shared_ptr<libp2p::protocol::Identify>>();

  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, identify] {  // NOLINT
    auto server_ma_res = libp2p::multi::Multiaddress::create(
        //"/ip4/127.0.0.1/tcp/30333/p2p/12D3KooWNoMM7DGZZiEoeTYmcmFMW16Xr3dfs2tbjE7GJdXgeeSb");  // NOLINT
        "/ip4/127.0.0.1/tcp/30333/p2p/12D3KooWEyoppNCUx8Yx66oV9fJnriXwCcXwDDUA2kj6vnc6iDEp");
    if (!server_ma_res) {
      std::cerr << "unable to create server multiaddress: "
                << server_ma_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    std::string t{server_ma_res.value().getStringAddress()};
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      std::cerr << "unable to get peer id" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id_res =
        libp2p::peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      std::cerr << "Unable to decode peer id from base 58: "
                << server_peer_id_res.error().message() << std::endl;
      std::exit(EXIT_FAILURE);
    }

    auto server_peer_id = std::move(server_peer_id_res.value());
    auto peer_info = libp2p::peer::PeerInfo{server_peer_id, {server_ma}};

    host->getPeerRepository().getAddressRepository().addAddresses(
        server_peer_id,
        gsl::span<const libp2p::multi::Multiaddress>(&server_ma, 1),
        libp2p::peer::ttl::kPermanent);

    host->setProtocolHandler(identify->getProtocolId(),
                             [identify](auto &&stream) {
                               tr("handled identify", stream);
                               identify->handle(stream);
                             });

    host->setProtocolHandler("/sup/transactions/1", [](auto stream) {
      std::vector<uint8_t> buffer(kReadCount);
      if (!stream) {
        tr_error("handled sup/transactions", stream, "ERROR");
        return;
      } else {
        tr("handled sup/transactions", stream);
      }

      gsl::span<uint8_t> r(buffer);
      stream->read(
          r, kReadCount,
          [buffer{std::move(buffer)}, stream](auto result) mutable {
            if (!result) {
              tr_error("read/handled sup/transactions", stream,
                       result.error().message());
              return;
            } else {
              gsl::span<uint8_t> r(buffer.data(),
                                   gsl::span<uint8_t>::index_type(kReadCount));
              tr(fmt::format("read/handled sup/transactions {} bytes:\n\t{}",
                             result.value(), libp2p::common::hex_lower(r)),
                 stream);
            }
          });
    });

    identify->start();
    host->start();

    /*    host->newStream(
            peer_info, identify->getProtocolId(),
            [identify](auto &&stream_res) {
              identify->handle(stream_res);
            });*/

    host->newStream(peer_info, "/sup/transactions/1", [&](auto &&stream_res) {
      if (!stream_res) {
        tr_error("initiated sup/transactions", {},
                 stream_res.error().message());
        return;
      } else {
        tr("initiated sup/transactions", stream_res.value());
      }
    });

    /*    host->newStream(peer_info, "/sup/sync/2", [&](auto &&stream_res) {
          if (!stream_res) {
            FAIL() << "Cannot connect to server: " <<
       stream_res.error().message();
          }
          std::cerr << "Connected" << std::endl;
        });*/

    /*host->newStream(peer_info, "/sup/sync/2", [&](auto &&stream_res) {
      if (!stream_res) {
        FAIL() << "Cannot connect to server: " << stream_res.error().message();
      }
      std::cerr << "Connected" << std::endl;

      std::vector<uint8_t> request_buf = {
          44,  8,   128, 128, 128, 152, 1,   18,  32,  52,  189, 238,
          44,  52,  228, 153, 78,  3,   11,  253, 252, 168, 165, 91,
          172, 110, 34,  30,  172, 203, 223, 102, 173, 232, 127, 77,
          55,  193, 186, 63,  222, 40,  1,   48,  1};
      auto stream_p = std::move(stream_res.value());
      ASSERT_FALSE(stream_p->isClosedForWrite());

      stream_p->write(
          request_buf, request_buf.size(),
          [request_buf, stream_p](auto &&write_res) {
            stream_p->close([stream_p{std::move(stream_p)}](auto res) {
              std::vector<uint8_t> read_buf{};
              read_buf.resize(10);
              stream_p->read(
                  read_buf, 10, [read_buf, stream_p](auto &&read_res) {
                    FAIL() << "Read res: " << read_res.error().message();
                    ASSERT_TRUE(read_res) << read_res.error().message();
                  });
            });
          });
    });*/
  });

  try {
    context->run();
  } catch (std::exception &e) {
    std::cout << e.what();
  }

  std::this_thread::sleep_for(std::chrono::seconds(200));

  int p = 0;
  ++p;
}