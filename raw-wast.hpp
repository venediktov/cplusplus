/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
 #include <string.h>
 #include <unistd.h>
 #include <stdio.h>
 
 extern "C" {

/**
 *  @defgroup types Builtin Types
 *  @ingroup contractdev
 *  @brief Specifies typedefs and aliases
 *
 *  @{
 */

 

typedef unsigned __int128    uint128_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef unsigned short       uint16_t; 
typedef unsigned char        uint8_t;

typedef __int128             int128_t;
typedef long long            int64_t;
typedef long                 int32_t;
typedef short                int16_t;
typedef char                 int8_t;

struct uint256 {
   uint64_t words[4];
};

typedef unsigned int size_t;

typedef uint64_t account_name;
typedef uint64_t permission_name;
typedef uint64_t token_name;
typedef uint64_t table_name;
typedef uint32_t time;
typedef uint64_t func_name;

typedef uint64_t asset_symbol;
typedef int64_t share_type;

#define PACKED(X) __attribute__((packed)) X

struct public_key {
   uint8_t data[33];
};

struct asset {
   share_type   amount;
   asset_symbol symbol;
};

struct price {
   asset base;
   asset quote;
};

struct signature {
   uint8_t data[65];
};

struct checksum {
   uint64_t hash[4];
};

struct fixed_string16 {
   uint8_t len;
   char    str[16];
};

typedef fixed_string16 field_name;

struct fixed_string32 {
   uint8_t len;
   char    str[32];
};

typedef fixed_string32 type_name;

struct bytes {
   uint32_t len;
   uint8_t* data;
};


} /// extern "C"


 
 void  assert( uint32_t test, const char* cstr );
 
 
 namespace eosio {

   //using ::memset;
   //using ::memcpy;

  /**
   *  @defgroup memorycppapi Memory C++ API
   *  @brief Defines common memory functions
   *  @ingroup memoryapi
   *
   *  @{
   */

   class memory_manager  // NOTE: Should never allocate another instance of memory_manager
   {
   friend void* malloc(uint32_t size);
   friend void* realloc(void* ptr, uint32_t size);
   friend void free(void* ptr);
   public:
      memory_manager()
      // NOTE: it appears that WASM has an issue with initialization lists if the object is globally allocated,
      //       and seems to just initialize members to 0
      : _heaps_actual_size(0)
      , _active_heap(0)
      , _active_free_heap(0)
      {
      }

   private:
      class memory;

      memory* next_active_heap()
      {
         memory* const current_memory = _available_heaps + _active_heap;

         // make sure we will not exceed the 1M limit (needs to match wasm_interface.cpp _max_memory)
         auto remaining = 1024 * 1024 - reinterpret_cast<int32_t>(sbrk(0));
         if (remaining <= 0)
         {
            // ensure that any remaining unallocated memory gets cleaned up
            current_memory->cleanup_remaining();
            ++_active_heap;
            _heaps_actual_size = _active_heap;
            return nullptr;
         }

         const uint32_t new_heap_size = remaining > _new_heap_size ? _new_heap_size : remaining;
         char* new_memory_start = static_cast<char*>(sbrk(new_heap_size));
         // if we can expand the current memory, keep working with it
         if (current_memory->expand_memory(new_memory_start, new_heap_size))
            return current_memory;

         // ensure that any remaining unallocated memory gets cleaned up
         current_memory->cleanup_remaining();

         ++_active_heap;
         memory* const next = _available_heaps + _active_heap;
         next->init(new_memory_start, new_heap_size);

         return next;
      }

      void* malloc(uint32_t size)
      {
         if (size == 0)
            return nullptr;

         // see Note on ctor
         if (_heaps_actual_size == 0)
            _heaps_actual_size = _heaps_size;

         adjust_to_mem_block(size);

         // first pass of loop never has to initialize the slot in _available_heap
         uint32_t needs_init = 0;
         char* buffer = nullptr;
         memory* current = nullptr;
         // need to make sure
         if (_active_heap < _heaps_actual_size)
         {
            memory* const start_heap = &_available_heaps[_active_heap];
            // only heap 0 won't be initialized already
            if(_active_heap == 0 && !start_heap->is_init())
            {
               start_heap->init(_initial_heap, _initial_heap_size);
            }

            current = start_heap;
         }

         while (current != nullptr)
         {
            buffer = current->malloc(size);
            // done if we have a buffer
            if (buffer != nullptr)
               break;

            current = next_active_heap();
         }

         if (buffer == nullptr)
         {
            const uint32_t end_free_heap = _active_free_heap;

            do
            {
               buffer = _available_heaps[_active_free_heap].malloc_from_freed(size);

               if (buffer != nullptr)
                  break;

               if (++_active_free_heap == _heaps_actual_size)
                  _active_free_heap = 0;

            } while (_active_free_heap != end_free_heap);
         }

         return buffer;
      }

      void* realloc(void* ptr, uint32_t size)
      {
         if (size == 0)
         {
            free(ptr);
            return nullptr;
         }

         const uint32_t REMOVE = size;
         adjust_to_mem_block(size);

         char* realloc_ptr = nullptr;
         uint32_t orig_ptr_size = 0;
         if (ptr != nullptr)
         {
            char* const char_ptr = static_cast<char*>(ptr);
            for (memory* realloc_heap = _available_heaps; realloc_heap < _available_heaps + _heaps_actual_size && realloc_heap->is_init(); ++realloc_heap)
            {
               if (realloc_heap->is_in_heap(char_ptr))
               {
                  realloc_ptr = realloc_heap->realloc_in_place(char_ptr, size, &orig_ptr_size);

                  if (realloc_ptr != nullptr)
                     return realloc_ptr;
                  else
                     break;
               }
            }
         }

         char* new_alloc = static_cast<char*>(malloc(size));
         if (new_alloc == nullptr)
            return nullptr;

         const uint32_t copy_size = (size < orig_ptr_size) ? size : orig_ptr_size;
         if (copy_size > 0)
         {
            memcpy(new_alloc, ptr, copy_size);
            free (ptr);
         }

         return new_alloc;
      }

      void free(void* ptr)
      {
         if (ptr == nullptr)
            return;

         char* const char_ptr = static_cast<char*>(ptr);
         for (memory* free_heap = _available_heaps; free_heap < _available_heaps + _heaps_actual_size && free_heap->is_init(); ++free_heap)
         {
            if (free_heap->is_in_heap(char_ptr))
            {
               free_heap->free(char_ptr);
               break;
            }
         }
      }

      void adjust_to_mem_block(uint32_t& size)
      {
         const uint32_t remainder = (size + _size_marker) & _rem_mem_block_mask;
         if (remainder > 0)
         {
            size += _mem_block - remainder;
         }
      }

      class memory
      {
      public:
         memory()
         : _heap_size(0)
         , _heap(nullptr)
         , _offset(0)
         {
         }

         void init(char* const mem_heap, uint32_t size)
         {
            _heap_size = size;
            _heap = mem_heap;
            memset(_heap, 0, _heap_size);
         }

         uint32_t is_init() const
         {
            return _heap != nullptr;
         }

         uint32_t is_in_heap(const char* const ptr) const
         {
            const char* const end_of_buffer = _heap + _heap_size;
            const char* const first_ptr_of_buffer = _heap + _size_marker;
            return ptr >= first_ptr_of_buffer && ptr < end_of_buffer;
         }

         uint32_t is_capacity_remaining() const
         {
            return _offset + _size_marker < _heap_size;
         }

         char* malloc(uint32_t size)
         {
            uint32_t used_up_size = _offset + size + _size_marker;
            if (used_up_size > _heap_size)
            {
               return nullptr;
            }

            buffer_ptr new_buff(&_heap[_offset + _size_marker], size, _heap + _heap_size);
            _offset += size + _size_marker;
            new_buff.mark_alloc();
            return new_buff.ptr();
         }

         char* malloc_from_freed(uint32_t size)
         {
            assert(_offset == _heap_size, "malloc_from_freed was designed to only be called after _heap was completely allocated");

            char* current = _heap + _size_marker;
            while (current != nullptr)
            {
               buffer_ptr current_buffer(current, _heap + _heap_size);
               if (!current_buffer.is_alloc())
               {
                  // done if we have enough contiguous memory
                  if (current_buffer.merge_contiguous(size))
                  {
                     current_buffer.mark_alloc();
                     return current;
                  }
               }

               current = current_buffer.next_ptr();
            }

            // failed to find any free memory
            return nullptr;
         }

         char* realloc_in_place(char* const ptr, uint32_t size, uint32_t* orig_ptr_size)
         {
            const char* const end_of_buffer = _heap + _heap_size;

            buffer_ptr orig_buffer(ptr, end_of_buffer);
            *orig_ptr_size = orig_buffer.size();
            // is the passed in pointer valid
            char* const orig_buffer_end = orig_buffer.end();
            if (orig_buffer_end > end_of_buffer)
            {
               *orig_ptr_size = 0;
               return nullptr;
            }

            if (ptr > end_of_buffer - size)
            {
               // cannot resize in place
               return nullptr;
            }

            const int32_t diff = size - *orig_ptr_size;
            if (diff < 0)
            {
               // use a buffer_ptr to allocate the memory to free
               char* const new_ptr = ptr + size + _size_marker;
               buffer_ptr excess_to_free(new_ptr, -diff, _heap + _heap_size);
               excess_to_free.mark_free();

               return ptr;
            }
            // if ptr was the last allocated buffer, we can expand
            else if (orig_buffer_end == &_heap[_offset])
            {
               orig_buffer.size(size);
               _offset += diff;

               return ptr;
            }
            if (-diff == 0)
               return ptr;

            if (!orig_buffer.merge_contiguous_if_available(size))
               // could not resize in place
               return nullptr;

            orig_buffer.mark_alloc();
            return ptr;
         }

         void free(char* ptr)
         {
            buffer_ptr to_free(ptr, _heap + _heap_size);
            to_free.mark_free();
         }

         void cleanup_remaining()
         {
            if (_offset == _heap_size)
               return;

            // take the remaining memory and act like it was allocated
            const uint32_t size = _heap_size - _offset - _size_marker;
            buffer_ptr new_buff(&_heap[_offset + _size_marker], size, _heap + _heap_size);
            _offset = _heap_size;
            new_buff.mark_free();
         }

         bool expand_memory(char* exp_mem, uint32_t size)
         {
            if (_heap + _heap_size != exp_mem)
               return false;

            _heap_size += size;

            return true;
         }

      private:
         class buffer_ptr
         {
         public:
            buffer_ptr(void* ptr, const char* const heap_end)
            : _ptr(static_cast<char*>(ptr))
            , _size(*reinterpret_cast<uint32_t*>(static_cast<char*>(ptr) - _size_marker) & ~_alloc_memory_mask)
            , _heap_end(heap_end)
            {
            }

            buffer_ptr(void* ptr, uint32_t buff_size, const char* const heap_end)
            : _ptr(static_cast<char*>(ptr))
            , _heap_end(heap_end)
            {
               size(buff_size);
            }

            uint32_t size() const
            {
               return _size;
            }

            char* next_ptr() const
            {
               char* const next = end() + _size_marker;
               if (next >= _heap_end)
                  return nullptr;

               return next;
            }

            void size(uint32_t val)
            {
               // keep the same state (allocated or free) as was set before
               const uint32_t memory_state = *reinterpret_cast<uint32_t*>(_ptr - _size_marker) & _alloc_memory_mask;
               *reinterpret_cast<uint32_t*>(_ptr - _size_marker) = val | memory_state;
               _size = val;
            }

            char* end() const
            {
               return _ptr + _size;
            }

            char* ptr() const
            {
               return _ptr;
            }

            void mark_alloc()
            {
               *reinterpret_cast<uint32_t*>(_ptr - _size_marker) |= _alloc_memory_mask;
            }

            void mark_free()
            {
               *reinterpret_cast<uint32_t*>(_ptr - _size_marker) &= ~_alloc_memory_mask;
            }

            bool is_alloc() const
            {
               return *reinterpret_cast<const uint32_t*>(_ptr - _size_marker) & _alloc_memory_mask;
            }

            bool merge_contiguous_if_available(uint32_t needed_size)
            {
               return merge_contiguous(needed_size, true);
            }

            bool merge_contiguous(uint32_t needed_size)
            {
               return merge_contiguous(needed_size, false);
            }
         private:
            bool merge_contiguous(uint32_t needed_size, bool all_or_nothing)
            {
               // do not bother if there isn't contiguious space to allocate
               if (all_or_nothing && _heap_end - _ptr < needed_size)
                  return false;

               uint32_t possible_size = _size;
               while (possible_size < needed_size  && (_ptr + possible_size < _heap_end))
               {
                  const uint32_t next_mem_flag_size = *reinterpret_cast<const uint32_t*>(_ptr + possible_size);
                  // if ALLOCed then done with contiguous free memory
                  if (next_mem_flag_size & _alloc_memory_mask)
                     break;

                  possible_size += (next_mem_flag_size & ~_alloc_memory_mask) + _size_marker;
               }

               if (all_or_nothing && possible_size < needed_size)
                  return false;

               // combine
               const uint32_t new_size = possible_size < needed_size ? possible_size : needed_size;
               size(new_size);

               if (possible_size > needed_size)
               {
                  const uint32_t freed_size = possible_size - needed_size - _size_marker;
                  buffer_ptr freed_remainder(_ptr + needed_size + _size_marker, freed_size, _heap_end);
                  freed_remainder.mark_free();
               }

               return new_size == needed_size;
            }

            char* _ptr;
            uint32_t _size;
            const char* const _heap_end;
         };

         uint32_t _heap_size;
         char* _heap;
         uint32_t _offset;
      };

      static const uint32_t _size_marker = sizeof(uint32_t);
      // allocate memory in 8 char blocks
      static const uint32_t _mem_block = 8;
      static const uint32_t _rem_mem_block_mask = _mem_block - 1;
      static const uint32_t _initial_heap_size = 8192;//32768;
      static const uint32_t _new_heap_size = 65536;
      // if sbrk is not called outside of this file, then this is the max times we can call it
      static const uint32_t _heaps_size = 16;
      char _initial_heap[_initial_heap_size];
      memory _available_heaps[_heaps_size];
      uint32_t _heaps_actual_size;
      uint32_t _active_heap;
      uint32_t _active_free_heap;
      static const uint32_t _alloc_memory_mask = 1 << 31;
   } memory_heap;

  /**
   * Allocate a block of memory.
   * @brief Allocate a block of memory.
   * @param size  Size of memory block
   *
   * Example:
   * @code
   * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
   * @endcode
   */
   inline void* malloc(uint32_t size)
   {
      return memory_heap.malloc(size);
   }

   /**
    * Allocate a block of memory.
    * @brief Allocate a block of memory.
    * @param size  Size of memory block
    *
    * Example:
    * @code
    * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
    * ...
    * uint64_t* bigger_int_buffer = realloc(int_buffer, 600 * sizeof(uint64_t));
    * @endcode
    */

   inline void* realloc(void* ptr, uint32_t size)
   {
      return memory_heap.realloc(ptr, size);
   }

   /**
    * Free a block of memory.
    * @brief Free a block of memory.
    * @param ptr  Pointer to memory block to free.
    *
    * Example:
    * @code
    * uint64_t* int_buffer = malloc(500 * sizeof(uint64_t));
    * ...
    * free(int_buffer);
    * @endcode
    */
    inline void free(void* ptr)
    {
       return memory_heap.free(ptr);
    }
   /// @} /// mathcppapi
}





template<typename T> struct remove_reference           { typedef T type; };
   template<typename T> struct remove_reference<T&>       { typedef T type; };
   template<typename T> struct remove_reference<const T&> { typedef T type; };
   ///@}

    /**
     * @ingroup types
     * needed for universal references since we build with --nostdlib and thus std::forward<T> is not available
     *  with forward<Args...>(args...) we always guarantee correctness of the calling code
    */
    template<typename T, typename U>
    constexpr decltype(auto) forward(U && u) noexcept
    {
       return static_cast<T &&>(u);
    }

    template< class T >
    constexpr typename remove_reference<T>::type&& move( T&& t ) noexcept {
       return static_cast<typename remove_reference<decltype(t)>::type&&>(t);
    }
    
//#include <eoslib/varint.hpp>

struct unsigned_int {
    unsigned_int( uint32_t v = 0 ):value(v){}

    template<typename T>
    unsigned_int( T v ):value(v){}

    //operator uint32_t()const { return value; }
    //operator uint64_t()const { return value; }

    template<typename T>
    operator T()const { return static_cast<T>(value); }

    unsigned_int& operator=( int32_t v ) { value = v; return *this; }
    
    uint32_t value;

    friend bool operator==( const unsigned_int& i, const uint32_t& v )     { return i.value == v; }
    friend bool operator==( const uint32_t& i, const unsigned_int& v )     { return i       == v.value; }
    friend bool operator==( const unsigned_int& i, const unsigned_int& v ) { return i.value == v.value; }

    friend bool operator!=( const unsigned_int& i, const uint32_t& v )     { return i.value != v; }
    friend bool operator!=( const uint32_t& i, const unsigned_int& v )     { return i       != v.value; }
    friend bool operator!=( const unsigned_int& i, const unsigned_int& v ) { return i.value != v.value; }

    friend bool operator<( const unsigned_int& i, const uint32_t& v )      { return i.value < v; }
    friend bool operator<( const uint32_t& i, const unsigned_int& v )      { return i       < v.value; }
    friend bool operator<( const unsigned_int& i, const unsigned_int& v )  { return i.value < v.value; }

    friend bool operator>=( const unsigned_int& i, const uint32_t& v )     { return i.value >= v; }
    friend bool operator>=( const uint32_t& i, const unsigned_int& v )     { return i       >= v.value; }
    friend bool operator>=( const unsigned_int& i, const unsigned_int& v ) { return i.value >= v.value; }
};

/**
 *  @brief serializes a 32 bit signed integer in as few bytes as possible
 *
 *  Uses the google protobuf algorithm for seralizing signed numbers
 */
struct signed_int {
    signed_int( int32_t v = 0 ):value(v){}
    operator int32_t()const { return value; }
    template<typename T>
    signed_int& operator=( const T& v ) { value = v; return *this; }
    signed_int operator++(int) { return value++; }
    signed_int& operator++(){ ++value; return *this; }

    int32_t value;

    friend bool operator==( const signed_int& i, const int32_t& v )    { return i.value == v; }
    friend bool operator==( const int32_t& i, const signed_int& v )    { return i       == v.value; }
    friend bool operator==( const signed_int& i, const signed_int& v ) { return i.value == v.value; }

    friend bool operator!=( const signed_int& i, const int32_t& v )    { return i.value != v; }
    friend bool operator!=( const int32_t& i, const signed_int& v )    { return i       != v.value; }
    friend bool operator!=( const signed_int& i, const signed_int& v ) { return i.value != v.value; }

    friend bool operator<( const signed_int& i, const int32_t& v )     { return i.value < v; }
    friend bool operator<( const int32_t& i, const signed_int& v )     { return i       < v.value; }
    friend bool operator<( const signed_int& i, const signed_int& v )  { return i.value < v.value; }

    friend bool operator>=( const signed_int& i, const int32_t& v )    { return i.value >= v; }
    friend bool operator>=( const int32_t& i, const signed_int& v )    { return i       >= v.value; }
    friend bool operator>=( const signed_int& i, const signed_int& v ) { return i.value >= v.value; }
};

//#include <eoslib/datastream.hpp>
//#include <eoslib/memory.hpp>

namespace eosio {
/**
 *  @brief A data stream for reading and writing data in the form of bytes
 */
template<typename T>
class datastream {
   public:
      datastream( T start, size_t s )
      :_start(start),_pos(start),_end(start+s){};
      
     /**
      *  Skips a specified number of bytes from this stream
      *  @brief Skips a specific number of bytes from this stream
      *  @param s The number of bytes to skip
      */
      inline void skip( size_t s ){ _pos += s; }
      
     /**
      *  Reads a specified number of bytes from the stream into a buffer
      *  @brief Reads a specified number of bytes from this stream into a buffer
      *  @param d pointer to the destination buffer
      *  @param s the number of bytes to read
      */
      inline bool read( char* d, size_t s ) {
        assert( size_t(_end - _pos) >= (size_t)s, "read" );
        memcpy( d, _pos, s );
        _pos += s;
        return true;
      }

     /**
      *  Writes a specified number of bytes into the stream from a buffer
      *  @brief Writes a specified number of bytes into the stream from a buffer
      *  @param d pointer to the source buffer
      *  @param s The number of bytes to write
      */
      inline bool write( const char* d, size_t s ) {
        assert( _end - _pos >= (int32_t)s, "write" );
        memcpy( _pos, d, s );
        _pos += s;
        return true;
      }
     
     /**
      *  Writes a byte into the stream
      *  @brief Writes a byte into the stream
      *  @param c byte to write
      */
      inline bool put(char c) { 
        assert( _pos < _end, "put" );
        *_pos = c; 
        ++_pos; 
        return true;
      }
     
     /**
      *  Reads a byte from the stream
      *  @brief Reads a byte from the stream
      *  @param c reference to destination byte
      */
      inline bool get( unsigned char& c ) { return get( *(char*)&c ); }
      inline bool get( char& c ) 
      {
        assert( _pos < _end, "get" );
        c = *_pos;
        ++_pos; 
        return true;
      }

     /**
      *  Retrieves the current position of the stream
      *  @brief Retrieves the current position of the stream
      *  @return the current position of the stream
      */
      T pos()const { return _pos; }
      inline bool valid()const { return _pos <= _end && _pos >= _start;  }
      
     /**
      *  Sets the position within the current stream
      *  @brief Sets the position within the current stream
      *  @param p offset relative to the origin
      */
      inline bool seekp(size_t p) { _pos = _start + p; return _pos <= _end; }

     /**
      *  Gets the position within the current stream
      *  @brief Gets the position within the current stream
      *  @return p the position within the current stream
      */
      inline size_t tellp()const      { return _pos - _start; }
      
     /**
      *  Returns the number of remaining bytes that can be read/skipped
      *  @brief Returns the number of remaining bytes that can be read/skipped
      *  @return number of remaining bytes
      */
      inline size_t remaining()const  { return _end - _pos; }
    private:
      T _start;
      T _pos;
      T _end;
};

/**
 *  @brief Specialization of datastream used to help determine the final size of a serialized value
 */
template<>
class datastream<size_t> {
   public:
     datastream( size_t init_size = 0):_size(init_size){};
     inline bool     skip( size_t s )                 { _size += s; return true;  }
     inline bool     write( const char* ,size_t s )  { _size += s; return true;  }
     inline bool     put(char )                      { ++_size; return  true;    }
     inline bool     valid()const                     { return true;              }
     inline bool     seekp(size_t p)                  { _size = p;  return true;  }
     inline size_t   tellp()const                     { return _size;             }
     inline size_t   remaining()const                 { return 0;                 }
  private:
     size_t _size;
};

/**
 *  Serialize a uint256 into a stream
 *  @brief Serialize a uint256
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint256 d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint256 from a stream
 *  @brief Deserialize a uint256
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint256& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint128_t into a stream
 *  @brief Serialize a uint128_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint128_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint128_t from a stream
 *  @brief Deserialize a uint128_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint128_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int128_t into a stream
 *  @brief Serialize a int128_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int128_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int128_t from a stream
 *  @brief Deserialize a int128_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int128_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int32_t into a stream
 *  @brief Serialize a int32_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int32_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int32_t from a stream
 *  @brief Deserialize a int32_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint32_t into a stream
 *  @brief Serialize a uint32_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint32_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint32_t from a stream
 *  @brief Deserialize a uint32_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint32_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int64_t into a stream
 *  @brief Serialize a int64_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int64_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int64_t from a stream
 *  @brief Deserialize a int64_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint64_t into a stream
 *  @brief Serialize a uint64_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint64_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint64_t from a stream
 *  @brief Deserialize a uint64_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint64_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int16_t into a stream
 *  @brief Serialize a int16_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int16_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int16_t from a stream
 *  @brief Deserialize a int16_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint16_t into a stream
 *  @brief Serialize a uint16_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint16_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint16_t from a stream
 *  @brief Deserialize a uint16_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint16_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a int8_t into a stream
 *  @brief Serialize a int8_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const int8_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a int8_t from a stream
 *  @brief Deserialize a int8_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, int8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}

/**
 *  Serialize a uint8_t into a stream
 *  @brief Serialize a uint8_t
 *  @param ds stream to write
 *  @param d value to serialize
 */
template<typename Stream>
inline datastream<Stream>& operator<<(datastream<Stream>& ds, const uint8_t d) {
  ds.write( (const char*)&d, sizeof(d) );
  return ds;
}
/**
 *  Deserialize a uint8_t from a stream
 *  @brief Deserialize a uint8_t
 *  @param ds stream to read
 *  @param d destination for deserialized value
 */
template<typename Stream>
inline datastream<Stream>& operator>>(datastream<Stream>& ds, uint8_t& d) {
  ds.read((char*)&d, sizeof(d) );
  return ds;
}
}


//string class
namespace eosio {
  /**
   * @brief Count the length of null terminated string (excluding the null terminated symbol)
   * Non-null terminated string need to be passed here, 
   * Otherwise it will not give the right length
   * @param cstr - null terminated string
   */
  inline size_t cstrlen(const char* cstr) {
    size_t len = 0;
    while(*cstr != '\0') {
      len++;
      cstr++;
    }
    return len;
  }
    
  class string {

  private:
    size_t size; // size of the string
    char* data; // underlying data
    bool own_memory; // true if the object is responsible to clean the memory
    uint32_t* refcount; // shared reference count to the underlying data

    // Release data if no more string reference to it
    void release_data_if_needed() {
      if (own_memory && refcount != nullptr) {
        (*refcount)--;
        if (*refcount == 0) {
          free(data);
        }
      }
    }

  public:
    /**
     * Default constructor
     */
    string() : size(0), data(nullptr), own_memory(false), refcount(nullptr) {
    }

    /**
     * Constructor to create string with reserved space
     * @param s size to be reserved (in number o)
     */
    string(size_t s) : size(s) {
      if (s == 0) {
        data = nullptr;
        own_memory = false;
        refcount = nullptr;
      } else {
        data = (char *)malloc(s * sizeof(char));
        own_memory = true;
        refcount = (uint32_t*)malloc(sizeof(uint32_t));
        *refcount = 1;
      }
    }

    /**
     * Constructor to create string with given data and size
     * @param d    data
     * @param s    size of the string (in number of bytes)
     * @param copy true to have the data copied and owned by the object
     */
    string(char* d, size_t s, bool copy) {
      assign(d, s, copy);
    }

    // Copy constructor
    string(const string& obj) {
      if (this != &obj) {
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
    }

    /**
     * @brief Constructor for string literal
     * Non-null terminated string need to be passed here, 
     * Otherwise it will have extraneous data
     * @param cstr - null terminated string
     */
    string(const char* cstr) {
      size = cstrlen(cstr) + 1;
      data = (char *)malloc(size * sizeof(char));
      memcpy(data, cstr, size * sizeof(char));
      own_memory = true;
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;
    }

    // Destructor
    ~string() {
      release_data_if_needed();
    }

    // Get size of the string (in number of bytes)
    const size_t get_size() const {
      return size;
    }

    // Get the underlying data of the string
    const char* get_data() const {
      return data;
    }

    // Check if it owns memory
    const bool is_own_memory() const {
      return own_memory;
    }

    // Get the ref count
    const uint32_t get_refcount() const {
      return *refcount;
    }

    /**
     * Assign string with new data and size
     * @param  d    data
     * @param  s    size (in number of bytes)
     * @param  copy true to have the data copied and owned by the object
     * @return      the current string
     */
    string& assign(char* d, size_t s, bool copy) {
      if (s == 0) {
        clear();
      } else {
        release_data_if_needed();
        if (copy) {
          data = (char *)malloc(s * sizeof(char));
          memcpy(data, d, s * sizeof(char));
          own_memory = true;
          refcount = (uint32_t*)malloc(sizeof(uint32_t));
          *refcount = 1;
        } else {
          data = d;
          own_memory = false;
          refcount = nullptr;
        }
        size = s;
      }

      return *this;
    }
    
    /**
     * Clear the content of the string
     */
    void clear() {
      release_data_if_needed();
      data = nullptr;
      size = 0;
      own_memory = false;
      refcount = nullptr;
    }

    /**
     * Create substring from current string
     * @param  offset      offset from the current string's data
     * @param  substr_size size of the substring
     * @param  copy        true to have the data copied and owned by the object
     * @return             substring of the current string
     */
    string substr(size_t offset, size_t substr_size, bool copy) {
      assert((offset < size) && (offset + substr_size < size), "out of bound");
      return string(data + offset, substr_size, copy);
    }

    char operator [] (const size_t index) {
      assert(index < size, "index out of bound");
      return *(data + index);
    }

    // Assignment operator
    string& operator = (const string& obj) {
      if (this != &obj) {
        release_data_if_needed();
        data = obj.data;
        size = obj.size;
        own_memory = obj.own_memory;
        refcount = obj.refcount;
        if (refcount != nullptr) (*refcount)++;
      }
      return *this;
    }

    /**
     * @brief Assignment operator for string literal
     * Non-null terminated string need to be passed here, 
     * Otherwise it will have extraneous data
     * @param cstr - null terminated string
     */
    string& operator = (const char* cstr) {
        release_data_if_needed();
        size = cstrlen(cstr) + 1;
        data = (char *)malloc(size * sizeof(char));
        memcpy(data, cstr, size * sizeof(char));
        own_memory = true;
        refcount = (uint32_t*)malloc(sizeof(uint32_t));
        *refcount = 1;
        return *this;
    }

    string& operator += (const string& str){
      assert((size + str.size > size) && (size + str.size > str.size), "overflow");

      char* new_data;
      size_t new_size;
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string, remove the \0 when concatenates
        new_size = size - 1 + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, (size - 1) * sizeof(char));
        memcpy(new_data + size - 1, str.data, str.size * sizeof(char));
      } else {
        new_size = size + str.size;
        new_data = (char *)malloc(new_size * sizeof(char));
        memcpy(new_data, data, size * sizeof(char));
        memcpy(new_data + size, str.data, str.size * sizeof(char));
      }

      // Release old data
      release_data_if_needed();
      // Assign new data
      data = new_data;

      size = new_size;
      own_memory = true;
      refcount = (uint32_t*)malloc(sizeof(uint32_t));
      *refcount = 1;

      return *this;
    }

    // Compare two strings
    // Return an integral value indicating the relationship between strings
    //   >0 if the first string is greater than the second string
    //   0 if both strings are equal
    //   <0 if the first string is smaller than the second string
    // The return value also represents the difference between the first character that doesn't match of the two strings
    int32_t compare(const string& str) const {
      int32_t result;
      if (size == str.size) {
        result = memcmp(data, str.data, size);
      } else if (size < str.size) {
        result = memcmp(data, str.data, size);
        if (result == 0) {
          // String is equal up to size of the shorter string, return the difference in byte of the next character
          result = 0 - (unsigned char)str.data[size];
        }
      } else if (size > str.size) {
        result = memcmp(data, str.data, str.size);
        if (result == 0) {
          // String is equal up to size of the shorter string, return the difference in byte of the next character
          result = (unsigned char)data[str.size];
        }
      }
      return result;
    }

    friend bool operator < (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) < 0;
    }

    friend bool operator > (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) > 0;
    }

    friend bool operator == (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) == 0;
    }

    friend bool operator != (const string& lhs, const string& rhs) {
      return lhs.compare(rhs) != 0;
    }

    friend string operator + (string lhs, const string& rhs) {
      return lhs += rhs;
    }

    void print() const {
      if (size > 0 && *(data + size - 1) == '\0') {
        // Null terminated string
        //prints(data);
      } else {
        // Non null terminated string
        // We need to specify the size of string so it knows where to stop
        //prints_l(data, size);
      }
   }
  };

}


namespace eosio { namespace raw { 

  template<typename Stream, typename Arg0, typename... Args>  void pack( Stream& s, Arg0&& a0, Args&&... args );
  // template<typename Stream, typename T>  void pack( Stream& s, T&& v );
  // template<typename Stream, typename T>  void unpack( Stream& s, T& v );
  // template<typename Stream>  void pack( Stream& s, signed_int v );
  // template<typename Stream>  void pack( Stream& s, unsigned_int v );
  // template<typename Stream>  void unpack( Stream& s, signed_int& vi );
  // template<typename Stream>  void unpack( Stream& s, unsigned_int& vi );
  // template<typename Stream>  void pack( Stream& s, const bytes& value );
  // template<typename Stream>  void unpack( Stream& s, bytes& value );
  // template<typename Stream>  void pack( Stream& s, const public_key& value );
  // template<typename Stream>  void unpack( Stream& s, public_key& value );
  // template<typename Stream>  void pack( Stream& s, const string& v );
  // template<typename Stream>  void unpack( Stream& s, string& v);
  template<typename Stream>  void pack( Stream& s, const fixed_string32& v);
  // template<typename Stream>  void unpack( Stream& s, fixed_string32& v);
  // template<typename Stream>  void pack( Stream& s, const fixed_string16& v );
  // template<typename Stream>  void unpack( Stream& s, fixed_string16& v);
  // template<typename Stream>  void pack( Stream& s, const price& v );
  // template<typename Stream>  void unpack( Stream& s, price& v);
  // template<typename Stream>  void pack( Stream& s, const asset& v );
  // template<typename Stream>  void unpack( Stream& s, asset& v);
  // template<typename Stream>  void pack( Stream& s, const bool& v );
  // template<typename Stream>  void unpack( Stream& s, bool& v );
  // template<typename T> size_t pack_size( T&& v );
  // template<typename T> bytes pack(  T&& v );
  // template<typename T> void pack( char* d, uint32_t s, T&& v );
  // template<typename T> T unpack( const char* d, uint32_t s );
  // template<typename T>  void unpack( const char* d, uint32_t s, T& v );
} } // namespace eosio::raw

namespace eosio {
    namespace raw {    
  /**
   *  Serialize a list of values into a stream
   *  @param s    stream to write
   *  @param a0   value to be serialized
   *  @param args other values to be serialized
   */
  template<typename Stream, typename Arg0, typename... Args>
  void pack( Stream& s, Arg0&& a0, Args&&... args ) {
      pack( s, forward<Arg0>(a0) );
      pack( s, forward<Args>(args)... );
  }

  /**
   *  Serialize a value into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream, typename T>
   void pack( Stream& s, T && v )
   {
      s << forward<T>(v);
   }

  /**
   *  Deserialize a value from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream, typename T>
   void unpack( Stream& s, T& v )
   {
      s >> v;
   }
   
  /**
   *  Serialize a signed_int into a stream
   *  @param s stream to write
   *  @param v value to be serialized (must use eosio::move to pass it)
   */
   template<typename Stream>  void pack( Stream& s, signed_int v ) {
      uint32_t val = (v.value<<1) ^ (v.value>>31);
      do {
         uint8_t b = uint8_t(val) & 0x7f;
         val >>= 7;
         b |= ((val > 0) << 7);
         s.write((char*)&b,1);//.put(b);
      } while( val );
   }
  
  /**
   *  Serialize an unsigned_int into a stream
   *  @param s stream to write
   *  @param v value to be serialized ( must use eosio::move to pass it)
   */
   template<typename Stream>  void pack( Stream& s, unsigned_int v ) {
      uint64_t val = v.value;
      do {
         uint8_t b = uint8_t(val) & 0x7f;
         val >>= 7;
         b |= ((val > 0) << 7);
         s.write((char*)&b,1);//.put(b);
      }while( val );
   }

  /**
   *  Deserialize a signed_int from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, signed_int& vi ) {
      uint32_t v = 0; char b = 0; int by = 0;
      do {
         s.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = ((v>>1) ^ (v>>31)) + (v&0x01);
      vi.value = v&0x01 ? vi.value : -vi.value;
      vi.value = -vi.value;
   }

  /**
   *  Deserialize an unsigned_int from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, unsigned_int& vi ) {
      uint64_t v = 0; char b = 0; uint8_t by = 0;
      do {
         s.get(b);
         v |= uint32_t(uint8_t(b) & 0x7f) << by;
         by += 7;
      } while( uint8_t(b) & 0x80 );
      vi.value = static_cast<uint32_t>(v);
   }

  /**
   *  Serialize a bytes struct into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const bytes& value ) {
      eosio::raw::pack( s, move(unsigned_int((uint32_t)value.len)) );
      if( value.len )
         s.write( (char *)value.data, (uint32_t)value.len );
   }
   
  /**
   *  Deserialize a bytes struct from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, bytes& value ) {
      unsigned_int size; eosio::raw::unpack( s, size );
      value.len = size.value;
      if( value.len ) {
         value.data = (uint8_t *)eosio::malloc(value.len);
         s.read( (char *)value.data, value.len );
      }
   }

  /**
   *  Serialize a public_key into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const public_key& value ) {
      s.write( (char *)value.data, sizeof(public_key) );
   }
   
  /**
   *  Deserialize a public_key from a stream
   *  @param s stream to read
   *  @param v destination of deserialized public_key
   */
   template<typename Stream>  void unpack( Stream& s, public_key& value ) {
      s.read( (char *)value.data, sizeof(public_key) );
   }

  /**
   *  Serialize a string into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const string& v )  {
      auto size = v.get_size();
      eosio::raw::pack( s, move(unsigned_int(size)));
      if( size ) s.write( v.get_data(), size );
   }

  /**
   *  Deserialize a string from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, string& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      v.assign((char*)s.pos(), size.value, true);
      s.skip(size.value);
   }

  /**
   *  Serialize a fixed_string32 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const fixed_string32& v )  {
      auto size = v.len;
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }

  /**
   *  Deserialize a fixed_string32 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, fixed_string32& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      assert(size.value <= 32, "unpack fixed_string32");
      s.read( (char *)v.str, size );
      v.len = size;
   }

  /**
   *  Serialize a fixed_string16 into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const fixed_string16& v )  {
      auto size = v.len;
      eosio::raw::pack( s, unsigned_int(size));
      if( size ) s.write( v.str, size );
   }

  /**
   *  Deserialize a fixed_string16 from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, fixed_string16& v)  {
      unsigned_int size; eosio::raw::unpack( s, size );
      assert(size.value <= 16, "unpack fixed_string16");
      s.read( (char *)v.str, size );
      v.len = size;
   }
  
  /**
   *  Serialize a price into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const price& v )  {
      eosio::raw::pack(s, v.base);
      eosio::raw::pack(s, v.quote);
   }

  /**
   *  Deserialize a price from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, price& v)  {
      eosio::raw::unpack(s, v.base);
      eosio::raw::unpack(s, v.quote);
   }
  /**
   *  Serialize an asset into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const asset& v )  {
      eosio::raw::pack(s, v.amount);
      eosio::raw::pack(s, v.symbol);
   }

  /**
   *  Deserialize an asset from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, asset& v)  {
      eosio::raw::unpack(s, v.amount);
      eosio::raw::unpack(s, v.symbol);
   }
  /**
   *  Serialize a bool into a stream
   *  @param s stream to write
   *  @param v value to be serialized
   */
   template<typename Stream>  void pack( Stream& s, const bool& v ) 
   { 
     eosio::raw::pack( s, uint8_t(v) );
   }

  /**
   *  Deserialize a bool from a stream
   *  @param s stream to read
   *  @param v destination of deserialized value
   */
   template<typename Stream>  void unpack( Stream& s, bool& v )
   {
      uint8_t b;
      eosio::raw::unpack( s, b );
      assert( (b & ~1) == 0, "unpack bool" );
      v=(b!=0);
   }
  
  /** Calculates the serialized size of a value
   *  @return serialized size of the value
   *  @param v value to calculate its serialized size
   */
   template<typename T>
   size_t pack_size( T&& v )
   {
      datastream<size_t> ps;
      eosio::raw::pack(ps,forward<T>(v));
      return ps.tellp();
   }
  
  /** Serialize a value into a bytes struct
   *  @return bytes struct with the serialized value 
   *  @param v value to be serialized
   */
   template<typename T>
   bytes pack(  T&& v ) {
      datastream<size_t> ps;
      eosio::raw::pack(ps, forward<T>(v));
      bytes b;
      b.len = ps.tellp();
      b.data = (uint8_t*)eosio::malloc(b.len);

      if( b.len ) {
         datastream<char*>  ds( (char*)b.data, b.len );
         eosio::raw::pack(ds,forward<T>(v));
      }
      return b;
   }

  /** Serialize a value into a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v value to be serialized
   */
   template<typename T>
   void pack( char* d, uint32_t s, T&& v ) {
      datastream<char*> ds(d,s);
      eosio::raw::pack(ds,forward<T>(v));
    }

  /** Deserialize a value from a buffer
   *  @return the deserialized value 
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   */
   template<typename T>
   T unpack( const char* d, uint32_t s )
   {
      T v;
      datastream<const char*>  ds( d, s );
      eosio::raw::unpack(ds,v);
      return v;
   }
  
  /** Deserialize a value from a buffer
   *  @param d pointer to the buffer
   *  @param s size of the buffer
   *  @param v destination of deserialized value
   */
   template<typename T>
   void unpack( const char* d, uint32_t s, T& v )
   {
      datastream<const char*>  ds( d, s );
      eosio::raw::unpack(ds,v);
   }

} } // namespace eosio::raw



int main() { 
  fixed_string32 tmp;
  tmp.len = 8;
  memcpy(tmp.str , "Vladimir", tmp.len);
  const fixed_string32 tmp2  = tmp;
  bytes b = eosio::raw::pack(static_cast< decltype(tmp2) const&>(tmp2));
  return b.len;
 
}
