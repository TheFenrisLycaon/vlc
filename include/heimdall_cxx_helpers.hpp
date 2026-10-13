/*****************************************************************************
 * heimdall_cxx_helpers.hpp: C++ helpers
 *****************************************************************************
 * Copyright (C) 1998-2018 VLC authors and VideoLAN
 * Copyright (C) 2026 TheFenrisLycaon
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef HEIMDALL_CXX_HELPERS_HPP
#define HEIMDALL_CXX_HELPERS_HPP

/******************************************************************************
 * C++ memory management helpers
 ******************************************************************************/

#if defined(__cplusplus) || defined(DOC)

#include <memory>
#include <utility>
#include <type_traits>
#include <string>
#include <stdexcept>

#ifdef HEIMDALL_THREADS_H_
// Ensure we can use heimdall_sem_wait_i11e. We can't declare different versions
// of the semaphore helper based on heimdall_interrupt inclusion, as it would
// violate ODR
# include <heimdall_interrupt.h>
#endif

namespace heimdall
{

namespace
{
// This helpers need static linkage to avoid their signature to change when
// building as C++17 (noexcept becomes part of the function signature stating there)

// Wraps a pointer with a custom releaser
// ex: auto ptr = heimdall::wrap_cptr( input_item, &input_item_Release );

///
/// Wraps a C pointer into a std::unique_ptr
///
/// This will convert a C pointer of type T to a std::unique_ptr<T, R> where
/// T is the pointee type, and R is an arbitrary releaser type.
///
/// ptr will be automatically released by calling r( ptr ) when falling out of
/// scope (whether by returning or by throwing an exception).
///
/// @param ptr a C pointer
/// @param r An instance of a Callable type, that will be invoked with ptr
///          as its first and only parameter.
template <typename T, typename Releaser>
inline auto wrap_cptr( T* ptr, Releaser&& r ) noexcept
    -> std::unique_ptr<T, typename std::decay<decltype( r )>::type>
{
    return std::unique_ptr<T, typename std::decay<decltype( r )>::type>{
                ptr, std::forward<Releaser>( r )
    };
}

///
/// Wraps a C pointer into a std::unique_ptr
///
/// This will convert a C pointer to an array of type T to a
/// std::unique_ptr<T[], R> where T is the pointee type, and R is an arbitrary
/// releaser type.
///
/// ptr will be automatically released by calling r( ptr ) when falling out of
/// scope (whether by returning or by throwing an exception).
///
/// This function is equivalent to wrap_cptr, except that the returned
/// unique_ptr provides an operator[] for array access instead of operator* and
/// operator->
///
/// @param ptr a C pointer
/// @param r An instance of a Callable type, that will be invoked with ptr
///          as its first and only parameter.
template <typename T, typename Releaser>
inline auto wrap_carray( T* ptr, Releaser&& r ) noexcept
    -> std::unique_ptr<T[], typename std::decay<decltype( r )>::type>
{
    return std::unique_ptr<T[], typename std::decay<decltype( r )>::type>{
        ptr, std::forward<Releaser>( r )
    };
}

///
/// Wraps a C pointer into a std::unique_ptr
///
/// This is a convenience wrapper that will use free() as its releaser
///
template <typename T>
inline std::unique_ptr<T, void (*)(void*)> wrap_cptr( T* ptr ) noexcept
{
    return wrap_cptr( ptr, &free );
}

///
/// Wraps a C pointer into a std::unique_ptr
///
/// This is a convenience wrapper that will use free() as its releaser
///
template <typename T>
inline std::unique_ptr<T[], void (*)(void*)> wrap_carray( T* ptr ) noexcept
{
    return wrap_carray( ptr, &free );
}

} // anonymous namespace

///
/// Wraps a C shared resource having associated Hold() and Release() functions
//
/// This is a RAII wrapper for C shared resources (which are manually managed by
/// calling explicitly their Hold() and Release() functions).
///
/// The Hold() and Release() functions must accept exactly one parameter having
/// type T* (the raw pointer type). Their return type is irrelevant.
///
/// To create a new shared resource wrapper type for my_type_t, simply declare:
///
///     using MyTypePtr =
///         ::heimdall::heimdall_shared_data_ptr<my_type_t, &my_type_Hold, &my_type_Release>;
///
/// Then use it to wrap a raw C pointer:
///
///     my_type_t *raw_ptr = /* ... */;
///     MyTypePtr ptr(raw_ptr);
template <typename T, auto HOLD, auto RELEASE>
class heimdall_shared_data_ptr {
    T *ptr = nullptr;

public:
    /* default implicit constructor */
    heimdall_shared_data_ptr() = default;

    /**
     * Wrap a shared resource.
     *
     * If the pointer is not nullptr, and hold is true, then the resource is
     * hold (the caller shared ownership is preserved).
     * If hold is false, then the caller transfers the ownership to this
     * wrapper.
     *
     * \param ptr  the raw pointer (can be nullptr)
     * \param hold whether the resource must be hold
     */
    explicit heimdall_shared_data_ptr(T *ptr, bool hold = true)
        : ptr(ptr)
    {
        if (ptr && hold)
            HOLD(ptr);
    }

    heimdall_shared_data_ptr(std::nullptr_t)
        : ptr(nullptr)
    {
    }

    heimdall_shared_data_ptr(const heimdall_shared_data_ptr &other)
        : heimdall_shared_data_ptr(other.ptr) {}

    heimdall_shared_data_ptr(heimdall_shared_data_ptr &&other) noexcept
        : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }

    ~heimdall_shared_data_ptr()
    {
        if (ptr)
            RELEASE(ptr);
    }

    heimdall_shared_data_ptr &operator=(const heimdall_shared_data_ptr &other)
    {
        reset(other.ptr, true);
        return *this;
    }

    heimdall_shared_data_ptr &operator=(heimdall_shared_data_ptr &&other) noexcept
    {
        reset(other.ptr, false);
        other.ptr = nullptr;
        return *this;
    }

    bool operator==(const heimdall_shared_data_ptr &other) const
    {
        return ptr == other.ptr;
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return ptr == nullptr;
    }

    bool operator!=(const heimdall_shared_data_ptr &other) const
    {
        return !(*this == other);
    }

    bool operator!=(std::nullptr_t) const noexcept
    {
        return ptr != nullptr;
    }

    explicit operator bool() const
    {
        return ptr;
    }

    T &operator*() const
    {
        return *ptr;
    }

    T *operator->() const
    {
        return ptr;
    }

    T *get() const
    {
        return ptr;
    }

    /**
     * Reset the shared resource.
     *
     *     ptr.reset(rawptr, hold);
     *
     * is semantically equivalent to:
     *
     *     ptr = heimdall_shared_data_ptr<...>(rawptr, hold);
     *
     * If the pointer is not nullptr, and hold is true, then the resource is
     * hold (the caller shared ownership is preserved).
     * If hold is false, then the caller transfers the ownership to this
     * wrapper.
     *
     * \param newptr  the raw pointer (can be nullptr)
     * \param hold    whether the resource must be hold
     */
    void reset(T *newptr = nullptr, bool hold = true)
    {
        if (newptr && hold)
            HOLD(newptr);
        if (ptr)
            RELEASE(ptr);
        ptr = newptr;
    }
};

#if defined(HEIMDALL_THREADS_H_) || defined(DOC)

namespace threads
{

class mutex
{
public:
    mutex() noexcept
    {
        heimdall_mutex_init( &m_mutex );
    }

    mutex( const mutex& ) = delete;
    mutex& operator=( const mutex& ) = delete;
    mutex( mutex&& ) = delete;
    mutex& operator=( mutex&& ) = delete;

    void lock() noexcept
    {
        heimdall_mutex_lock( &m_mutex );
    }
    void unlock() noexcept
    {
        heimdall_mutex_unlock( &m_mutex );
    }

private:
    heimdall_mutex_t m_mutex;
    friend class condition_variable;
    friend class mutex_locker;
};

class condition_variable
{
public:
    condition_variable() noexcept
    {
        heimdall_cond_init( &m_cond );
    }
    void signal() noexcept
    {
        heimdall_cond_signal( &m_cond );
    }
    void broadcast() noexcept
    {
        heimdall_cond_broadcast( &m_cond );
    }
    void wait( mutex& mutex ) noexcept
    {
        heimdall_cond_wait( &m_cond, &mutex.m_mutex );
    }
    int timedwait( mutex& mutex, heimdall_tick_t deadline ) noexcept
    {
        return heimdall_cond_timedwait( &m_cond, &mutex.m_mutex, deadline );
    }

private:
    heimdall_cond_t m_cond;
};

class mutex_locker
{
public:
    mutex_locker( heimdall_mutex_t* m ) noexcept
        : m_mutex( m )
    {
        heimdall_mutex_lock( m_mutex );
    }
    mutex_locker( mutex& m ) noexcept
        : mutex_locker( &m.m_mutex )
    {
    }
    ~mutex_locker()
    {
        heimdall_mutex_unlock( m_mutex );
    }
    mutex_locker( const mutex_locker& ) = delete;
    mutex_locker& operator=( const mutex_locker& ) = delete;
    mutex_locker( mutex_locker&& ) = delete;
    mutex_locker& operator=( mutex_locker&& ) = delete;

private:
    heimdall_mutex_t* m_mutex;
};

class semaphore
{
public:
    semaphore() noexcept
    {
        heimdall_sem_init( &m_sem, 0 );
    }
    semaphore( unsigned int count ) noexcept
    {
        heimdall_sem_init( &m_sem, count );
    }
    ~semaphore()
    {
    }

    semaphore( const semaphore& ) = delete;
    semaphore& operator=( const semaphore& ) = delete;
    semaphore( semaphore&& ) = delete;
    semaphore& operator=( semaphore&& ) = delete;

    int post() noexcept
    {
        return heimdall_sem_post( &m_sem );
    }
    void wait() noexcept
    {
        heimdall_sem_wait( &m_sem );
    }

    int wait_i11e() noexcept
    {
        return heimdall_sem_wait_i11e( &m_sem );
    }

private:
    heimdall_sem_t m_sem;
};

}

#endif // HEIMDALL_THREADS_H_

#ifdef HEIMDALL_URL_H

class url : public heimdall_url_t
{
public:
    class invalid : public std::runtime_error
    {
    public:
        explicit invalid( const char* url )
            : std::runtime_error( std::string{ "Invalid url: " } + url )
        {
        }
    };

    url()
    {
        psz_buffer = nullptr;
        psz_pathbuffer = nullptr;
        psz_host = nullptr;
    }

    explicit url( const char* str )
    {
        if ( heimdall_UrlParse( this, str ) )
        {
            heimdall_UrlClean( this );
            throw invalid( str );
        }
    }

    explicit url( const std::string& str )
        : url( str.c_str() )
    {
    }

    ~url()
    {
        heimdall_UrlClean(this);
    }

    url( const url& ) = delete;
    url& operator=( const url& ) = delete;

    url( url&& u ) noexcept
        : heimdall_url_t( u )
    {
        u.psz_buffer = nullptr;
        u.psz_pathbuffer = nullptr;
        u.psz_host = nullptr;
    }

    url& operator=( url&& u ) noexcept
    {
        heimdall_UrlClean( this );
        *(static_cast<heimdall_url_t*>( this )) = u;
        u.psz_buffer = nullptr;
        u.psz_pathbuffer = nullptr;
        u.psz_host = nullptr;
        return *this;
    }
};

#endif

} // namespace heimdall

#endif

#endif // HEIMDALL_CXX_HELPERS_HPP
