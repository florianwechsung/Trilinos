#ifndef KOKKOS_TEST_GLOBAL_TO_LOCAL_IDS_HPP
#define KOKKOS_TEST_GLOBAL_TO_LOCAL_IDS_HPP

#include <Kokkos_View.hpp>
#include <Kokkos_UnorderedMap.hpp>
#include <vector>
#include <algorithm>

#include <impl/Kokkos_Timer.hpp>

// This test will simulate global ids

namespace Performance {

static const unsigned begin_id_size = 256u;
static const unsigned end_id_size = 1u << 25;
static const unsigned id_step = 2u;

union helper
{
  uint32_t word;
  uint8_t byte[4];
};


template <typename Device>
struct generate_ids
{
  typedef Device device_type;
  typedef typename device_type::size_type size_type;
  typedef Kokkos::View<uint32_t*,device_type> local_id_view;

  local_id_view local_2_global;

  generate_ids( local_id_view & ids)
    : local_2_global(ids)
  {
    Kokkos::parallel_for(local_2_global.size(), *this);
  }


  KOKKOS_INLINE_FUNCTION
  void operator()(size_type i) const
  {

    helper x = {static_cast<uint32_t>(i)};

    // shuffle the bytes of i to create a unique, semi-random global_id
    x.word = ~x.word;

    uint8_t tmp = x.byte[3];
    x.byte[3] = x.byte[1];
    x.byte[1] = tmp;

    tmp = x.byte[2];
    x.byte[2] = x.byte[0];
    x.byte[0] = tmp;

    local_2_global[i] = x.word;
  }

};

template <typename Device>
struct fill_map
{
  typedef Device device_type;
  typedef typename device_type::size_type size_type;
  typedef Kokkos::View<const uint32_t*,device_type, Kokkos::MemoryRandomRead> local_id_view;
  typedef Kokkos::UnorderedMap<uint32_t,size_type,device_type> global_id_view;

  global_id_view global_2_local;
  local_id_view local_2_global;

  fill_map( global_id_view gIds, local_id_view lIds)
    : global_2_local(gIds) , local_2_global(lIds)
  {
    Kokkos::parallel_for(local_2_global.size(), *this);
  }

  KOKKOS_INLINE_FUNCTION
  void operator()(size_type i) const
  {
    global_2_local.insert( local_2_global[i], i);
  }

};

template <typename Device>
struct find_test
{
  typedef Device device_type;
  typedef typename device_type::size_type size_type;
  typedef Kokkos::View<const uint32_t*,device_type, Kokkos::MemoryRandomRead> local_id_view;
  typedef Kokkos::UnorderedMap<const uint32_t, const size_type,device_type> global_id_view;
  typedef typename global_id_view::const_pointer const_pointer;

  global_id_view global_2_local;
  local_id_view local_2_global;

  typedef size_t value_type;

  find_test( global_id_view gIds, local_id_view lIds, value_type & num_errors)
    : global_2_local(gIds) , local_2_global(lIds)
  {
    Kokkos::parallel_reduce(local_2_global.size(), *this, num_errors);
  }

  KOKKOS_INLINE_FUNCTION
  void init(value_type & v) const
  { v = 0; }

  KOKKOS_INLINE_FUNCTION
  void join(volatile value_type & dst, volatile value_type const & src) const
  { dst += src; }

  KOKKOS_INLINE_FUNCTION
  void operator()(size_type i, value_type & num_errors) const
  {
    const_pointer ptr = global_2_local.find( local_2_global[i] );

    if (ptr->first != local_2_global[i] || ptr->second != i)
      ++num_errors;
  }

};

template <typename Device>
void test_global_to_local_ids(unsigned num_ids)
{

  typedef Device device_type;
  typedef typename device_type::size_type size_type;

  typedef Kokkos::View<uint32_t*,device_type> local_id_view;
  typedef Kokkos::UnorderedMap<uint32_t,size_type,device_type> global_id_view;

  //size
  std::cout << num_ids << ", ";

  double elasped_time = 0;
  Kokkos::Impl::Timer timer;

  local_id_view local_2_global("local_ids", num_ids);
  global_id_view global_2_local((3u*num_ids)/2u);

  //create
  elasped_time = timer.seconds();
  std::cout << elasped_time << ", ";
  timer.reset();

  // generate unique ids
  {
    generate_ids<Device> gen(local_2_global);
  }
  Device::fence();
  // generate
  elasped_time = timer.seconds();
  std::cout << elasped_time << ", ";
  timer.reset();

  {
    fill_map<Device> fill(global_2_local, local_2_global);
  }
  Device::fence();

  // fill
  elasped_time = timer.seconds();
  std::cout << elasped_time << ", ";
  timer.reset();


  size_t num_errors = 0;
  for (int i=0; i<100; ++i)
  {
    find_test<Device> find(global_2_local, local_2_global,num_errors);
  }
  Device::fence();

  // find
  elasped_time = timer.seconds();
  std::cout << elasped_time << std::endl;

  ASSERT_EQ( num_errors, 0u);
}


} // namespace Performance


#endif //KOKKOS_TEST_GLOBAL_TO_LOCAL_IDS_HPP
