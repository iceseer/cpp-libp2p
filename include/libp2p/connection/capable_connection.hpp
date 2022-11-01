/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CAPABLE_CONNECTION_HPP
#define LIBP2P_CAPABLE_CONNECTION_HPP

#include <functional>

#include <libp2p/connection/secure_connection.hpp>

namespace libp2p::connection {

  class CapableConnection;

  extern void storeKeeper();
  extern void removeKeeper();
}

namespace std {

  template<>
  class shared_ptr<libp2p::connection::CapableConnection> : public __shared_ptr<libp2p::connection::CapableConnection>
  {
    using _Tp = libp2p::connection::CapableConnection;

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
      libp2p::connection::storeKeeper();
    }

    shared_ptr(const shared_ptr& t) noexcept : __shared_ptr<_Tp>(t) {
      libp2p::connection::storeKeeper();
    }

    ~shared_ptr() {
      libp2p::connection::removeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
    }

    /**
     *  @brief  Move-constructs a %shared_ptr instance from @a __r.
     *  @param  __r  A %shared_ptr rvalue.
     *  @post   *this contains the old value of @a __r, @a __r is empty.
     */
    shared_ptr(shared_ptr&& __r) noexcept
        : __shared_ptr<_Tp>(std::move(__r)) {
      libp2p::connection::storeKeeper();
    }

    /**
     *  @brief  Move-constructs a %shared_ptr instance from @a __r.
     *  @param  __r  A %shared_ptr rvalue.
     *  @post   *this contains the old value of @a __r, @a __r is empty.
     */
    template<typename _Yp, typename = _Constructible<shared_ptr<_Yp>>>
    shared_ptr(shared_ptr<_Yp>&& __r) noexcept
        : __shared_ptr<_Tp>(std::move(__r)) {
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
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
      libp2p::connection::storeKeeper();
    }

    template<typename _Yp, typename _Alloc, typename... _Args>
    friend shared_ptr<_Yp>
    allocate_shared(const _Alloc& __a, _Args&&... __args);

    // This constructor is non-standard, it is used by weak_ptr::lock().
    shared_ptr(const weak_ptr<_Tp>& __r, std::nothrow_t)
        : __shared_ptr<_Tp>(__r, std::nothrow) {
      libp2p::connection::storeKeeper();
    }

    friend class weak_ptr<_Tp>;
  };

}


namespace libp2p::connection {
  struct Stream;

  /**
   * Connection that provides basic libp2p requirements to the connection: it is
   * both secured and muxed (streams can be created over that connection)
   */
  struct CapableConnection : public SecureConnection {
    using StreamHandler = void(outcome::result<std::shared_ptr<Stream>>);
    using StreamHandlerFunc = std::function<StreamHandler>;

    using NewStreamHandlerFunc = std::function<void(std::shared_ptr<Stream>)>;

    using ConnectionClosedCallback = std::function<void(
        const peer::PeerId &,
        const std::shared_ptr<connection::CapableConnection> &)>;

    ~CapableConnection() override = default;

    /**
     * Start to process incoming messages for this connection
     * @note non-blocking
     *
     * @note make sure onStream(..) was called, so that new streams are accepted
     * by this connection - call to start() will fail otherwise
     */
    virtual void start() = 0;

    /**
     * Stop processing incoming messages for this connection without closing the
     * connection itself
     * @note calling 'start' after 'close' is UB
     */
    virtual void stop() = 0;

    /**
     * @brief Opens new stream in a synchronous (optimistic) manner
     * @return Stream or error
     */
    virtual outcome::result<std::shared_ptr<Stream>> newStream() = 0;

    /**
     * @brief Opens new stream using this connection
     * @param cb - callback to be called, when a new stream is established or
     * error appears
     */
    virtual void newStream(StreamHandlerFunc cb) = 0;

    /**
     * @brief Set a handler, which is called, when a new stream arrives from the
     * other side
     * @param cb, to which a received stream is passed
     * @note if a handler is not set, all received streams will be immediately
     * reset
     */
    virtual void onStream(NewStreamHandlerFunc cb) = 0;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_CAPABLE_CONNECTION_HPP
