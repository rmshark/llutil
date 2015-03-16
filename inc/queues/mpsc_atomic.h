#ifndef _MPSC_ATOMIC_H_
#define _MPSC_ATOMIC_H_

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#define QUEUE_SIZE_MPSC_ATOMIC 1000000

template< class T>
class MPSCAtomic
{
  private:
  const uint64_t _queue_size;

#if defined(STATIC_ALLOC_MPSC_QUEUE)
  T _data[QUEUE_SIZE_MPSC_ATOMIC];
  std::atomic<bool> _flag[QUEUE_SIZE_MPSC_ATOMIC];
#else
  T* _data;
  std::atomic<bool>* _flag;
#endif

std::atomic<uint64_t> _write_index;
  std::atomic<uint64_t> _read_index;

  public:
#if defined(STATIC_ALLOC_MPSC_QUEUE)
  explicit MPSCAtomic(uint64_t queue_size):_queue_size(QUEUE_SIZE_MPSC_ATOMIC),_read_index(0),_write_index(0){};
#else
  explicit MPSCAtomic(uint64_t queue_size):_queue_size(queue_size),_read_index(0),_write_index(0)
  {
    assert ( _queue_size >= 2);
    _data = NULL;
    _flag = NULL;
    _data = new T[_queue_size];
    _flag = new std::atomic<bool>[_queue_size];
  };
#endif

#if defined(STATIC_ALLOC_MPSC_ATOMIC)
  ~MPSCAtomic()
  {
    while(_read_index != _write_index)
    {
      auto index = _read_index % _queue_size;
      _data[index].~T();
      _flag[index] = false;
      _read_index = _read_index + 1;
    }
    _read_index = 0;
    _write_index = 0;
  };
#else
  ~MPSCAtomic()
  {
    delete _data;
    delete _flag;
  };
#endif

  template<class ...Args>
  bool enqueue(Args&&... record_args)
  {
    auto current_write_index = _write_index.fetch_add((uint64_t)1,std::memory_order_seq_cst) % _queue_size ;
    while ( _flag[current_write_index].load(std::memory_order_seq_cst) == true )
    {
      sched_yield();
    }
    new (&_data[current_write_index]) T(std::forward<Args>(record_args)...);
    _flag[current_write_index].store(true, std::memory_order_seq_cst);
    return true;
  };

  bool dequeue(T& record)
  {
    auto current_read_index = _read_index.load(std::memory_order_seq_cst) % _queue_size;
    while ( _flag[current_read_index].load(std::memory_order_seq_cst) != true  )
    {
      //sched_yield();
    }
    record = std::move(_data[current_read_index]);
    _data[current_read_index].~T();
    _flag[current_read_index].store(false, std::memory_order_seq_cst);
    _read_index.fetch_add((uint64_t)1, std::memory_order_seq_cst);
    return true;
  };

  template<class ...Args>
  bool enqueue_noblock(Args&&... record_args)
  {

  };

  bool dequeue_noblock(T& record)
  {
    auto current_read_index = _read_index.load(std::memory_order_seq_cst) % _queue_size;
    if ( !_flag[current_read_index].load(std::memory_order_seq_cst) )
    {
      return false;
    }
    record = std::move(_data[current_read_index]);
    _data[current_read_index].~T();
    _flag[current_read_index].store(false, std::memory_order_seq_cst);
    _read_index.fetch_add((uint64_t)1, std::memory_order_seq_cst);
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
#endif//_MPSC_ATOMIC_H_
