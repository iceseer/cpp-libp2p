/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUXED_CONNECTION_HPP
#define LIBP2P_YAMUXED_CONNECTION_HPP

#include <unordered_map>
#include <iostream>

#include <libp2p/basic/read_buffer.hpp>
#include <libp2p/basic/scheduler.hpp>
#include <libp2p/common/metrics/instance_count.hpp>
#include <libp2p/connection/capable_connection.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/muxer/yamux/yamux_reading_state.hpp>
#include <libp2p/muxer/yamux/yamux_stream.hpp>

namespace libp2p::connection {

  class YamuxedConnection;

  template<typename T>
  void storeKeeper(std::shared_ptr<T> const &ptr);
  template<typename T>
  void removeKeeper(std::shared_ptr<T> const &ptr);
}

namespace std {

  template<>
  class shared_ptr<libp2p::connection::YamuxedConnection> : public __shared_ptr<libp2p::connection::YamuxedConnection>
  {
    using _Tp = libp2p::connection::YamuxedConnection;

    template<typename... _Args>
    using _Constructible = typename enable_if<
        is_constructible<__shared_ptr<_Tp>, _Args...>::value
    >::type;

    template<typename _Arg>
    using _Assignable = typename enable_if<
        is_assignable<__shared_ptr<_Tp>&, _Arg>::value, shared_ptr&
    >::type;

   public:

    /// The type pointed to by the stored pointer, remove_extent_t<_Tp>
    using element_type = typename __shared_ptr<_Tp>::element_type;

#if __cplusplus >= 201703L
# define __cpp_lib_shared_ptr_weak_type 201606
    /// The corresponding weak_ptr type for this shared_ptr
    using weak_type = weak_ptr<_Tp>;
#endif
    /**
     *  @brief  Construct an empty %shared_ptr.
     *  @post   use_count()==0 && get()==0
     */
    constexpr shared_ptr() noexcept : __shared_ptr<_Tp>() {
      libp2p::connection::storeKeeper(*this);
    }

    shared_ptr(const shared_ptr& t) noexcept : __shared_ptr<_Tp>(t) {
      libp2p::connection::storeKeeper(*this);
    }

    ~shared_ptr() {
      libp2p::connection::removeKeeper(*this);
    }

    /**
     *  @brief  Construct a %shared_ptr that owns the pointer @a __p.
     *  @param  __p  A pointer that is convertible to element_type*.
     *  @post   use_count() == 1 && get() == __p
     *  @throw  std::bad_alloc, in which case @c delete @a __p is called.
     */
    template<typename _Yp, typename = _Constructible<_Yp*>>
    explicit
    shared_ptr(_Yp* __p) : __shared_ptr<_Tp>(__p) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Construct a %shared_ptr that owns the pointer @a __p
     *          and the deleter @a __d.
     *  @param  __p  A pointer.
     *  @param  __d  A deleter.
     *  @post   use_count() == 1 && get() == __p
     *  @throw  std::bad_alloc, in which case @a __d(__p) is called.
     *
     *  Requirements: _Deleter's copy constructor and destructor must
     *  not throw
     *
     *  __shared_ptr will release __p by calling __d(__p)
     */
    template<typename _Yp, typename _Deleter,
        typename = _Constructible<_Yp*, _Deleter>>
    shared_ptr(_Yp* __p, _Deleter __d)
        : __shared_ptr<_Tp>(__p, std::move(__d)) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Construct a %shared_ptr that owns a null pointer
     *          and the deleter @a __d.
     *  @param  __p  A null pointer constant.
     *  @param  __d  A deleter.
     *  @post   use_count() == 1 && get() == __p
     *  @throw  std::bad_alloc, in which case @a __d(__p) is called.
     *
     *  Requirements: _Deleter's copy constructor and destructor must
     *  not throw
     *
     *  The last owner will call __d(__p)
     */
    template<typename _Deleter>
    shared_ptr(nullptr_t __p, _Deleter __d)
        : __shared_ptr<_Tp>(__p, std::move(__d)) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Construct a %shared_ptr that owns the pointer @a __p
     *          and the deleter @a __d.
     *  @param  __p  A pointer.
     *  @param  __d  A deleter.
     *  @param  __a  An allocator.
     *  @post   use_count() == 1 && get() == __p
     *  @throw  std::bad_alloc, in which case @a __d(__p) is called.
     *
     *  Requirements: _Deleter's copy constructor and destructor must
     *  not throw _Alloc's copy constructor and destructor must not
     *  throw.
     *
     *  __shared_ptr will release __p by calling __d(__p)
     */
    template<typename _Yp, typename _Deleter, typename _Alloc,
        typename = _Constructible<_Yp*, _Deleter, _Alloc>>
    shared_ptr(_Yp* __p, _Deleter __d, _Alloc __a)
        : __shared_ptr<_Tp>(__p, std::move(__d), std::move(__a)) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Construct a %shared_ptr that owns a null pointer
     *          and the deleter @a __d.
     *  @param  __p  A null pointer constant.
     *  @param  __d  A deleter.
     *  @param  __a  An allocator.
     *  @post   use_count() == 1 && get() == __p
     *  @throw  std::bad_alloc, in which case @a __d(__p) is called.
     *
     *  Requirements: _Deleter's copy constructor and destructor must
     *  not throw _Alloc's copy constructor and destructor must not
     *  throw.
     *
     *  The last owner will call __d(__p)
     */
    template<typename _Deleter, typename _Alloc>
    shared_ptr(nullptr_t __p, _Deleter __d, _Alloc __a)
        : __shared_ptr<_Tp>(__p, std::move(__d), std::move(__a)) {
      libp2p::connection::storeKeeper(*this);
    }

    // Aliasing constructor

    /**
     *  @brief  Constructs a `shared_ptr` instance that stores `__p`
     *          and shares ownership with `__r`.
     *  @param  __r  A `shared_ptr`.
     *  @param  __p  A pointer that will remain valid while `*__r` is valid.
     *  @post   `get() == __p && use_count() == __r.use_count()`
     *
     *  This can be used to construct a `shared_ptr` to a sub-object
     *  of an object managed by an existing `shared_ptr`. The complete
     *  object will remain valid while any `shared_ptr` owns it, even
     *  if they don't store a pointer to the complete object.
     *
     * @code
     * shared_ptr<pair<int,int>> pii(new pair<int,int>());
     * shared_ptr<int> pi(pii, &pii->first);
     * assert(pii.use_count() == 2);
     * @endcode
     */
    template<typename _Yp>
    shared_ptr(const shared_ptr<_Yp>& __r, element_type* __p) noexcept
        : __shared_ptr<_Tp>(__r, __p) {
      libp2p::connection::storeKeeper(*this);
    }

#if __cplusplus > 201703L
    // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 2996. Missing rvalue overloads for shared_ptr operations
      /**
       *  @brief  Constructs a `shared_ptr` instance that stores `__p`
       *          and shares ownership with `__r`.
       *  @param  __r  A `shared_ptr`.
       *  @param  __p  A pointer that will remain valid while `*__r` is valid.
       *  @post   `get() == __p && !__r.use_count() && !__r.get()`
       *
       *  This can be used to construct a `shared_ptr` to a sub-object
       *  of an object managed by an existing `shared_ptr`. The complete
       *  object will remain valid while any `shared_ptr` owns it, even
       *  if they don't store a pointer to the complete object.
       *
       * @code
       * shared_ptr<pair<int,int>> pii(new pair<int,int>());
       * shared_ptr<int> pi1(pii, &pii->first);
       * assert(pii.use_count() == 2);
       * shared_ptr<int> pi2(std::move(pii), &pii->second);
       * assert(pii.use_count() == 0);
       * @endcode
       */
      template<typename _Yp>
	shared_ptr(shared_ptr<_Yp>&& __r, element_type* __p) noexcept
	: __shared_ptr<_Tp>(std::move(__r), __p) { }
#endif
    /**
     *  @brief  If @a __r is empty, constructs an empty %shared_ptr;
     *          otherwise construct a %shared_ptr that shares ownership
     *          with @a __r.
     *  @param  __r  A %shared_ptr.
     *  @post   get() == __r.get() && use_count() == __r.use_count()
     */
    template<typename _Yp,
        typename = _Constructible<const shared_ptr<_Yp>&>>
    shared_ptr(const shared_ptr<_Yp>& __r) noexcept
        : __shared_ptr<_Tp>(__r) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Move-constructs a %shared_ptr instance from @a __r.
     *  @param  __r  A %shared_ptr rvalue.
     *  @post   *this contains the old value of @a __r, @a __r is empty.
     */
    shared_ptr(shared_ptr&& __r) noexcept
        : __shared_ptr<_Tp>(std::move(__r)) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Move-constructs a %shared_ptr instance from @a __r.
     *  @param  __r  A %shared_ptr rvalue.
     *  @post   *this contains the old value of @a __r, @a __r is empty.
     */
    template<typename _Yp, typename = _Constructible<shared_ptr<_Yp>>>
    shared_ptr(shared_ptr<_Yp>&& __r) noexcept
        : __shared_ptr<_Tp>(std::move(__r)) {
      libp2p::connection::storeKeeper(*this);
    }

    /**
     *  @brief  Constructs a %shared_ptr that shares ownership with @a __r
     *          and stores a copy of the pointer stored in @a __r.
     *  @param  __r  A weak_ptr.
     *  @post   use_count() == __r.use_count()
     *  @throw  bad_weak_ptr when __r.expired(),
     *          in which case the constructor has no effect.
     */
    template<typename _Yp, typename = _Constructible<const weak_ptr<_Yp>&>>
    explicit shared_ptr(const weak_ptr<_Yp>& __r)
        : __shared_ptr<_Tp>(__r) {
      libp2p::connection::storeKeeper(*this);
    }

#if _GLIBCXX_USE_DEPRECATED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    template<typename _Yp, typename = _Constructible<auto_ptr<_Yp>>>
    shared_ptr(auto_ptr<_Yp>&& __r);
#pragma GCC diagnostic pop
#endif

    // _GLIBCXX_RESOLVE_LIB_DEFECTS
    // 2399. shared_ptr's constructor from unique_ptr should be constrained
    template<typename _Yp, typename _Del,
        typename = _Constructible<unique_ptr<_Yp, _Del>>>
    shared_ptr(unique_ptr<_Yp, _Del>&& __r)
        : __shared_ptr<_Tp>(std::move(__r)) {
      libp2p::connection::storeKeeper(*this);
    }

#if __cplusplus <= 201402L && _GLIBCXX_USE_DEPRECATED
    // This non-standard constructor exists to support conversions that
      // were possible in C++11 and C++14 but are ill-formed in C++17.
      // If an exception is thrown this constructor has no effect.
      template<typename _Yp, typename _Del,
		_Constructible<unique_ptr<_Yp, _Del>, __sp_array_delete>* = 0>
	shared_ptr(unique_ptr<_Yp, _Del>&& __r)
	: __shared_ptr<_Tp>(std::move(__r), __sp_array_delete()) { }
#endif

    /**
     *  @brief  Construct an empty %shared_ptr.
     *  @post   use_count() == 0 && get() == nullptr
     */
    constexpr shared_ptr(nullptr_t) noexcept : shared_ptr() {
    }

    shared_ptr& operator=(const shared_ptr&) noexcept = default;

    template<typename _Yp>
    _Assignable<const shared_ptr<_Yp>&>
    operator=(const shared_ptr<_Yp>& __r) noexcept
    {
      this->__shared_ptr<_Tp>::operator=(__r);
      return *this;
    }

#if _GLIBCXX_USE_DEPRECATED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    template<typename _Yp>
    _Assignable<auto_ptr<_Yp>>
    operator=(auto_ptr<_Yp>&& __r)
    {
      this->__shared_ptr<_Tp>::operator=(std::move(__r));
      return *this;
    }
#pragma GCC diagnostic pop
#endif

    shared_ptr&
    operator=(shared_ptr&& __r) noexcept
    {
      this->__shared_ptr<_Tp>::operator=(std::move(__r));
      return *this;
    }

    template<class _Yp>
    _Assignable<shared_ptr<_Yp>>
    operator=(shared_ptr<_Yp>&& __r) noexcept
    {
      this->__shared_ptr<_Tp>::operator=(std::move(__r));
      return *this;
    }

    template<typename _Yp, typename _Del>
    _Assignable<unique_ptr<_Yp, _Del>>
    operator=(unique_ptr<_Yp, _Del>&& __r)
    {
      this->__shared_ptr<_Tp>::operator=(std::move(__r));
      return *this;
    }

   private:
    // This constructor is non-standard, it is used by allocate_shared.
    template<typename _Alloc, typename... _Args>
    shared_ptr(_Sp_alloc_shared_tag<_Alloc> __tag, _Args&&... __args)
        : __shared_ptr<_Tp>(__tag, std::forward<_Args>(__args)...)
    {
      libp2p::connection::storeKeeper(*this);
    }

    template<typename _Yp, typename _Alloc, typename... _Args>
    friend shared_ptr<_Yp>
    allocate_shared(const _Alloc& __a, _Args&&... __args);

    // This constructor is non-standard, it is used by weak_ptr::lock().
    shared_ptr(const weak_ptr<_Tp>& __r, std::nothrow_t)
        : __shared_ptr<_Tp>(__r, std::nothrow) {
      libp2p::connection::storeKeeper(*this);
    }

    friend class weak_ptr<_Tp>;
  };

}

namespace libp2p::connection {

  /**
   * Implementation of stream multiplexer - connection, which has only one
   * physical link to another peer, but many logical streams, for example, for
   * several applications
   * Read more: https://github.com/hashicorp/yamux/blob/master/spec.md
   */
  class YamuxedConnection final
      : public CapableConnection,
        public YamuxStreamFeedback,
        public std::enable_shared_from_this<YamuxedConnection> {
   public:
    using StreamId = uint32_t;

    YamuxedConnection(const YamuxedConnection &other) = delete;
    YamuxedConnection &operator=(const YamuxedConnection &other) = delete;
    YamuxedConnection(YamuxedConnection &&other) = delete;
    YamuxedConnection &operator=(YamuxedConnection &&other) = delete;
    ~YamuxedConnection();

    /**
     * Create a new YamuxedConnection instance
     * @param connection to be multiplexed by this instance
     * @param config to configure this instance
     * @param logger to output messages
     */
    explicit YamuxedConnection(std::shared_ptr<SecureConnection> connection,
                               std::shared_ptr<basic::Scheduler> scheduler,
                               ConnectionClosedCallback closed_callback,
                               muxer::MuxedConnectionConfig config = {});

    void start() override;

    void stop() override;

    outcome::result<std::shared_ptr<Stream>> newStream() override;

    void newStream(StreamHandlerFunc cb) override;

    void onStream(NewStreamHandlerFunc cb) override;

    outcome::result<peer::PeerId> localPeer() const override;

    outcome::result<peer::PeerId> remotePeer() const override;

    outcome::result<crypto::PublicKey> remotePublicKey() const override;

    bool isInitiator() const noexcept override;

    outcome::result<multi::Multiaddress> localMultiaddr() override;

    outcome::result<multi::Multiaddress> remoteMultiaddr() override;

    outcome::result<void> close() override;

    bool isClosed() const override;
    void printStream();

    void deferReadCallback(outcome::result<size_t> res,
                           ReadCallbackFunc cb) override;
    void deferWriteCallback(std::error_code ec, WriteCallbackFunc cb) override;

   private:
    using Streams = std::unordered_map<StreamId, std::shared_ptr<YamuxStream>>;

    using PendingOutboundStreams =
        std::unordered_map<StreamId, StreamHandlerFunc>;

    using Buffer = common::ByteArray;

    struct WriteQueueItem {
      // TODO(artem): reform in buffers (shared + vector writes)

      Buffer packet;
      StreamId stream_id;
      bool some;
    };

    // YamuxStreamFeedback interface overrides

    /// Stream transfers data to connection
    void writeStreamData(uint32_t stream_id, gsl::span<const uint8_t> data,
                         bool some) override;

    /// Stream acknowledges received bytes
    void ackReceivedBytes(uint32_t stream_id, uint32_t bytes) override;

    /// Stream defers callback to avoid reentrancy
    void deferCall(std::function<void()>) override;

    /// Stream closes (if immediately==false then all pending data will be sent)
    void resetStream(uint32_t stream_id) override;

    void streamClosed(uint32_t stream_id) override;

    /// usage of these four methods is highly not recommended or even forbidden:
    /// use stream over this connection instead
    void read(gsl::span<uint8_t> out, size_t bytes,
              ReadCallbackFunc cb) override;
    void readSome(gsl::span<uint8_t> out, size_t bytes,
                  ReadCallbackFunc cb) override;
    void write(gsl::span<const uint8_t> in, size_t bytes,
               WriteCallbackFunc cb) override;
    void writeSome(gsl::span<const uint8_t> in, size_t bytes,
                   WriteCallbackFunc cb) override;

    /// Initiates async readSome on connection
    void continueReading();

    /// Read callback
    void onRead(outcome::result<size_t> res);

    /// Processes incoming header, called from YamuxReadingState
    bool processHeader(boost::optional<YamuxFrame> header);

    /// Processes incoming data, called from YamuxReadingState
    void processData(gsl::span<uint8_t> segment, StreamId stream_id);

    /// FIN received from peer to stream (either in header or with last data
    /// segment)
    void processFin(StreamId stream_id);

    /// RST received from peer to stream (either in header or with last data
    /// segment)
    void processRst(StreamId stream_id);

    /// Processes incoming GO_AWAY frame
    void processGoAway(const YamuxFrame &frame);

    /// Processes incoming frame with SYN flag
    bool processSyn(const YamuxFrame &frame);

    /// Processes incoming frame with ACK flag
    bool processAck(const YamuxFrame &frame);

    /// Processes incoming WINDOW_UPDATE message
    bool processWindowUpdate(const YamuxFrame &frame);

    /// Closes everything, notifies streams and handlers
    void close(std::error_code notify_streams_code,
               boost::optional<YamuxFrame::GoAwayError> reply_to_peer_code);

    /// Writes data to underlying connection or (if is_writing_) enqueues them
    /// If stream_id != 0, stream will be acknowledged about data written
    void enqueue(Buffer packet, StreamId stream_id = 0, bool some = false);

    /// Performs write into connection
    void doWrite(WriteQueueItem packet);

    /// Write callback
    void onDataWritten(outcome::result<size_t> res, StreamId stream_id,
                       bool some);

    /// Creates new yamux stream
    std::shared_ptr<Stream> createStream(StreamId stream_id);

    /// Erases stream by id, may affect incactivity timer
    void eraseStream(StreamId stream_id);

    /// Erases entry from pending streams, may affect incactivity timer
    void erasePendingOutboundStream(PendingOutboundStreams::iterator it);

    /// Sets expire timer if last stream was just closed. Called from erase*()
    /// functions
    void adjustExpireTimer();

    /// Expire timer callback
    void onExpireTimer();

    /// Copy of config
    const muxer::MuxedConnectionConfig config_;

    /// Underlying connection
    std::shared_ptr<SecureConnection> connection_;

    /// Scheduler
    std::shared_ptr<basic::Scheduler> scheduler_;

    /// True if started
    bool started_ = false;

    struct Test {
        Buffer buffer;
        ~Test() {
            std::cout << "71476825636254 ~Test\n";
        }
    };

    /// TODO(artem): change read() interface to reduce copying
    std::shared_ptr<Test> raw_read_buffer_;

    /// Buffering and segmenting
    YamuxReadingState reading_state_;

    /// True if waiting for current write operation to complete
    bool is_writing_ = false;

    /// Write queue
    std::deque<WriteQueueItem> write_queue_;

    /// Active streams
    Streams streams_;

    /// Streams just created. Need to call handlers after all
    /// data is processed. StreamHandlerFunc is null for inbound streams
    std::vector<std::pair<StreamId, StreamHandlerFunc>> fresh_streams_;

    /// Handler for new inbound streams
    NewStreamHandlerFunc new_stream_handler_;

    /// New stream id (odd if underlying connection is outbound)
    StreamId new_stream_id_ = 0;

    /// Pending outbound streams
    PendingOutboundStreams pending_outbound_streams_;

    /// Timer handle for pings
    basic::Scheduler::Handle ping_handle_;

    /// Cleanup for detached streams
    basic::Scheduler::Handle cleanup_handle_;

    /// Timer handle for auto closing if inactive
    basic::Scheduler::Handle inactivity_handle_;

    /// Called on connection close
    ConnectionClosedCallback closed_callback_;

    /// Remote peer saved here
    peer::PeerId remote_peer_;

   public:
    LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(
        libp2p::connection::YamuxedConnection);
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_YAMUX_IMPL_HPP
