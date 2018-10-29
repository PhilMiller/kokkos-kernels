#ifndef __KOKKOSBATCHED_HESSENBERG_QR_WITH_SHIFT_SERIAL_INTERNAL_HPP__
#define __KOKKOSBATCHED_HESSENBERG_QR_WITH_SHIFT_SERIAL_INTERNAL_HPP__


/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "KokkosBatched_Util.hpp"
#include "KokkosBatched_Givens_Serial_Internal.hpp"
#include "KokkosBatched_ApplyGivens_Serial_Internal.hpp"

namespace KokkosBatched {
  namespace Experimental {
    ///
    /// Serial Internal Impl
    /// ====================
    ///
    /// this impl follows the flame interface of householder transformation
    ///
    struct SerialHessenbergQR_WithShiftInternal {
      template<typename ValueType>
      KOKKOS_INLINE_FUNCTION
      static int
      invoke(const int m,
             /* */ ValueType * H, const int hs0, const int hs1,
             const ValueType shift) {
        typedef ValueType value_type;
        const value_type zero(0);

        /// Given a strict Hessenberg matrix H (m x m),
        /// it computes a single implicit QR step with a given shift
        /// - it assumes H has zeros on subdiagonal entries (
        /// givens rotation is defined as G = [gamma -sigma
        ///                                    sigma  gamma]
        ///   G' [chi1 chi2]^t = [alpha 0]^T
        /// where G is stored as a pair of gamma and sigma
        Kokkos::pair<value_type,value_type> G;

        /// 0. compute the first givens rotation that introduces a bulge
        {
          const value_type chi1 = H[0] - shift;
          const value_type chi2 = H[hs0];
          /* */ value_type alpha;
          SerialGivensInternal::invoke(chi1, chi2,
                                       &G,
                                       &alpha);

          value_type *h11 = H;
          value_type *h21 = H + hs0;
          value_type *h12 = H + hs1;

          // apply G' from left
          G.second = -G.second; // transpose G
          const int nn = m;
          SerialApplyLeftGivensInternal::invoke (G, nn,
                                                 h11, hs1,
                                                 h21, hs1);

          // apply (G')' from right
          const int mm = m < 3 ? m : 3;
          SerialApplyRightGivensInternal::invoke(G, mm,
                                                 h11, hs0,
                                                 h12, hs0);
        }

        /// 1. chase the bulge

        // partitions used for loop iteration
        Partition2x2<value_type> H_part2x2(hs0, hs1);
        Partition3x3<value_type> H_part3x3(hs0, hs1);

        // initial partition of A where ATL has a zero dimension
        H_part2x2.partWithATL(H, m, m, 1, 1);

        for (int m_htl=1;m_htl<(m-1);++m_htl) {
          // part 2x2 into 3x3
          H_part3x3.partWithABR(H_part2x2, 1, 1);
          const int n_hbr = m - m_htl;
          /// -----------------------------------------------------
          const value_type chi1 = *H_part3x3.A10;
          const value_type chi2 = *H_part3x3.A20;
          SerialGivensInternal::invoke(chi1, chi2,
                                       &G,
                                       H_part3x3.A10);
          *H_part3x3.A20 = zero; // explicitly set zero
          G.second = -G.second; // transpose G

          const int nn = m - m_htl;
          SerialApplyLeftGivensInternal::invoke (G, nn,
                                                 H_part3x3.A11, hs1,
                                                 H_part3x3.A21, hs1);
          const int mtmp = m_htl+3, mm = mtmp < m ? mtmp : m;
          SerialApplyRightGivensInternal::invoke(G, mm,
                                                 H_part3x3.A01, hs0,
                                                 H_part3x3.A02, hs0);
          /// -----------------------------------------------------
          H_part2x2.mergeToATL(H_part3x3);
        }
        return 0;
      }
    };

  }//  end namespace Experimental
} // end namespace KokkosBatched


#endif
