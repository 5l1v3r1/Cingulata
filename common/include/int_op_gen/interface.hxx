/*
    (C) Copyright 2019 CEA LIST. All Rights Reserved.
    Contributor(s): Cingulata team

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#ifndef INT_OP_GEN_INTERFACE
#define INT_OP_GEN_INTERFACE

#include <ci_bit.hxx>
#include <ci_bit_vector.hxx>
#include <ci_int.hxx>
#include <int_op_gen/impl/all.hxx>

#include <vector>

namespace cingulata
{

class IIntOpGen {
public:
  IIntOpGen();

  /**
   * @brief      Add integers represented as vectors of bits. The output
   *             vector size is equal to the @c lhs input vector size
   *
   * @param[in]  lhs   The left hand side bit vector
   * @param[in]  rhs   The right hand side bit vector
   *
   * @return     The sum of input bit vectors as a bit vector
   */
  virtual CiBitVector add     ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const = 0;

  virtual CiBitVector sub     ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const;

  virtual CiBitVector neg     ( const CiBitVector& lhs) const = 0;

  virtual CiBitVector mul     ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const = 0;

  virtual CiBitVector square  ( const CiBitVector& lhs) const;


  virtual CiBit equal         ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const = 0;

  virtual CiBit not_equal     ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const;

  virtual CiBit lower         ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const = 0;

  virtual CiBit lower_equal   ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const;

  virtual CiBit greater_equal ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const;

  virtual CiBit greater       ( const CiBitVector& lhs,
                                const CiBitVector& rhs) const;

  virtual CiBitVector mux     ( const CiBit &cond,
                                const CiBitVector &a,
                                const CiBitVector &b) const;

  virtual CiBitVector mux     ( const CiBitVector &cond,
                                const std::vector<CiBitVector> &inps) const;

  virtual std::vector<CiBitVector>
                      sort    ( const std::vector<CiBitVector> &v_cbv,
                                const std::vector<CiBitVector> &i_cbv,
                                const bool reverse) const;

  virtual CiBitVector sum     ( const std::vector<CiBitVector> &inps) const;

private:
  int_ops::MuxDepth           m_mux;
  int_ops::MultiInputAdder    m_multi_input_adder;
  int_ops::SortDepth          m_sort;
};

} // namespace cingulata

#endif
