#include "Tpetra_CrsMatrix.hpp"

#ifdef HAVE_TPETRA_EXPLICIT_INSTANTIATION

// #include "Tpetra_ExplicitInstantiationHelpers.hpp"

#include "Tpetra_CrsMatrix_def.hpp"

namespace Tpetra {

#if defined(HAVE_TPETRA_INST_FLOAT)
  TPETRA_CRSMATRIX_INSTANT(float,int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_CRSMATRIX_INSTANT(float,int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_CRSMATRIX_INSTANT(float,int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST) && defined(HAVE_KOKKOS_CUDA_FLOAT)
    TPETRA_CRSMATRIX_INSTANT(float,int,int,Kokkos::ThrustGPUNode)
#endif
#endif

#if defined(HAVE_TPETRA_INST_DOUBLE)
  TPETRA_CRSMATRIX_INSTANT(double,int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_CRSMATRIX_INSTANT(double,int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_CRSMATRIX_INSTANT(double,int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST) && defined(HAVE_KOKKOS_CUDA_DOUBLE)
    TPETRA_CRSMATRIX_INSTANT(double,int,int,Kokkos::ThrustGPUNode)
#endif
#endif


} // namespace Tpetra

#endif // HAVE_TPETRA_EXPLICIT_INSTANTIATION
