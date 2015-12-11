// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef KOKKOS_EXPERIMENTAL_VIEW_SACADO_FAD_HPP
#define KOKKOS_EXPERIMENTAL_VIEW_SACADO_FAD_HPP

#include <Kokkos_Core_fwd.hpp>

#if defined( KOKKOS_USING_EXPERIMENTAL_VIEW )

#include <Kokkos_Macros.hpp>
#include <Sacado_ConfigDefs.h>

#include <impl/KokkosExp_ViewMapping.hpp>

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Experimental {
namespace Impl {

struct ViewSpecializeSacadoFad {};

template< class ... Args >
struct is_ViewSpecializeSacadoFad { enum { value = false }; };

template< class D , class ... P , class ... Args >
struct is_ViewSpecializeSacadoFad< Kokkos::View<D,P...> , Args... > {
  enum { value =
    std::is_same< typename Kokkos::ViewTraits<D,P...>::specialize
                , ViewSpecializeSacadoFad >::value
    &&
    ( ( sizeof...(Args) == 0 ) ||
      is_ViewSpecializeSacadoFad< Args... >::value ) };
};

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

namespace Kokkos {

// Overload of deep_copy for Fad views intializing to a constant scalar
template< class DT, class ... DP >
void deep_copy(
  const View<DT,DP...> & view ,
  const typename View<DT,DP...>::array_type::value_type & value
  , typename std::enable_if<(
  std::is_same< typename ViewTraits<DT,DP...>::specialize
              , Kokkos::Experimental::Impl::ViewSpecializeSacadoFad >::value
  )>::type * = 0 )
{
  static_assert(
    std::is_same< typename ViewTraits<DT,DP...>::value_type ,
                  typename ViewTraits<DT,DP...>::non_const_value_type >::value
    , "Can only deep copy into non-const type" );

  Kokkos::Experimental::Impl::ViewFill< View<DT,DP...> >( view , value );
}

// Overload of deep_copy for Fad views intializing to a constant Fad
template< class DT, class ... DP >
void deep_copy(
  const View<DT,DP...> & view ,
  const typename View<DT,DP...>::value_type & value
  , typename std::enable_if<(
  std::is_same< typename ViewTraits<DT,DP...>::specialize
              , Kokkos::Experimental::Impl::ViewSpecializeSacadoFad >::value
  )>::type * = 0 )
{
  static_assert(
    std::is_same< typename ViewTraits<DT,DP...>::value_type ,
                  typename ViewTraits<DT,DP...>::non_const_value_type >::value
    , "Can only deep copy into non-const type" );

  static_assert(
    Sacado::StaticSize< typename View<DT,DP...>::value_type >::value
    ||
    std::is_same< Kokkos::Impl::ActiveExecutionMemorySpace
                , Kokkos::HostSpace >::value
    , "Deep copy from a FAD type must be statically sized or host space" );

  Kokkos::Experimental::Impl::ViewFill< View<DT,DP...> >( view , value );
}

/* Specialize for deep copy of FAD */
template< class DT , class ... DP , class ST , class ... SP >
inline
void deep_copy( const View<DT,DP...> & dst ,
                const View<ST,SP...> & src
  , typename std::enable_if<(
  std::is_same< typename ViewTraits<DT,DP...>::specialize
              , Kokkos::Experimental::Impl::ViewSpecializeSacadoFad >::value
  &&
  std::is_same< typename ViewTraits<ST,SP...>::specialize
              , Kokkos::Experimental::Impl::ViewSpecializeSacadoFad >::value
  )>::type * = 0 )
{
  static_assert(
    std::is_same< typename ViewTraits<DT,DP...>::value_type ,
                  typename ViewTraits<DT,DP...>::non_const_value_type >::value
    , "Deep copy destination must be non-const" );

  static_assert(
    ( unsigned(ViewTraits<DT,DP...>::rank) ==
      unsigned(ViewTraits<ST,SP...>::rank) )
    , "Deep copy destination and source must have same rank" );

  Kokkos::deep_copy(
    typename View<DT,DP...>::array_type( dst ) ,
    typename View<ST,SP...>::array_type( src ) );
}

} // namespace Kokkos

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Experimental {
namespace Impl {

template< class DataType , class ArrayLayout , class ScalarType , unsigned DimFad >
struct FadViewDataAnalysis
{
private:

  typedef ViewArrayAnalysis< DataType > array_analysis ;

public:

  // Specialized view data mapping:
  typedef ViewSpecializeSacadoFad specialize ;

  typedef typename array_analysis::dimension             dimension ;
  typedef typename array_analysis::value_type            value_type ;
  typedef typename array_analysis::const_value_type      const_value_type ;
  typedef typename array_analysis::non_const_value_type  non_const_value_type ;

  // Generate analogous multidimensional array specification type.
  typedef typename
    ViewDataType< value_type , dimension >::type  type ;
  typedef typename
    ViewDataType< const_value_type , dimension >::type  const_type ;
  typedef typename
    ViewDataType< non_const_value_type , dimension >::type  non_const_type ;

private:

  // A const ?
  enum { is_const = std::is_same< value_type , const_value_type >::value };

  // The unwrapped scalar types:
  typedef typename
    std::conditional< is_const , const ScalarType , ScalarType >::type
      scalar_type ;

  typedef ScalarType        non_const_scalar_type ;
  typedef const ScalarType  const_scalar_type ;

  // Append the FAD static dimension
  typedef typename array_analysis::dimension::
    template append<( DimFad ? DimFad + 1 : 0 )>::type
      scalar_dimension ;

public:

  // Generate "flattened" multidimensional array specification type.
  typedef typename
    ViewDataType< scalar_type , scalar_dimension >::type array_scalar_type ;

  typedef typename
    ViewDataType< const_scalar_type , scalar_dimension >::type
      const_array_scalar_type ;

  typedef typename
    ViewDataType< non_const_scalar_type , scalar_dimension >::type
      non_const_array_scalar_type ;
};

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

//----------------------------------------------------------------------------

namespace Sacado {
namespace Fad         { template< typename > class DFad ; }
namespace CacheFad    { template< typename > class DFad ; }
namespace ELRFad      { template< typename > class DFad ; }
namespace ELRCacheFad { template< typename > class DFad ; }

namespace Fad         { template< typename , int > class SFad ; }
namespace CacheFad    { template< typename , int > class SFad ; }
namespace ELRFad      { template< typename , int > class SFad ; }
namespace ELRCacheFad { template< typename , int > class SFad ; }

namespace Fad         { template< typename , int > class SLFad ; }
namespace CacheFad    { template< typename , int > class SLFad ; }
namespace ELRFad      { template< typename , int > class SLFad ; }
namespace ELRCacheFad { template< typename , int > class SLFad ; }
}

namespace Kokkos {
namespace Experimental {
namespace Impl {

#define KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD( NS ) \
template< class DataType , class ArrayLayout , typename ScalarType > \
struct ViewDataAnalysis \
  < DataType     /* Original view data type */ \
  , ArrayLayout \
  , Sacado:: NS ::DFad< ScalarType > \
  > : public FadViewDataAnalysis< DataType, ArrayLayout, ScalarType , 0 > {}; \
\
template< class DataType , class ArrayLayout , typename ScalarType , int N > \
struct ViewDataAnalysis \
  < DataType     /* Original view data type */ \
  , ArrayLayout \
  , Sacado:: NS ::SFad< ScalarType , N > \
  > : public FadViewDataAnalysis< DataType, ArrayLayout, ScalarType , \
       int(Sacado::StaticSize< Sacado:: NS ::SFad< ScalarType , N > >::value) \
       > {}; \
\
template< class DataType , class ArrayLayout , typename ScalarType , int N > \
struct ViewDataAnalysis \
  < DataType     /* Original view data type */ \
  , ArrayLayout \
  , Sacado:: NS ::SLFad< ScalarType , N > \
  > : public FadViewDataAnalysis< DataType, ArrayLayout, ScalarType , \
       int(Sacado::StaticSize< Sacado:: NS ::SLFad< ScalarType , N > >::value) \
       > {}; \

KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD( Fad )
KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD( CacheFad )
KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD( ELRFad )
KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD( ELRCacheFad )

#undef KOKKOS_VIEW_DATA_ANALYSIS_SACADO_FAD

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Experimental {
namespace Impl {

template< class Traits >
class ViewMapping< Traits , /* View internal mapping */
  typename std::enable_if<
    ( std::is_same< typename Traits::specialize
                  , ViewSpecializeSacadoFad >::value
      &&
      ( std::is_same< typename Traits::array_layout
                    , Kokkos::LayoutLeft >::value
        ||
        std::is_same< typename Traits::array_layout
                    , Kokkos::LayoutRight >::value
        ||
        std::is_same< typename Traits::array_layout
                    , Kokkos::LayoutStride >::value
      )
    )>::type >
{
private:

  template< class , class ... > friend class ViewMapping ;
  template< class , class ... > friend class Kokkos::Experimental::View ;

  typedef typename Traits::value_type  fad_type ;
  typedef typename Sacado::ValueType< fad_type >::type fad_value_type ;
  typedef typename
    std::add_const< fad_value_type >::type  const_fad_value_type ;

  enum { FadStaticDimension = Sacado::StaticSize< fad_type >::value };

  // Only LayoutRight has a static stride one
  enum { FadStaticStride =
    std::is_same< typename Traits::array_layout
                , Kokkos::LayoutRight >::value ? 1 : 0 };

  typedef fad_value_type * handle_type ;

  typedef ViewArrayAnalysis< typename Traits::data_type > array_analysis ;

  // Append the fad dimension for the internal offset mapping.
  typedef ViewOffset
    < typename array_analysis::dimension::
        template append<( FadStaticDimension ? FadStaticDimension + 1 : 0 )>::type
    , typename Traits::array_layout
    , void
    >  offset_type ;

  handle_type  m_handle ;
  offset_type  m_offset ;
  unsigned     m_fad_size ;

public:

  //----------------------------------------
  // Domain dimensions

  enum { Rank = Traits::dimension::rank };

  // Using the internal offset mapping so limit to public rank:
  template< typename iType >
  KOKKOS_INLINE_FUNCTION constexpr size_t extent( const iType & r ) const
    { return unsigned(r) < unsigned(Rank) ? m_offset.m_dim.extent(r) : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_0() const
    { return 0 < unsigned(Rank) ? m_offset.dimension_0() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_1() const
    { return 1 < unsigned(Rank) ? m_offset.dimension_1() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_2() const
    { return 2 < unsigned(Rank) ? m_offset.dimension_2() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_3() const
    { return 3 < unsigned(Rank) ? m_offset.dimension_3() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_4() const
    { return 4 < unsigned(Rank) ? m_offset.dimension_4() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_5() const
    { return 5 < unsigned(Rank) ? m_offset.dimension_5() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_6() const
    { return 6 < unsigned(Rank) ? m_offset.dimension_6() : 1 ; }

  KOKKOS_INLINE_FUNCTION constexpr size_t dimension_7() const
    { return 7 < unsigned(Rank) ? m_offset.dimension_7() : 1 ; }

  // Can only be regular layout with uniform striding
  // when LayoutRight with contiguous values so not guaranteed true.
  using is_regular = std::false_type ;

  KOKKOS_INLINE_FUNCTION constexpr size_t stride_0() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_1() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_2() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_3() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_4() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_5() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_6() const { return 0 ; }
  KOKKOS_INLINE_FUNCTION constexpr size_t stride_7() const { return 0 ; }

  //----------------------------------------
  // Range of mapping

  // Return type of reference operators
  typedef typename
    Sacado::ViewFadType< fad_type , FadStaticDimension , FadStaticStride >::type  reference_type ;

  /** \brief Pointer to underlying memory type */
  typedef fad_value_type * pointer_type ;

  /** \brief  Span of the mapped range : [ data() .. data() + span() ) */
  KOKKOS_INLINE_FUNCTION constexpr size_t span() const
    { return m_offset.span(); }

  /** \brief  The mapped range span cannot be guaranteed contiguous */
  KOKKOS_INLINE_FUNCTION constexpr bool span_is_contiguous() const
    { return false ; }

  /** \brief Raw data access */
  KOKKOS_INLINE_FUNCTION constexpr pointer_type data() const
    { return m_handle ; }

  //----------------------------------------

  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference() const
    { return reference_type( m_handle
                           , m_fad_size
                           , 1 ); }

  template< typename I0 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type
  reference( const I0 & i0 ) const
    { return reference_type( m_handle + m_offset(i0,0)
                           , m_fad_size
                           , m_offset.stride_1() ); }

  template< typename I0 , typename I1 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 ) const
    { return reference_type( m_handle + m_offset(i0,i1,0)
                           , m_fad_size
                           , m_offset.stride_2() ); }


  template< typename I0 , typename I1 , typename I2 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 , const I2 & i2 ) const
    { return reference_type( m_handle + m_offset(i0,i1,i2,0)
                           , m_fad_size
                           , m_offset.stride_3() ); }

  template< typename I0 , typename I1 , typename I2 , typename I3 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 , const I2 & i2 , const I3 & i3 ) const
    { return reference_type( m_handle + m_offset(i0,i1,i2,i3,0)
                           , m_fad_size
                           , m_offset.stride_4() ); }

  template< typename I0 , typename I1 , typename I2 , typename I3
          , typename I4 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 , const I2 & i2 , const I3 & i3
                          , const I4 & i4 ) const
    { return reference_type( m_handle + m_offset(i0,i1,i2,i3,i4,0)
                           , m_fad_size
                           , m_offset.stride_5() ); }

  template< typename I0 , typename I1 , typename I2 , typename I3
          , typename I4 , typename I5 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 , const I2 & i2 , const I3 & i3
                          , const I4 & i4 , const I5 & i5 ) const
    { return reference_type( m_handle + m_offset(i0,i1,i2,i3,i4,i5,0)
                           , m_fad_size
                           , m_offset.stride_6() ); }


  template< typename I0 , typename I1 , typename I2 , typename I3
          , typename I4 , typename I5 , typename I6 >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type reference( const I0 & i0 , const I1 & i1 , const I2 & i2 , const I3 & i3
                          , const I4 & i4 , const I5 & i5 , const I6 & i6 ) const
    { return reference_type( m_handle + m_offset(i0,i1,i2,i3,i4,i5,i6,0)
                           , m_fad_size
                           , m_offset.stride_7() ); }

  //----------------------------------------

  /** \brief  Span, in bytes, of the potentially non-contiguous referenced memory */
  KOKKOS_INLINE_FUNCTION constexpr size_t memory_span() const
  {
    return m_offset.span() * sizeof(fad_value_type);
  }

  /** \brief  Span, in bytes, of the required memory */
  template< bool AllowPadding >
  KOKKOS_INLINE_FUNCTION
  static constexpr size_t memory_span
    ( const std::integral_constant<bool,AllowPadding> &
    , const size_t N0 , const size_t N1 , const size_t N2 , const size_t N3
    , const size_t N4 , const size_t N5 , const size_t N6 , const size_t N7
    )
    {
      // Do not introduce padding...
      return offset_type( std::integral_constant< unsigned , 0 >()
                        , N0, N1, N2, N3, N4, N5, N6, N7
                        ).span() * sizeof(fad_value_type);
    }

  //----------------------------------------

  ~ViewMapping() = default ;
  ViewMapping() = default ;

  ViewMapping( const ViewMapping & ) = default ;
  ViewMapping & operator = ( const ViewMapping & ) = default ;

  ViewMapping( ViewMapping && ) = default ;
  ViewMapping & operator = ( ViewMapping && ) = default ;

  template< bool AllowPadding >
  KOKKOS_INLINE_FUNCTION
  ViewMapping
    ( pointer_type ptr
    , const std::integral_constant<bool,AllowPadding> &
    , const size_t N0 , const size_t N1 , const size_t N2 , const size_t N3
    , const size_t N4 , const size_t N5 , const size_t N6 , const size_t N7
    )
    : m_handle( reinterpret_cast< handle_type >( ptr ) )
    , m_offset( std::integral_constant< unsigned , 0 >()
              , N0, N1, N2, N3, N4, N5, N6, N7 )
    // Query m_offset, not input, in case of static dimension
    , m_fad_size(
       ( Rank == 0 ? m_offset.dimension_0() :
       ( Rank == 1 ? m_offset.dimension_1() :
       ( Rank == 2 ? m_offset.dimension_2() :
       ( Rank == 3 ? m_offset.dimension_3() :
       ( Rank == 4 ? m_offset.dimension_4() :
       ( Rank == 5 ? m_offset.dimension_5() :
       ( Rank == 6 ? m_offset.dimension_6() :
                     m_offset.dimension_7() ))))))) - 1 )
    {}

  //----------------------------------------
  // If the View is to construct or destroy the elements.

  template< class ExecSpace >
  void construct( const ExecSpace & space ) const
    {
      typedef ViewValueFunctor< fad_value_type , ExecSpace > FunctorType ;

      (void) FunctorType( space , m_handle , m_offset.span() , FunctorType::CONSTRUCT );
    }

  template< class ExecSpace >
  void destroy( const ExecSpace & space ) const
    {
      typedef ViewValueFunctor< fad_value_type , ExecSpace > FunctorType ;

      (void) FunctorType( space , m_handle , m_offset.span() , FunctorType::DESTROY );
    }

};

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Experimental {
namespace Impl {

/**\brief  Assign compatible Sacado FAD view mappings.
 *
 *  View<FAD>      = View<FAD>
 *  View<ordinary> = View<FAD>
 *  (TBD)  View<FAD> = View<ordinary>
 */
template< class DstTraits , class SrcTraits >
class ViewMapping< DstTraits , SrcTraits ,
  typename std::enable_if<(
    std::is_same< typename DstTraits::memory_space
                , typename SrcTraits::memory_space >::value
    &&
    // Destination view has FAD or ordinary
    ( std::is_same< typename DstTraits::specialize
                , ViewSpecializeSacadoFad >::value
      ||
      std::is_same< typename DstTraits::specialize , void >::value
    )
    &&
    // Source view has FAD only
    std::is_same< typename SrcTraits::specialize
                , ViewSpecializeSacadoFad >::value
  )>::type >
{
public:

  enum { is_assignable = true };

  typedef Kokkos::Experimental::Impl::SharedAllocationTracker  TrackType ;
  typedef ViewMapping< DstTraits , void >  DstType ;
  typedef ViewMapping< SrcTraits , void >  SrcFadType ;

  template< class S , class D >
  KOKKOS_INLINE_FUNCTION static
  typename std::enable_if<( 
    std::is_same< S , ViewSpecializeSacadoFad >::value
    )>::type
  assign_fad_size( D & dst , unsigned size )
    { dst.m_fad_size = size ; }

  template< class S , class D >
  KOKKOS_INLINE_FUNCTION static
  typename std::enable_if<(
    ! std::is_same< S , ViewSpecializeSacadoFad >::value 
    )>::type
  assign_fad_size( D & , unsigned ) {}

  template< class DstType >
  KOKKOS_INLINE_FUNCTION static
  void assign( DstType & dst
             , const SrcFadType & src
             , const TrackType & )
    {
      static_assert(
        (
          std::is_same< typename DstTraits::array_layout
                      , Kokkos::LayoutLeft >::value ||
          std::is_same< typename DstTraits::array_layout
                      , Kokkos::LayoutRight >::value ||
          std::is_same< typename DstTraits::array_layout
                      , Kokkos::LayoutStride >::value
        )
        &&
        (
          std::is_same< typename SrcTraits::array_layout
                      , Kokkos::LayoutLeft >::value ||
          std::is_same< typename SrcTraits::array_layout
                      , Kokkos::LayoutRight >::value ||
          std::is_same< typename SrcTraits::array_layout
                      , Kokkos::LayoutStride >::value
        )
        , "View of FAD requires LayoutLeft, LayoutRight, or LayoutStride" );

      static_assert(
        std::is_same< typename DstTraits::array_layout
                    , typename SrcTraits::array_layout >::value ||
        std::is_same< typename DstTraits::array_layout
                    , Kokkos::LayoutStride >::value ,
        "View assignment must have compatible layout" );

      static_assert(
        std::is_same< typename DstTraits::array_scalar_type
                    , typename SrcTraits::array_scalar_type >::value ||
        std::is_same< typename DstTraits::array_scalar_type
                    , typename SrcTraits::const_array_scalar_type >::value ,
        "View assignment must have same value type or const = non-const" );

      static_assert(
        ViewDimensionAssignable
          < typename DstType::offset_type::dimension_type
          , typename SrcFadType::offset_type::dimension_type >::value ,
        "View assignment must have compatible dimensions" );

      typedef typename DstType::offset_type  dst_offset_type ;

      dst.m_offset  = dst_offset_type( src.m_offset );
      dst.m_handle  = src.m_handle ;

      ViewMapping::template assign_fad_size< typename DstTraits::specialize >( dst , src.m_fad_size );
    }
};

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

//----------------------------------------------------------------------------

namespace Kokkos {
namespace Experimental {
namespace Impl {

// Subview mapping

template< class SrcTraits , class ... Args >
struct ViewMapping
  < typename std::enable_if<(
      // Source view has FAD only
      std::is_same< typename SrcTraits::specialize
                  , ViewSpecializeSacadoFad >::value
      &&
      (
        std::is_same< typename SrcTraits::array_layout
                    , Kokkos::LayoutLeft >::value ||
        std::is_same< typename SrcTraits::array_layout
                    , Kokkos::LayoutRight >::value ||
        std::is_same< typename SrcTraits::array_layout
                    , Kokkos::LayoutStride >::value
      )
    )>::type
  , SrcTraits
  , Args ... >
{
private:

  static_assert( SrcTraits::rank == sizeof...(Args) , "" );

  enum
    { RZ = false
    , R0 = bool(is_integral_extent<0,Args...>::value)
    , R1 = bool(is_integral_extent<1,Args...>::value)
    , R2 = bool(is_integral_extent<2,Args...>::value)
    , R3 = bool(is_integral_extent<3,Args...>::value)
    , R4 = bool(is_integral_extent<4,Args...>::value)
    , R5 = bool(is_integral_extent<5,Args...>::value)
    , R6 = bool(is_integral_extent<6,Args...>::value)
    };

  // Public rank
  enum { rank = unsigned(R0) + unsigned(R1) + unsigned(R2) + unsigned(R3)
              + unsigned(R4) + unsigned(R5) + unsigned(R6) };

  // Whether right-most non-FAD rank is a range.
  enum { R0_rev = ( 0 == SrcTraits::rank ? RZ : (
                    1 == SrcTraits::rank ? R0 : (
                    2 == SrcTraits::rank ? R1 : (
                    3 == SrcTraits::rank ? R2 : (
                    4 == SrcTraits::rank ? R3 : (
                    5 == SrcTraits::rank ? R4 : (
                    6 == SrcTraits::rank ? R5 : R6 ))))))) };

  // Subview's layout
  // If LayoutRight then FAD is contiguous
  typedef typename std::conditional<
    ( /* Same layout IF */
      ( rank == 0 )
      ||
      ( std::is_same< typename SrcTraits::array_layout
                    , Kokkos::LayoutRight >::value
        &&
        ( rank == 1 ) && R0_rev
      )
      ||
      ( std::is_same< typename SrcTraits::array_layout
                    , Kokkos::LayoutLeft >::value
        &&
        ( rank == 1 ) && R0
      )
    ), typename SrcTraits::array_layout , Kokkos::LayoutStride
    >::type  array_layout ;

  typedef typename SrcTraits::value_type  fad_type ;

  typedef typename std::conditional< rank == 0 , fad_type ,
          typename std::conditional< rank == 1 , fad_type * ,
          typename std::conditional< rank == 2 , fad_type ** ,
          typename std::conditional< rank == 3 , fad_type *** ,
          typename std::conditional< rank == 4 , fad_type **** ,
          typename std::conditional< rank == 5 , fad_type ***** ,
          typename std::conditional< rank == 6 , fad_type ****** ,
                                                 fad_type *******
          >::type >::type >::type >::type >::type >::type >::type
    data_type ;

public:

  typedef Kokkos::Experimental::ViewTraits
    < data_type
    , array_layout
    , typename SrcTraits::device_type
    , typename SrcTraits::memory_traits > traits_type ;

  typedef Kokkos::Experimental::View
    < data_type
    , array_layout
    , typename SrcTraits::device_type
    , typename SrcTraits::memory_traits > type ;


  KOKKOS_INLINE_FUNCTION
  static void assign( ViewMapping< traits_type , void > & dst
                    , ViewMapping< SrcTraits , void > const & src
                    , Args ... args )
    {
      typedef ViewMapping< traits_type , void > DstType ;
      typedef typename DstType::offset_type  dst_offset_type ;
      typedef typename DstType::handle_type  dst_handle_type ;

      const SubviewExtents< SrcTraits::rank + 1 , rank + 1 >
        extents( src.m_offset.m_dim , args... , Kokkos::ALL() );

      dst.m_offset = dst_offset_type( src.m_offset , extents );
      dst.m_handle = dst_handle_type( src.m_handle +
                                      src.m_offset( extents.domain_offset(0)
                                                  , extents.domain_offset(1)
                                                  , extents.domain_offset(2)
                                                  , extents.domain_offset(3)
                                                  , extents.domain_offset(4)
                                                  , extents.domain_offset(5)
                                                  , extents.domain_offset(6)
                                                  , extents.domain_offset(7)
                                                  ) );
    }

};

} // namespace Impl
} // namespace Experimental
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if defined(HAVE_SACADO_KOKKOSCORE) && \
    defined(HAVE_SACADO_TEUCHOSKOKKOSCOMM) && \
    defined(HAVE_SACADO_VIEW_SPEC) && \
    ! defined(SACADO_DISABLE_FAD_VIEW_SPEC)

#include "Kokkos_TeuchosCommAdapters.hpp"

namespace Teuchos {

template< typename Ordinal , class SendViewType , class RecvViewType >
void reduceAll
  ( const Comm<Ordinal>& comm,
    typename std::enable_if<
      Kokkos::Experimental::Impl::
        is_ViewSpecializeSacadoFad< SendViewType , RecvViewType >::value ,
      const EReductionType >::type reductType ,
    const Ordinal count,
    const SendViewType & sendBuffer ,
    const RecvViewType & recvBuffer )
{
  // We can't implement reduceAll by extracting the underlying array (since we
  // can't reduce across the derivative dimension) and we can't just extract
  // a pointer due to ViewFad.  In principle we could handle ViewFad in the
  // serializer, but for the time being we just copy the view's into local
  // buffers (on the host).
  typedef typename SendViewType::value_type send_value_type;
  typedef typename RecvViewType::value_type recv_value_type;

  TEUCHOS_TEST_FOR_EXCEPTION(
    SendViewType::rank > 1 || RecvViewType::rank > 1, std::invalid_argument,
    "Teuchos::reduceAll: Both send and receive Views must have rank 1.  "
    "The send View's rank is " << SendViewType::rank << " and the receive "
    "View's rank is " << RecvViewType::rank << ".");

  // Copy send buffer into local array
  Teuchos::Array<send_value_type> localSendBuffer(count);
  typename SendViewType::HostMirror hostSendBuffer =
    Kokkos::create_mirror_view(sendBuffer);
  Kokkos::deep_copy(hostSendBuffer, sendBuffer);
  for (Ordinal i=0; i<count; ++i)
    localSendBuffer[i] = hostSendBuffer(i);

  // Copy receive buffer into local array (necessary to initialize Fad types
  // properly)
  Teuchos::Array<recv_value_type> localRecvBuffer(count);
  typename RecvViewType::HostMirror hostRecvBuffer =
    Kokkos::create_mirror_view(recvBuffer);
  Kokkos::deep_copy(hostRecvBuffer, recvBuffer);
  for (Ordinal i=0; i<count; ++i)
    localRecvBuffer[i] = hostRecvBuffer(i);

  // Do reduce-all
  reduceAll(comm, reductType, count,
            localSendBuffer.getRawPtr(),
            localRecvBuffer.getRawPtr());

  // Copy back into original buffer
  for (Ordinal i=0; i<count; ++i)
    hostRecvBuffer(i) = localRecvBuffer[i];
  Kokkos::deep_copy(recvBuffer, hostRecvBuffer);
}


template< typename Ordinal , typename Serializer
        , class SendViewType , class RecvViewType >
void
reduceAll
  ( const Comm<Ordinal>& comm,
    const Serializer& serializer,
    typename std::enable_if<(
      Kokkos::Experimental::Impl::is_ViewSpecializeSacadoFad< SendViewType >::value
      &&
      Kokkos::Experimental::Impl::is_ViewSpecializeSacadoFad< RecvViewType >::value
      ), const EReductionType >::type reductType ,
    const Ordinal count,
    const SendViewType & sendBuffer ,
    const RecvViewType & recvBuffer )
{
  // We can't implement reduceAll by extracting the underlying array (since we
  // can't reduce across the derivative dimension) and we can't just extract
  // a pointer due to ViewFad.  In principle we could handle ViewFad in the
  // serializer, but for the time being we just copy the view's into local
  // buffers (on the host).
  typedef typename SendViewType::value_type send_value_type;
  typedef typename RecvViewType::value_type recv_value_type;

  TEUCHOS_TEST_FOR_EXCEPTION(
    SendViewType::rank > 1 || RecvViewType::rank > 1, std::invalid_argument,
    "Teuchos::reduceAll: Both send and receive Views must have rank 1.  "
    "The send View's rank is " << SendViewType::rank << " and the receive "    "View's rank is " << RecvViewType::rank << ".");

  // Copy send buffer into local array
  Teuchos::Array<send_value_type> localSendBuffer(count);
  typename SendViewType::HostMirror hostSendBuffer =
    Kokkos::create_mirror_view(sendBuffer);
  Kokkos::deep_copy(hostSendBuffer, sendBuffer);
  for (Ordinal i=0; i<count; ++i)
    localSendBuffer[i] = hostSendBuffer(i);

  // Copy receive buffer into local array (necessary to initialize Fad types
  // properly)
  Teuchos::Array<recv_value_type> localRecvBuffer(count);
  typename RecvViewType::HostMirror hostRecvBuffer =
    Kokkos::create_mirror_view(recvBuffer);
  Kokkos::deep_copy(hostRecvBuffer, recvBuffer);
  for (Ordinal i=0; i<count; ++i)
    localRecvBuffer[i] = hostRecvBuffer(i);

  // Do reduce-all
  reduceAll(comm, serializer, reductType, count,
            localSendBuffer.getRawPtr(),
            localRecvBuffer.getRawPtr());

  // Copy back into original buffer
  for (Ordinal i=0; i<count; ++i)
    hostRecvBuffer(i) = localRecvBuffer[i];
  Kokkos::deep_copy(recvBuffer, hostRecvBuffer);
}


template<typename Ordinal, class view_type >
void broadcast
  ( const Comm<Ordinal>& comm,
    typename std::enable_if<(
      Kokkos::Experimental::Impl::is_ViewSpecializeSacadoFad< view_type >::value
      ), const int >::type rootRank ,
    const Ordinal count,
    const view_type & buffer)
{
  typename view_type::array_type array_buffer = buffer;
  Ordinal array_count = count * buffer.storage_size();
  broadcast( comm, rootRank, array_count, array_buffer );
}

template<typename Ordinal,
         typename Serializer ,
         class view_type >
void broadcast
  ( const Comm<Ordinal>& comm,
    const Serializer& serializer,
    typename std::enable_if<(
      Kokkos::Experimental::Impl::is_ViewSpecializeSacadoFad< view_type >::value
      ), const int >::type rootRank ,
    const Ordinal count,
    const view_type & buffer)
{
  typename view_type::array_type array_buffer = buffer;
  Ordinal array_count = count * buffer.storage_size();
  broadcast( comm, *(serializer.getValueSerializer()), rootRank,
             array_count, array_buffer );
}

} // namespace Teuchos

#endif

//----------------------------------------------------------------------------

#endif

#endif /* #ifndef KOKKOS_EXPERIMENTAL_VIEW_SACADO_FAD_HPP */

