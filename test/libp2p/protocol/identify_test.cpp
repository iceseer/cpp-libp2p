/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/identify.hpp>
#include <libp2p/protocol/ping.hpp>

#include <vector>

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

#include <generated/protocol/identify/protobuf/block_request.pb.h>
//#include <generated/protocol/identify/protobuf/>
//#include <>

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
  identify->onIdentifyReceived([](auto const &peer_id) {
          });


  //auto ping = injector.create<std::shared_ptr<libp2p::protocol::Ping>>();

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, &identify] {  // NOLINT
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

/*    host->newStream(peer_info, "/ipfs/id/1.0.0",
        [&identify](auto &&stream_res) {
          identify->handle(std::move(stream_res));
        });*/

    auto ma =
        libp2p::multi::Multiaddress::create("/ip4/127.0.0.1/tcp/40010").value();
    auto listen_res = host->listen(ma);
    if (!listen_res) {
      std::cerr << "host cannot listen the given multiaddress: "
                << listen_res.error().message() << "\n";
      std::exit(EXIT_FAILURE);
    }

    host->setProtocolHandler(
        "/sup/transactions/1",
        [](auto &&stream) {
          //auto read_writer = std::make_shared<MessageReadWriter>(stream);
/*          if (!read_res) {
            return cb(read_res.error());
          }

          auto msg_res = scale::decode<MsgType>(*read_res.value());
          if (!msg_res) {
            return cb(msg_res.error());
          }

          return cb(std::move(msg_res.value()));*/
          int p = 0; ++p;

        });

    identify->start();
    host->start();

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
          44, 8, 128, 128, 128, 152, 1, 18, 32, 52,
          189, 238, 44, 52, 228, 153, 78, 3, 11,
          253, 252, 168, 165, 91, 172, 110, 34,
          30, 172, 203, 223, 102, 173, 232, 127,
          77, 55, 193, 186, 63, 222, 40, 1, 48, 1
      };
      auto stream_p = std::move(stream_res.value());
      ASSERT_FALSE(stream_p->isClosedForWrite());

      stream_p->write(
          request_buf,
          request_buf.size(),
          [request_buf, stream_p](auto &&write_res) {
            /*std::vector<uint8_t> read_buf{};
            read_buf.resize(10);
            stream_p->read(read_buf, 10, [read_buf, stream_p](auto &&read_res) {
              FAIL() << "Read res: " << read_res.error().message();
              ASSERT_TRUE(read_res) << read_res.error().message();
            });*/

            stream_p->close([stream_p{std::move(stream_p)}] (auto res) {
              std::vector<uint8_t> read_buf{};
              read_buf.resize(10);
              stream_p->read(read_buf, 10, [read_buf, stream_p](auto &&read_res) {
                FAIL() << "Read res: " << read_res.error().message();
                ASSERT_TRUE(read_res) << read_res.error().message();
              });
            });
          });

      //proto2::Message::
      /*auto msg = std::make_shared<api::v1::BlockRequest>();
      msg->set_fields(19);
      uint8_t s = 5;
      msg->set_number(&s, 1);
      msg->set_direction(api::v1::Direction::Descending);
      msg->set_max_blocks(6);*/

      //MessageReadWriterBigEndian
      //auto rw = std::make_shared<libp2p::basic::ProtobufMessageReadWriter>(std::make_shared<libp2p::basic::MessageReadWriterUvarint>(stream_p));
      /*auto rw = std::make_shared<libp2p::basic::ProtobufMessageReadWriter>(std::make_shared<libp2p::basic::MessageReadWriterUvarint>(stream_p));
      rw->write(*msg, [stream_p{std::move(stream_p)}](libp2p::outcome::result<size_t> res) {
        auto q = res.value();
        std::cerr << "Written: " << q << std::endl;
        //FAIL() << "Write res: " << res.error().message();
        ASSERT_TRUE(res) << res.error().message();

        stream_p->close([stream_p{std::move(stream_p)}] (auto res) {
          ASSERT_TRUE(res) << res.error().message();
          ASSERT_TRUE(stream_p->isClosedForWrite());
          ASSERT_FALSE(stream_p->isClosedForRead());
          //FAIL() << "Close res: " << res.error().message();

          libp2p::basic::ProtobufMessageReadWriter::ReadCallbackFunc<api::v1::BlockResponse> f =
              [](libp2p::outcome::result<api::v1::BlockResponse> res) {
                auto q = res.value();
                ASSERT_TRUE(res) << res.error().message();
              };

          auto rw = std::make_shared<libp2p::basic::ProtobufMessageReadWriter>(stream_p);
          rw->read(std::move(f));
        });
      });*/

      //{8, 19, 26, 1, 5, 40, 1, 48, 1}
      //{9, 8, 19, 26, 1, 5, 40, 1, 48, 1}
      //libp2p::outcome::result<api::v1::BlockResponse> res) {


      //identify::pb::Identify id;
      //rw->write()

/*      std::vector<uint8_t> read_buf{};
      read_buf.resize(10);
      stream_p->read(read_buf, 5, [read_buf, stream_p](auto &&read_res) {
        FAIL() << "Read res: " << read_res.error().message();

        ASSERT_TRUE(read_res) << read_res.error().message();
        FAIL() << "!!!!!!!!!!!!!!!!!!!";
      });*/

      //strcpy((char*)&request_buf[0], "Hello!!!!!");
      /*stream_p->write(
          request_buf,
          request_buf.size(),
          [request_buf, stream_p](auto &&write_res) {
            ASSERT_TRUE(write_res) << write_res.error().message();*/
/*            std::vector<uint8_t> read_buf{};
            read_buf.resize(10);
            stream_p->read(read_buf, 10, [read_buf, stream_p](auto &&read_res) {
              FAIL() << "Read res: " << read_res.error().message();

              ASSERT_TRUE(read_res) << read_res.error().message();
              FAIL() << common::Buffer(read_buf).toHex();
            });*/

            //ASSERT_TRUE(write_res) << write_res.error().message();
            //ASSERT_EQ(request_buf.size(), write_res.value());
          //});
    });
  });

  try {
    //ping->start();
    context->run();
  } catch (std::exception &e) {
    std::cout << e.what();
  }

  int p =0; ++p;
}

