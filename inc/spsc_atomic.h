#ifndef _SPSC_ATOMIC_H_
#define _SPSC_ATOMIC_H_

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

#define QUEUE_SIZE_SPSC_ATOMIC 1000000

template <class T>
class SPSCAtomic
{
  private:
  const uint64_t _queue_size;
  #if defined(STATIC_ALLOC_SPSC_ATOMIC)
  T _data[QUEUE_SIZE_SPSC_ATOMIC];
  #else
  T* _data;
  #endif
  
  std::atomic<uint64_t> _write_index;
  std::atomic<uint64_t> _read_index;
  
  public:
  #if defined(STATIC_ALLOC_SPSC_ATOMIC)
  explicit SPSCAtomic(uint64_t queue_size):_queue_size(QUEUE_SIZE_SPSC_ATOMIC),_read_index(0),_write_index(0){};
  #else
  explicit SPSCAtomic(uint64_t queue_size):_queue_size(queue_size),_read_index(0),_write_index(0)
  {
    assert (_queue_size >= 2);
    _data = nullptr;
    _data = new T[_queue_size];
  };
  #endif

  #if defined(STATIC_ALLOC_SPSC_ATOMIC)
  ~SPSCAtomic()
  {
    while(_read_index != _write_index)
    {
      auto index = _read_index % _queue_size;
      _data[index].~T();
      _read_index = _read_index + 1;
    }
    _read_index = 0;
    _write_index = 0;
  };
  #else
  ~SPSCAtomic()
  {
    delete _data;
  }
  #endif
  
  template <class ...Args>
  bool enqueue(Args&&... record_args)
  {
    auto current_write_index = (_write_index.load(std::memory_order_relaxed) + 1) % _queue_size;
    while ( current_write_index == (_read_index.load(std::memory_order_acquire) % _queue_size) ) 
    {
      sched_yield();
    }
    new (&_data[current_write_index]) T(std::forward<Args>(record_args)...);
    _write_index.fetch_add((uint64_t)1, std::memory_order_release);
    return true;
  };

  bool dequeue(T& record)
  {
    auto current_read_index = _read_index.load(std::memory_order_seq_cst) % _queue_size;
    while ( current_read_index == (_write_index.load(std::memory_order_acquire) % _queue_size) )
    {
      sched_yield();
    }
    current_read_index = (current_read_index + 1 ) % _queue_size;
    record = std::move(_data[current_read_index]);
    _data[current_read_index].~T();
    _read_index.fetch_add((uint64_t)1, std::memory_order_release);
    return true;
  };
  
  template<class ...Args>
  bool enqueue_noblock(Args&&... record_args)
  {
    auto current_write_index = (_write_index.load(std::memory_order_relaxed) + 1) % _queue_size;
    if ( current_write_index == (_read_index.load(std::memory_order_acquire) % _queue_size) ) 
    {
      return false;
    }
    new (&_data[current_write_index]) T(std::forward<Args>(record_args)...);
    _write_index.fetch_add((uint64_t)1, std::memory_order_release);
    return true;
  };

  bool dequeue_noblock(T& record)
  {
    auto current_read_index = _read_index.load(std::memory_order_relaxed) % _queue_size;
    if ( current_read_index == (_write_index.load(std::memory_order_acquire) % _queue_size) )
    {
      return false;
    }
    current_read_index = (current_read_index + 1 ) % _queue_size;
    record = std::move(_data[current_read_index]);
    _data[current_read_index].~T();
    _read_index.fetch_add((uint64_t)1, std::memory_order_release);
    return true;
  };

  T* front()
  {
  };

  T* front_noblock()
  {
  };

  bool pop_front()
  {
  };

  bool is_empty()
  {
  };
};
#endif//_SPSC_ATOMIC_H_
