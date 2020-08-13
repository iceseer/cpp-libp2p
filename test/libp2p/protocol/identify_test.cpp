/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify.hpp>

#include <vector>

#include <generated/protocol/identify/protobuf/identify.pb.h>
#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/network/network.hpp>
#include <libp2p/injector/host_injector.hpp>

class IdentifyTest : public testing::Test {
 public:
  void SetUp() override {
    // create a Protobuf message, which is to be "read" or written
  }

};

TEST_F(IdentifyTest, RealConnect) {
  auto injector = libp2p::injector::makeHostInjector();
  auto host = injector.create<std::shared_ptr<libp2p::Host>>();

  auto identify = injector.create<std::shared_ptr<libp2p::protocol::Identify>>();
  //identify->start();

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, &identify] {  // NOLINT
    auto server_ma_res = libp2p::multi::Multiaddress::create(
        "/ip4/127.0.0.1/tcp/30333/p2p/12D3KooWNoMM7DGZZiEoeTYmcmFMW16Xr3dfs2tbjE7GJdXgeeSb");  // NOLINT
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

    // libp2p::network::ConnectionManager cm;
    // auto manager = std::make_shared<libp2p::protocol::IdentifyMessageProcessor>(host, )

    // libp2p::protocol::Identify id()

    /*    host->newStream(peer_info, "/ipfs/id/1.0.0", [&](auto &&stream_res) {
          if (!stream_res)
            FAIL() << "Cannot connect to server: "
                   << stream_res.error().message();

          //stream_res.value() >> read_buf;
          //boost::outcome_v2::detail::basic_result_value_observers

          uint8_t s[1024*1024];
          gsl::span<uint8_t> out(s);

          std::shared_ptr<libp2p::connection::Stream> v = stream_res.value();
          v->read(out, 20, [](outcome::result<size_t> s) {
            size_t p = s.value();
            FAIL() << "Received: " << p;
          });


          int  p =0; ++p;
          //read_buf.resize(1024);
          //common::Buffer(read_buf).toHex();
        });*/
    /*    host->newStream(peer_info, "/ipfs/id/push/1.0.0", [&](auto
       &&stream_res) { if (!stream_res) FAIL() << "Cannot connect to server: "
                   << stream_res.error().message();

        });*/

    //targetPeerAddr, _ := ma.NewMultiaddr(fmt.Sprintf("/ipfs/%s", pid))
    //targetAddr := ipfsaddr.Decapsulate(targetPeerAddr)

    // We have a peer ID and a targetAddr so we add it to the peerstore
    // so LibP2P knows how to contact it
    //ha.Peerstore().AddAddr(peerid, targetAddr, peerstore.PermanentAddrTTL)

    host->getPeerRepository().getAddressRepository().addAddresses(
        server_peer_id, gsl::span<const libp2p::multi::Multiaddress>(&server_ma, 1), libp2p::peer::ttl::kPermanent);

    host->newStream(peer_info, "/ipfs/id/1.0.0",
        [&identify](auto &&stream_res) {
          identify->handle(std::move(stream_res));
        });


    host->newStream(peer_info, "/sup/sync/2", [&](auto &&stream_res) {
      if (!stream_res) {
        FAIL() << "Cannot connect to server: " << stream_res.error().message();
      }
      std::cerr << "Connected" << std::endl;

/*      auto request = prepareBlockRequest();
      network::GossipMessage message;
      message.type = network::GossipMessage::Type::BLOCK_REQUEST;
      //message.data = common::Buffer(scale::encode(request).value());
      message.data = common::Buffer(scale::encode(request).value());
      auto request_buf = scale::encode(message).value();*/

      std::vector<uint8_t> request_buf = {
          0x1, 0x64, 0x67, 0x45, 0x8b, 0x6b, 0x0, 0x0, 0x0, 0x0, 0x13, 0x1, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x5, 0x0, 0x0, 0x0
      };
      auto stream_p = std::move(stream_res.value());

      std::vector<uint8_t> read_buf{};
      read_buf.resize(10);
      stream_p->read(read_buf, 10, [read_buf, stream_p](auto &&read_res) {
        FAIL() << "Read res: " << read_res.error().message();

        ASSERT_TRUE(read_res) << read_res.error().message();
        FAIL() << "!!!!!!!!!!!!!!!!!!!";
      });

      //strcpy((char*)&request_buf[0], "Hello!!!!!");
      stream_p->write(
          request_buf,
          request_buf.size(),
          [request_buf, stream_p](auto &&write_res) {
            ASSERT_TRUE(write_res) << write_res.error().message();
/*            std::vector<uint8_t> read_buf{};
            read_buf.resize(10);
            stream_p->read(read_buf, 10, [read_buf, stream_p](auto &&read_res) {
              FAIL() << "Read res: " << read_res.error().message();

              ASSERT_TRUE(read_res) << read_res.error().message();
              FAIL() << common::Buffer(read_buf).toHex();
            });*/

            //ASSERT_TRUE(write_res) << write_res.error().message();
            //ASSERT_EQ(request_buf.size(), write_res.value());
          });
    });
  });

  try {
    context->run();
  } catch (std::exception &e) {
    std::cout << e.what();
  }
}

