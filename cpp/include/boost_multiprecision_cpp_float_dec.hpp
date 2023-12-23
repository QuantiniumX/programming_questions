///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2002 - 2021.
// Copyright 2011 -2021 John Maddock. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at <a href="http://www.boost.org/LICENSE_1_0.txt">http://www.boost.org/LICENSE_1_0.txt</a>)
//
// This work is based on an earlier work:
// &quot;Algorithm 910: A Portable C++ Multiple-Precision System for Special-Function Calculations&quot;,
// in ACM TOMS, {VOL 37, ISSUE 4, (February 2011)} (C) ACM, 2011. <a href="http://doi.acm.org/10.1145/1916461.1916469">http://doi.acm.org/10.1145/1916461.1916469</a>
//
// There are some &quot;noexcept&quot; specifications on the functions in this file.
// Unlike in pre-C++11 versions, compilers can now detect noexcept misuse
// at compile time, allowing for simple use of it here.
//

#ifndef BOOST_MP_CPP_DEC_FLOAT_BACKEND_HPP
#define BOOST_MP_CPP_DEC_FLOAT_BACKEND_HPP

#include &lt;algorithm&gt;
#include &lt;array&gt;
#include &lt;cstdint&gt;
#include &lt;initializer_list&gt;
#include &lt;limits&gt;
#include &lt;<a href="../../boost/multiprecision/detail/standalone_config.hpp">boost/multiprecision/detail/standalone_config.hpp</a>&gt;
#include &lt;<a href="../../boost/multiprecision/number.hpp">boost/multiprecision/number.hpp</a>&gt;

#include &lt;<a href="../../boost/multiprecision/detail/dynamic_array.hpp">boost/multiprecision/detail/dynamic_array.hpp</a>&gt;
#include &lt;<a href="../../boost/multiprecision/detail/hash.hpp">boost/multiprecision/detail/hash.hpp</a>&gt;
#include &lt;<a href="../../boost/multiprecision/detail/itos.hpp">boost/multiprecision/detail/itos.hpp</a>&gt;
#include &lt;<a href="../../boost/multiprecision/detail/static_array.hpp">boost/multiprecision/detail/static_array.hpp</a>&gt;
#include &lt;<a href="../../boost/multiprecision/detail/tables.hpp">boost/multiprecision/detail/tables.hpp</a>&gt;

//
// Headers required for Boost.Math integration:
//
#include &lt;<a href="../../boost/math/policies/policy.hpp">boost/math/policies/policy.hpp</a>&gt;
//
// Some includes we need from Boost.Math, since we rely on that library to provide these functions:
//
#include &lt;<a href="../../boost/math/special_functions/acosh.hpp">boost/math/special_functions/acosh.hpp</a>&gt;
#include &lt;<a href="../../boost/math/special_functions/asinh.hpp">boost/math/special_functions/asinh.hpp</a>&gt;
#include &lt;<a href="../../boost/math/special_functions/atanh.hpp">boost/math/special_functions/atanh.hpp</a>&gt;
#include &lt;<a href="../../boost/math/special_functions/cbrt.hpp">boost/math/special_functions/cbrt.hpp</a>&gt;
#include &lt;<a href="../../boost/math/special_functions/expm1.hpp">boost/math/special_functions/expm1.hpp</a>&gt;
#include &lt;<a href="../../boost/math/special_functions/gamma.hpp">boost/math/special_functions/gamma.hpp</a>&gt;

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable : 6326) // comparison of two constants
#endif

namespace boost {
namespace multiprecision {
namespace backends {

template &lt;unsigned Digits10, class ExponentType = std::int32_t, class Allocator = void&gt;
class cpp_dec_float;

} // namespace backends

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
struct number_category&lt;backends::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; &gt; : public std::integral_constant&lt;int, number_kind_floating_point&gt;
{};

namespace backends {

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
class cpp_dec_float
{
 private:
   // Perform some static sanity checks.
   static_assert(boost::multiprecision::detail::is_signed&lt;ExponentType&gt;::value,
                 &quot;ExponentType must be a signed built in integer type.&quot;);

   static_assert(sizeof(ExponentType) &gt; 1,
                 &quot;ExponentType is too small.&quot;);

   static_assert(Digits10 &lt; UINT32_C(0x80000000),
                 &quot;Digits10 exceeds the maximum.&quot;);

   // Private class-local constants.
   static constexpr std::int32_t  cpp_dec_float_digits10_limit_lo = INT32_C(9);
   static constexpr std::int32_t  cpp_dec_float_digits10_limit_hi = static_cast&lt;std::int32_t&gt;((std::numeric_limits&lt;std::int32_t&gt;::max)() - 100);

   static constexpr std::int32_t cpp_dec_float_elem_digits10      = INT32_C(8);
   static constexpr std::int32_t cpp_dec_float_elem_mask          = INT32_C(100000000);

   static constexpr std::int32_t cpp_dec_float_elems_for_kara     = static_cast&lt;std::int32_t&gt;(128 + 1);

 public:
   using signed_types   = std::tuple&lt;boost::long_long_type&gt; ;
   using unsigned_types = std::tuple&lt;boost::ulong_long_type&gt;;
   using float_types    = std::tuple&lt;double, long double&gt;;
   using exponent_type  = ExponentType;

   // Public class-local constants.
   static constexpr std::int32_t  cpp_dec_float_radix             = INT32_C(10);
   static constexpr std::int32_t  cpp_dec_float_digits10          = ((static_cast&lt;std::int32_t&gt;(Digits10) &lt; cpp_dec_float_digits10_limit_lo) ? cpp_dec_float_digits10_limit_lo : ((static_cast&lt;std::int32_t&gt;(Digits10) &gt; cpp_dec_float_digits10_limit_hi) ? cpp_dec_float_digits10_limit_hi : static_cast&lt;std::int32_t&gt;(Digits10)));
   static constexpr exponent_type cpp_dec_float_max_exp10         = (static_cast&lt;exponent_type&gt;(1) &lt;&lt; (std::numeric_limits&lt;exponent_type&gt;::digits - 5));
   static constexpr exponent_type cpp_dec_float_min_exp10         = -cpp_dec_float_max_exp10;
   static constexpr exponent_type cpp_dec_float_max_exp           = cpp_dec_float_max_exp10;
   static constexpr exponent_type cpp_dec_float_min_exp           = cpp_dec_float_min_exp10;

   static_assert(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_exp10 == -cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp10, &quot;Failed exponent range check&quot;);

   static_assert(0 == cpp_dec_float_max_exp10 % cpp_dec_float_elem_digits10, &quot;Failed digit sanity check&quot;);

 private:
   // There are three guard limbs.
   // 1) The first limb has 'play' from 1...8 decimal digits.
   // 2) The last limb also has 'play' from 1...8 decimal digits.
   // 3) One limb can get lost when justifying after multiply.
   static constexpr std::int32_t cpp_dec_float_elem_number    = static_cast&lt;std::int32_t&gt;(((Digits10 / cpp_dec_float_elem_digits10) + (((Digits10 % cpp_dec_float_elem_digits10) != 0) ? 1 : 0)) + 3);

 public:
   static constexpr std::int32_t cpp_dec_float_max_digits10   = static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_number * cpp_dec_float_elem_digits10);

 private:
   using array_type =
      typename std::conditional&lt;std::is_void&lt;Allocator&gt;::value,
                                detail::static_array &lt;std::uint32_t, cpp_dec_float_elem_number&gt;,
                                detail::dynamic_array&lt;std::uint32_t, cpp_dec_float_elem_number, Allocator&gt; &gt;::type;

   typedef enum enum_fpclass_type
   {
      cpp_dec_float_finite,
      cpp_dec_float_inf,
      cpp_dec_float_NaN
   } fpclass_type;

   array_type    data;
   exponent_type exp;
   bool          neg;
   fpclass_type  fpclass;
   std::int32_t  prec_elem;

   // Private constructor from the floating-point class type.
   explicit cpp_dec_float(fpclass_type c) : data(),
                                            exp(static_cast&lt;exponent_type&gt;(0)),
                                            neg(false),
                                            fpclass(c),
                                            prec_elem(cpp_dec_float_elem_number) {}

   // Constructor from an initializer_list, an optional
   // (value-aligned) exponent and a Boolean sign.
   static cpp_dec_float from_lst(std::initializer_list&lt;std::uint32_t&gt; lst,
                                 const exponent_type e = 0,
                                 const bool n = false)
   {
      cpp_dec_float a;

      a.data      = array_type(lst);
      a.exp       = e;
      a.neg       = n;
      a.fpclass   = cpp_dec_float_finite;
      a.prec_elem = cpp_dec_float_elem_number;

      return a;
   }

 public:
   // Public Constructors
   cpp_dec_float() noexcept(noexcept(array_type())) : data(),
                                                      exp(static_cast&lt;exponent_type&gt;(0)),
                                                      neg(false),
                                                      fpclass(cpp_dec_float_finite),
                                                      prec_elem(cpp_dec_float_elem_number) {}

   cpp_dec_float(const char* s) : data(),
                                  exp(static_cast&lt;exponent_type&gt;(0)),
                                  neg(false),
                                  fpclass(cpp_dec_float_finite),
                                  prec_elem(cpp_dec_float_elem_number)
   {
      *this = s;
   }

   template &lt;class I&gt;
   cpp_dec_float(I i,
                 typename std::enable_if&lt;boost::multiprecision::detail::is_unsigned&lt;I&gt;::value &gt;::type* = nullptr)
      : data(),
        exp(static_cast&lt;exponent_type&gt;(0)),
        neg(false),
        fpclass(cpp_dec_float_finite),
        prec_elem(cpp_dec_float_elem_number)
   {
      from_unsigned_long_long(i);
   }

   template &lt;class I&gt;
   cpp_dec_float(I i,
                 typename std::enable_if&lt;(   boost::multiprecision::detail::is_signed&lt;I&gt;::value
                                          &amp;&amp; boost::multiprecision::detail::is_integral&lt;I&gt;::value)&gt;::type* = nullptr)
      : data(),
        exp(static_cast&lt;exponent_type&gt;(0)),
        neg(false),
        fpclass(cpp_dec_float_finite),
        prec_elem(cpp_dec_float_elem_number)
   {
      if (i &lt; 0)
      {
         from_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(i));
         negate();
      }
      else
         from_unsigned_long_long(i);
   }

   cpp_dec_float(const cpp_dec_float&amp; f) noexcept(noexcept(array_type(std::declval&lt;const array_type&amp;&gt;())))
      : data(f.data),
        exp(f.exp),
        neg(f.neg),
        fpclass(f.fpclass),
        prec_elem(f.prec_elem) {}

   template &lt;unsigned D, class ET, class A&gt;
   cpp_dec_float(const cpp_dec_float&lt;D, ET, A&gt;&amp; f, typename std::enable_if&lt;D &lt;= Digits10&gt;::type* = nullptr)
      : data(),
        exp(f.exp),
        neg(f.neg),
        fpclass(static_cast&lt;fpclass_type&gt;(static_cast&lt;int&gt;(f.fpclass))),
        prec_elem(cpp_dec_float_elem_number)
   {
      std::copy(f.data.begin(), f.data.begin() + f.prec_elem, data.begin());
   }
   template &lt;unsigned D, class ET, class A&gt;
   explicit cpp_dec_float(const cpp_dec_float&lt;D, ET, A&gt;&amp; f, typename std::enable_if&lt; !(D &lt;= Digits10)&gt;::type* = nullptr)
      : data(),
        exp(f.exp),
        neg(f.neg),
        fpclass(static_cast&lt;fpclass_type&gt;(static_cast&lt;int&gt;(f.fpclass))),
        prec_elem(cpp_dec_float_elem_number)
   {
      // TODO: this doesn't round!
      std::copy(f.data.begin(), f.data.begin() + prec_elem, data.begin());
   }

   template &lt;class F&gt;
   cpp_dec_float(const F val, typename std::enable_if&lt;std::is_floating_point&lt;F&gt;::value
#ifdef BOOST_HAS_FLOAT128
                                                   &amp;&amp; !std::is_same&lt;F, __float128&gt;::value
#endif
                                                   &gt;::type* = 0) : data(),
                                                                   exp(static_cast&lt;exponent_type&gt;(0)),
                                                                   neg(false),
                                                                   fpclass(cpp_dec_float_finite),
                                                                   prec_elem(cpp_dec_float_elem_number)
   {
      *this = val;
   }

   cpp_dec_float(const double mantissa, const exponent_type exponent);

   std::size_t hash() const
   {
      std::size_t result = 0;
      for (int i = 0; i &lt; prec_elem; ++i)
         boost::multiprecision::detail::hash_combine(result, data[i]);
      boost::multiprecision::detail::hash_combine(result, exp, neg, static_cast&lt;std::size_t&gt;(fpclass));
      return result;
   }

   // Specific special values.
   static const cpp_dec_float&amp;  nan () { static const cpp_dec_float val(cpp_dec_float_NaN); return val; }
   static const cpp_dec_float&amp;  inf () { static const cpp_dec_float val(cpp_dec_float_inf); return val; }
   static const cpp_dec_float&amp; (max)() { static const cpp_dec_float val(from_lst({ std::uint32_t(1u) }, cpp_dec_float_max_exp10)); return val; }
   static const cpp_dec_float&amp; (min)() { static const cpp_dec_float val(from_lst({ std::uint32_t(1u) }, cpp_dec_float_min_exp10)); return val; }
   static const cpp_dec_float&amp;  zero() { static const cpp_dec_float val(from_lst({ std::uint32_t(0u) })); return val; }
   static const cpp_dec_float&amp;  one () { static const cpp_dec_float val(from_lst({ std::uint32_t(1u) })); return val; }
   static const cpp_dec_float&amp;  two () { static const cpp_dec_float val(from_lst({ std::uint32_t(2u) })); return val; }
   static const cpp_dec_float&amp;  half() { static const cpp_dec_float val(from_lst({ std::uint32_t(cpp_dec_float_elem_mask / 2)}, -8)); return val; }

   static const cpp_dec_float&amp; double_min() { static const cpp_dec_float val((std::numeric_limits&lt;double&gt;::min)()); return val; }
   static const cpp_dec_float&amp; double_max() { static const cpp_dec_float val((std::numeric_limits&lt;double&gt;::max)()); return val; }

   static const cpp_dec_float&amp; long_double_min()
   {
#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
      static const cpp_dec_float val(static_cast&lt;long double&gt;((std::numeric_limits&lt;double&gt;::min)()));
#else
      static const cpp_dec_float val((std::numeric_limits&lt;long double&gt;::min)());
#endif
      return val;
   }

   static const cpp_dec_float&amp; long_double_max()
   {
#ifdef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
      static const cpp_dec_float val(static_cast&lt;long double&gt;((std::numeric_limits&lt;double&gt;::max)()));
#else
      static const cpp_dec_float val((std::numeric_limits&lt;long double&gt;::max)());
#endif
      return val;
   }

   static const cpp_dec_float&amp; long_long_max () { static const cpp_dec_float val((std::numeric_limits&lt;boost::long_long_type&gt;::max)()); return val; }
   static const cpp_dec_float&amp; long_long_min () { static const cpp_dec_float val((std::numeric_limits&lt;boost::long_long_type&gt;::min)()); return val; }
   static const cpp_dec_float&amp; ulong_long_max() { static const cpp_dec_float val((std::numeric_limits&lt;boost::ulong_long_type&gt;::max)()); return val; }

   static const cpp_dec_float&amp; eps()
   {
      static const cpp_dec_float val
      (
        from_lst
        (
          {
            (std::uint32_t) detail::pow10_maker((std::uint32_t) ((std::int32_t) (INT32_C(1) + (std::int32_t) (((cpp_dec_float_digits10 / cpp_dec_float_elem_digits10) + ((cpp_dec_float_digits10 % cpp_dec_float_elem_digits10) != 0 ? 1 : 0)) * cpp_dec_float_elem_digits10)) - cpp_dec_float_digits10))
          },
          -(exponent_type) (((cpp_dec_float_digits10 / cpp_dec_float_elem_digits10) + ((cpp_dec_float_digits10 % cpp_dec_float_elem_digits10) != 0 ? 1 : 0)) * cpp_dec_float_elem_digits10)
        )
      );

      return val;
   }

   // Basic operations.
   cpp_dec_float&amp; operator=(const cpp_dec_float&amp; v) noexcept(noexcept(std::declval&lt;array_type&amp;&gt;() = std::declval&lt;const array_type&amp;&gt;()))
   {
      data      = v.data;
      exp       = v.exp;
      neg       = v.neg;
      fpclass   = v.fpclass;
      prec_elem = v.prec_elem;
      return *this;
   }

   template &lt;unsigned D&gt;
   cpp_dec_float&amp; operator=(const cpp_dec_float&lt;D&gt;&amp; f)
   {
      exp            = f.exp;
      neg            = f.neg;
      fpclass        = static_cast&lt;enum_fpclass_type&gt;(static_cast&lt;int&gt;(f.fpclass));
      unsigned elems = (std::min)(f.prec_elem, cpp_dec_float_elem_number);
      std::copy(f.data.begin(), f.data.begin() + elems, data.begin());
      std::fill(data.begin() + elems, data.end(), 0);
      prec_elem = cpp_dec_float_elem_number;
      return *this;
   }

   cpp_dec_float&amp; operator=(boost::long_long_type v)
   {
      if (v &lt; 0)
      {
         from_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(v));
         negate();
      }
      else
         from_unsigned_long_long(v);
      return *this;
   }

   cpp_dec_float&amp; operator=(boost::ulong_long_type v)
   {
      from_unsigned_long_long(v);
      return *this;
   }

   template &lt;class Float&gt;
   typename std::enable_if&lt;std::is_floating_point&lt;Float&gt;::value, cpp_dec_float&amp;&gt;::type operator=(Float v);

   cpp_dec_float&amp; operator=(const char* v)
   {
      rd_string(v);
      return *this;
   }

   cpp_dec_float&amp; operator+=(const cpp_dec_float&amp; v);
   cpp_dec_float&amp; operator-=(const cpp_dec_float&amp; v);
   cpp_dec_float&amp; operator*=(const cpp_dec_float&amp; v);
   cpp_dec_float&amp; operator/=(const cpp_dec_float&amp; v);

   cpp_dec_float&amp; add_unsigned_long_long(const boost::ulong_long_type n)
   {
      cpp_dec_float t;
      t.from_unsigned_long_long(n);
      return *this += t;
   }

   cpp_dec_float&amp; sub_unsigned_long_long(const boost::ulong_long_type n)
   {
      cpp_dec_float t;
      t.from_unsigned_long_long(n);
      return *this -= t;
   }

   cpp_dec_float&amp; mul_unsigned_long_long(const boost::ulong_long_type n);
   cpp_dec_float&amp; div_unsigned_long_long(const boost::ulong_long_type n);

   // Elementary primitives.
   cpp_dec_float&amp; calculate_inv();
   cpp_dec_float&amp; calculate_sqrt();

   void negate()
   {
      if (!iszero())
         neg = !neg;
   }

   // Comparison functions
   bool isnan BOOST_PREVENT_MACRO_SUBSTITUTION() const { return (fpclass == cpp_dec_float_NaN); }
   bool isinf BOOST_PREVENT_MACRO_SUBSTITUTION() const { return (fpclass == cpp_dec_float_inf); }
   bool isfinite BOOST_PREVENT_MACRO_SUBSTITUTION() const { return (fpclass == cpp_dec_float_finite); }

   bool iszero() const
   {
      return ((fpclass == cpp_dec_float_finite) &amp;&amp; (data[0u] == 0u));
   }

   bool isone() const;
   bool isint() const;
   bool isneg() const { return neg; }

   // Operators pre-increment and pre-decrement
   cpp_dec_float&amp; operator++()
   {
      return *this += one();
   }

   cpp_dec_float&amp; operator--()
   {
      return *this -= one();
   }

   std::string str(std::intmax_t digits, std::ios_base::fmtflags f) const;

   int compare(const cpp_dec_float&amp; v) const;

   template &lt;class V&gt;
   int compare(const V&amp; v) const
   {
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; t;
      t = v;
      return compare(t);
   }

   void swap(cpp_dec_float&amp; v)
   {
      data.swap(v.data);
      std::swap(exp, v.exp);
      std::swap(neg, v.neg);
      std::swap(fpclass, v.fpclass);
      std::swap(prec_elem, v.prec_elem);
   }

   double                 extract_double() const;
   long double            extract_long_double() const;
   boost::long_long_type  extract_signed_long_long() const;
   boost::ulong_long_type extract_unsigned_long_long() const;
   void                   extract_parts(double&amp; mantissa, exponent_type&amp; exponent) const;
   cpp_dec_float          extract_integer_part() const;

   void precision(const std::int32_t prec_digits)
   {
      const std::int32_t elems =
        static_cast&lt;std::int32_t&gt;(    static_cast&lt;std::int32_t&gt;(prec_digits / cpp_dec_float_elem_digits10)
                                  +                          (((prec_digits % cpp_dec_float_elem_digits10) != 0) ? 1 : 0));

      prec_elem = (std::min)(cpp_dec_float_elem_number, (std::max)(elems, static_cast&lt;std::int32_t&gt;(2)));
   }
   static cpp_dec_float pow2(boost::long_long_type i);
   exponent_type order() const
   {
      const bool bo_order_is_zero = ((!(isfinite)()) || (data[0] == static_cast&lt;std::uint32_t&gt;(0u)));
      //
      // Binary search to find the order of the leading term:
      //
      exponent_type prefix = 0;

      if (data[0] &gt;= 100000UL)
      {
         if (data[0] &gt;= 10000000UL)
         {
            if (data[0] &gt;= 100000000UL)
            {
               if (data[0] &gt;= 1000000000UL)
                  prefix = 9;
               else
                  prefix = 8;
            }
            else
               prefix = 7;
         }
         else
         {
            if (data[0] &gt;= 1000000UL)
               prefix = 6;
            else
               prefix = 5;
         }
      }
      else
      {
         if (data[0] &gt;= 1000UL)
         {
            if (data[0] &gt;= 10000UL)
               prefix = 4;
            else
               prefix = 3;
         }
         else
         {
            if (data[0] &gt;= 100)
               prefix = 2;
            else if (data[0] &gt;= 10)
               prefix = 1;
         }
      }

      return (bo_order_is_zero ? static_cast&lt;exponent_type&gt;(0) : static_cast&lt;exponent_type&gt;(exp + prefix));
   }

   template &lt;class Archive&gt;
   void serialize(Archive&amp; ar, const unsigned int /*version*/)
   {
      for (unsigned i = 0; i &lt; data.size(); ++i)
         ar&amp; boost::make_nvp(&quot;digit&quot;, data[i]);
      ar&amp; boost::make_nvp(&quot;exponent&quot;, exp);
      ar&amp; boost::make_nvp(&quot;sign&quot;, neg);
      ar&amp; boost::make_nvp(&quot;class-type&quot;, fpclass);
      ar&amp; boost::make_nvp(&quot;precision&quot;, prec_elem);
   }

 private:
   static bool data_elem_is_non_zero_predicate(const std::uint32_t&amp; d) { return (d != static_cast&lt;std::uint32_t&gt;(0u)); }
   static bool data_elem_is_non_nine_predicate(const std::uint32_t&amp; d) { return (d != static_cast&lt;std::uint32_t&gt;(cpp_dec_float::cpp_dec_float_elem_mask - 1)); }
   static bool char_is_nonzero_predicate(const char&amp; c) { return (c != static_cast&lt;char&gt;('0')); }

   void from_unsigned_long_long(const boost::ulong_long_type u);

   template &lt;typename InputIteratorTypeLeft,
             typename InputIteratorTypeRight&gt;
   static int compare_ranges(InputIteratorTypeLeft  a,
                             InputIteratorTypeRight b,
                             const std::uint32_t    count = cpp_dec_float_elem_number);

   static std::uint32_t eval_add_n(      std::uint32_t* r,
                                   const std::uint32_t* u,
                                   const std::uint32_t* v,
                                   const std::int32_t   count);

   static std::uint32_t eval_subtract_n(      std::uint32_t* r,
                                        const std::uint32_t* u,
                                        const std::uint32_t* v,
                                        const std::int32_t   count);

   static void eval_multiply_n_by_n_to_2n(      std::uint32_t* r,
                                          const std::uint32_t* a,
                                          const std::uint32_t* b,
                                          const std::uint32_t  count);

   static std::uint32_t mul_loop_n(std::uint32_t* const u, std::uint32_t n, const std::int32_t p);
   static std::uint32_t div_loop_n(std::uint32_t* const u, std::uint32_t n, const std::int32_t p);

   static void eval_multiply_kara_propagate_carry (std::uint32_t* t, const std::uint32_t n, const std::uint32_t carry);
   static void eval_multiply_kara_propagate_borrow(std::uint32_t* t, const std::uint32_t n, const bool has_borrow);
   static void eval_multiply_kara_n_by_n_to_2n    (      std::uint32_t* r,
                                                   const std::uint32_t* a,
                                                   const std::uint32_t* b,
                                                   const std::uint32_t  n,
                                                         std::uint32_t* t);

   template&lt;unsigned D&gt;
   void eval_mul_dispatch_multiplication_method(
      const cpp_dec_float&lt;D, ExponentType, Allocator&gt;&amp; v,
      const std::int32_t prec_elems_for_multiply,
      const typename std::enable_if&lt;   (D == Digits10)
                                    &amp;&amp; (cpp_dec_float&lt;D, ExponentType, Allocator&gt;::cpp_dec_float_elem_number &lt; cpp_dec_float_elems_for_kara)&gt;::type* = nullptr)
   {
      // Use school multiplication.

      using array_for_mul_result_type =
         typename std::conditional&lt;std::is_void&lt;Allocator&gt;::value,
                                   detail::static_array &lt;std::uint32_t, cpp_dec_float_elem_number * 2&gt;,
                                   detail::dynamic_array&lt;std::uint32_t, cpp_dec_float_elem_number * 2, Allocator&gt; &gt;::type;

      array_for_mul_result_type result;

      eval_multiply_n_by_n_to_2n(result.data(), data.data(), v.data.data(), prec_elems_for_multiply);

      // Handle a potential carry.
      if(result[0U] != static_cast&lt;std::uint32_t&gt;(0U))
      {
         exp += static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);

         // Shift the result of the multiplication one element to the right.
         std::copy(result.cbegin(),
                   result.cbegin() + static_cast&lt;std::ptrdiff_t&gt;(prec_elems_for_multiply),
                   data.begin());
      }
      else
      {
         std::copy(result.cbegin() + 1,
                   result.cbegin() + (std::min)(static_cast&lt;std::int32_t&gt;(prec_elems_for_multiply + 1), cpp_dec_float_elem_number),
                   data.begin());
      }
   }

   template&lt;unsigned D&gt;
   void eval_mul_dispatch_multiplication_method(
      const cpp_dec_float&lt;D, ExponentType, Allocator&gt;&amp; v,
      const std::int32_t prec_elems_for_multiply,
      const typename std::enable_if&lt;    (D == Digits10)
                                    &amp;&amp; !(cpp_dec_float&lt;D, ExponentType, Allocator&gt;::cpp_dec_float_elem_number &lt; cpp_dec_float_elems_for_kara)&gt;::type* = nullptr)
   {
      if(prec_elems_for_multiply &lt; cpp_dec_float_elems_for_kara)
      {
         // Use school multiplication.

         using array_for_mul_result_type =
            typename std::conditional&lt;std::is_void&lt;Allocator&gt;::value,
                                      detail::static_array &lt;std::uint32_t, cpp_dec_float_elem_number * 2&gt;,
                                      detail::dynamic_array&lt;std::uint32_t, cpp_dec_float_elem_number * 2, Allocator&gt; &gt;::type;

         array_for_mul_result_type result;

         eval_multiply_n_by_n_to_2n(result.data(), data.data(), v.data.data(), prec_elems_for_multiply);

         // Handle a potential carry.
         if(result[0U] != static_cast&lt;std::uint32_t&gt;(0U))
         {
            exp += static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);
         
            // Shift the result of the multiplication one element to the right.
            std::copy(result.cbegin(),
                      result.cbegin() + static_cast&lt;std::ptrdiff_t&gt;(prec_elems_for_multiply),
                      data.begin());
         }
         else
         {
            std::copy(result.cbegin() + 1,
                      result.cbegin() + (std::min)(static_cast&lt;std::int32_t&gt;(prec_elems_for_multiply + 1), cpp_dec_float_elem_number),
                      data.begin());
         }
      }
      else
      {
         // Use Karatsuba multiplication.

         using array_for_kara_tmp_type =
            typename std::conditional&lt;std::is_void&lt;Allocator&gt;::value,
                                      detail::static_array &lt;std::uint32_t, detail::a029750::a029750_as_constexpr(static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_number)) * 8U&gt;,
                                      detail::dynamic_array&lt;std::uint32_t, detail::a029750::a029750_as_constexpr(static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_number)) * 8U, Allocator&gt; &gt;::type;

         // Sloanes's A029747: Numbers of the form 2^k times 1, 3 or 5.
         const std::uint32_t kara_elems_for_multiply =
            detail::a029750::a029750_as_runtime_value(static_cast&lt;std::uint32_t&gt;(prec_elems_for_multiply));

         array_for_kara_tmp_type my_kara_mul_pool;

         std::uint32_t* result  = my_kara_mul_pool.data() + (kara_elems_for_multiply * 0U);
         std::uint32_t* t       = my_kara_mul_pool.data() + (kara_elems_for_multiply * 2U);
         std::uint32_t* u_local = my_kara_mul_pool.data() + (kara_elems_for_multiply * 6U);
         std::uint32_t* v_local = my_kara_mul_pool.data() + (kara_elems_for_multiply * 7U);

         std::copy(  data.cbegin(),   data.cbegin() + prec_elems_for_multiply, u_local);
         std::copy(v.data.cbegin(), v.data.cbegin() + prec_elems_for_multiply, v_local);

         eval_multiply_kara_n_by_n_to_2n(result,
                                         u_local,
                                         v_local,
                                         kara_elems_for_multiply,
                                         t);

         // Handle a potential carry.
         if(result[0U] != static_cast&lt;std::uint32_t&gt;(0U))
         {
            exp += static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);

            // Shift the result of the multiplication one element to the right.
            std::copy(result,
                      result + static_cast&lt;std::ptrdiff_t&gt;(prec_elems_for_multiply),
                      data.begin());
         }
         else
         {
            std::copy(result + 1,
                      result + (std::min)(static_cast&lt;std::int32_t&gt;(prec_elems_for_multiply + 1), cpp_dec_float_elem_number),
                      data.begin());
         }
      }
   }

   bool rd_string(const char* const s);

   template &lt;unsigned D, class ET, class A&gt;
   friend class cpp_dec_float;
};

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_radix;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10_limit_lo;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10_limit_hi;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const ExponentType cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_exp;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const ExponentType cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const ExponentType cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_exp10;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const ExponentType cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp10;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_elem_digits10;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_elem_number;
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
const std::int32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_elem_mask;

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::operator+=(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; v)
{
   if ((isnan)())
   {
      return *this;
   }

   if ((isinf)())
   {
      if ((v.isinf)() &amp;&amp; (isneg() != v.isneg()))
      {
         *this = nan();
      }
      return *this;
   }

   if (iszero())
   {
      return operator=(v);
   }

   if ((v.isnan)() || (v.isinf)())
   {
      *this = v;
      return *this;
   }

   // Get the offset for the add/sub operation.
   constexpr exponent_type max_delta_exp =
     static_cast&lt;exponent_type&gt;((cpp_dec_float_elem_number - 1) * cpp_dec_float_elem_digits10);

   const exponent_type ofs_exp = static_cast&lt;exponent_type&gt;(exp - v.exp);

   // Check if the operation is out of range, requiring special handling.
   if (v.iszero() || (ofs_exp &gt; max_delta_exp))
   {
      // Result is *this unchanged since v is negligible compared to *this.
      return *this;
   }
   else if (ofs_exp &lt; -max_delta_exp)
   {
      // Result is *this = v since *this is negligible compared to v.
      return operator=(v);
   }

   // Do the add/sub operation.

   typename array_type::pointer       p_u    = data.data();
   typename array_type::const_pointer p_v    = v.data.data();
   bool                               b_copy = false;
   const std::int32_t                 ofs    = static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(ofs_exp) / cpp_dec_float_elem_digits10);
   array_type                         n_data;

   if (neg == v.neg)
   {
      // Add v to *this, where the data array of either *this or v
      // might have to be treated with a positive, negative or zero offset.
      // The result is stored in *this. The data are added one element
      // at a time, each element with carry.
      if (ofs &gt;= static_cast&lt;std::int32_t&gt;(0))
      {
         std::copy(v.data.cbegin(), v.data.cend() - static_cast&lt;size_t&gt;(ofs), n_data.begin() + static_cast&lt;size_t&gt;(ofs));
         std::fill(n_data.begin(), n_data.begin() + static_cast&lt;size_t&gt;(ofs), static_cast&lt;std::uint32_t&gt;(0u));
         p_v = n_data.data();
      }
      else
      {
         std::copy(data.cbegin(), data.cend() - static_cast&lt;size_t&gt;(-ofs), n_data.begin() + static_cast&lt;size_t&gt;(-ofs));
         std::fill(n_data.begin(), n_data.begin() + static_cast&lt;size_t&gt;(-ofs), static_cast&lt;std::uint32_t&gt;(0u));
         p_u    = n_data.data();
         b_copy = true;
      }

      // Addition algorithm
      const std::uint32_t carry = eval_add_n(p_u, p_u, p_v, cpp_dec_float_elem_number);

      if (b_copy)
      {
         data = n_data;
         exp  = v.exp;
      }

      // There needs to be a carry into the element -1 of the array data
      if (carry != static_cast&lt;std::uint32_t&gt;(0u))
      {
         std::copy_backward(data.cbegin(), data.cend() - static_cast&lt;std::size_t&gt;(1u), data.end());
         data[0] = carry;
         exp += static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);
      }
   }
   else
   {
      // Subtract v from *this, where the data array of either *this or v
      // might have to be treated with a positive, negative or zero offset.
      if ((ofs &gt; static_cast&lt;std::int32_t&gt;(0)) || ((ofs == static_cast&lt;std::int32_t&gt;(0)) &amp;&amp; (compare_ranges(data.cbegin(), v.data.cbegin()) &gt; static_cast&lt;std::int32_t&gt;(0))))
      {
         // In this case, |u| &gt; |v| and ofs is positive.
         // Copy the data of v, shifted down to a lower value
         // into the data array m_n. Set the operand pointer p_v
         // to point to the copied, shifted data m_n.
         std::copy(v.data.cbegin(), v.data.cend() - static_cast&lt;size_t&gt;(ofs), n_data.begin() + static_cast&lt;size_t&gt;(ofs));
         std::fill(n_data.begin(), n_data.begin() + static_cast&lt;size_t&gt;(ofs), static_cast&lt;std::uint32_t&gt;(0u));
         p_v = n_data.data();
      }
      else
      {
         if (ofs != static_cast&lt;std::int32_t&gt;(0))
         {
            // In this case, |u| &lt; |v| and ofs is negative.
            // Shift the data of u down to a lower value.
            std::copy_backward(data.cbegin(), data.cend() - static_cast&lt;size_t&gt;(-ofs), data.end());
            std::fill(data.begin(), data.begin() + static_cast&lt;size_t&gt;(-ofs), static_cast&lt;std::uint32_t&gt;(0u));
         }

         // Copy the data of v into the data array n_data.
         // Set the u-pointer p_u to point to m_n and the
         // operand pointer p_v to point to the shifted
         // data m_data.
         n_data = v.data;
         p_u    = n_data.data();
         p_v    = data.data();
         b_copy = true;
      }

      // Subtraction algorithm
      static_cast&lt;void&gt;(eval_subtract_n(p_u, p_u, p_v, cpp_dec_float_elem_number));

      if (b_copy)
      {
         data = n_data;
         exp  = v.exp;
         neg  = v.neg;
      }

      // Is it necessary to justify the data?
      const typename array_type::const_iterator first_nonzero_elem = std::find_if(data.begin(), data.end(), data_elem_is_non_zero_predicate);

      if (first_nonzero_elem != data.begin())
      {
         if (first_nonzero_elem == data.end())
         {
            // This result of the subtraction is exactly zero.
            // Reset the sign and the exponent.
            neg = false;
            exp = static_cast&lt;exponent_type&gt;(0);
         }
         else
         {
            // Justify the data
            const std::size_t sj = static_cast&lt;std::size_t&gt;(std::distance&lt;typename array_type::const_iterator&gt;(data.begin(), first_nonzero_elem));

            std::copy(data.begin() + static_cast&lt;std::size_t&gt;(sj), data.end(), data.begin());
            std::fill(data.end() - sj, data.end(), static_cast&lt;std::uint32_t&gt;(0u));

            exp -= static_cast&lt;exponent_type&gt;(sj * static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_digits10));
         }
      }
   }

   // Handle underflow.
   if (iszero())
      return (*this = zero());

   // Check for potential overflow.
   const bool b_result_might_overflow = (exp &gt;= static_cast&lt;exponent_type&gt;(cpp_dec_float_max_exp10));

   // Handle overflow.
   if (b_result_might_overflow)
   {
      const bool b_result_is_neg = neg;
      neg                        = false;

      if (compare((cpp_dec_float::max)()) &gt; 0)
         *this = inf();

      neg = b_result_is_neg;
   }

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::operator-=(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; v)
{
   // Use *this - v = -(-*this + v).
   negate();
   *this += v;
   negate();
   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::operator*=(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; v)
{
   // Evaluate the sign of the result.
   const bool b_result_is_neg = (neg != v.neg);

   // Artificially set the sign of the result to be positive.
   neg = false;

   // Handle special cases like zero, inf and NaN.
   const bool b_u_is_inf  = (isinf)();
   const bool b_v_is_inf  = (v.isinf)();
   const bool b_u_is_zero = iszero();
   const bool b_v_is_zero = v.iszero();

   if (((isnan)() || (v.isnan)()) || (b_u_is_inf &amp;&amp; b_v_is_zero) || (b_v_is_inf &amp;&amp; b_u_is_zero))
   {
      *this = nan();
      return *this;
   }

   if (b_u_is_inf || b_v_is_inf)
   {
      *this = inf();
      if (b_result_is_neg)
         negate();
      return *this;
   }

   if (b_u_is_zero || b_v_is_zero)
   {
      return *this = zero();
   }

   // Check for potential overflow or underflow.
   const bool b_result_might_overflow  = ((exp + v.exp) &gt;= static_cast&lt;exponent_type&gt;(cpp_dec_float_max_exp10));
   const bool b_result_might_underflow = ((exp + v.exp) &lt;= static_cast&lt;exponent_type&gt;(cpp_dec_float_min_exp10));

   // Set the exponent of the result.
   exp += v.exp;

   const std::int32_t prec_mul = (std::min)(prec_elem, v.prec_elem);

   eval_mul_dispatch_multiplication_method(v, prec_mul);

   // Handle overflow.
   if (b_result_might_overflow &amp;&amp; (compare((cpp_dec_float::max)()) &gt; 0))
   {
      *this = inf();
   }

   // Handle underflow.
   if (b_result_might_underflow &amp;&amp; (compare((cpp_dec_float::min)()) &lt; 0))
   {
      *this = zero();

      return *this;
   }

   // Set the sign of the result.
   neg = b_result_is_neg;

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::operator/=(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; v)
{
   if (iszero())
   {
      if ((v.isnan)())
      {
         return *this = v;
      }
      else if (v.iszero())
      {
         return *this = nan();
      }
   }

   const bool u_and_v_are_finite_and_identical = ((isfinite)() &amp;&amp; (fpclass == v.fpclass) &amp;&amp; (exp == v.exp) &amp;&amp; (compare_ranges(data.cbegin(), v.data.cbegin()) == static_cast&lt;std::int32_t&gt;(0)));

   if (u_and_v_are_finite_and_identical)
   {
      if (neg != v.neg)
      {
         *this = one();
         negate();
      }
      else
         *this = one();
      return *this;
   }
   else
   {
      cpp_dec_float t(v);
      t.calculate_inv();
      return operator*=(t);
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::mul_unsigned_long_long(const boost::ulong_long_type n)
{
   // Multiply *this with a constant boost::ulong_long_type.

   // Evaluate the sign of the result.
   const bool b_neg = neg;

   // Artificially set the sign of the result to be positive.
   neg = false;

   // Handle special cases like zero, inf and NaN.
   const bool b_u_is_inf  = (isinf)();
   const bool b_n_is_zero = (n == static_cast&lt;std::int32_t&gt;(0));

   if ((isnan)() || (b_u_is_inf &amp;&amp; b_n_is_zero))
   {
      return (*this = nan());
   }

   if (b_u_is_inf)
   {
      *this = inf();
      if (b_neg)
         negate();
      return *this;
   }

   if (iszero() || b_n_is_zero)
   {
      // Multiplication by zero.
      return *this = zero();
   }

   if (n &gt;= static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask))
   {
      neg = b_neg;
      cpp_dec_float t;
      t = n;
      return operator*=(t);
   }

   if (n == static_cast&lt;boost::ulong_long_type&gt;(1u))
   {
      neg = b_neg;
      return *this;
   }

   // Set up the multiplication loop.
   const std::uint32_t nn    = static_cast&lt;std::uint32_t&gt;(n);
   const std::uint32_t carry = mul_loop_n(data.data(), nn, prec_elem);

   // Handle the carry and adjust the exponent.
   if (carry != static_cast&lt;std::uint32_t&gt;(0u))
   {
      exp += static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);

      // Shift the result of the multiplication one element to the right.
      std::copy_backward(data.begin(),
                         data.begin() + static_cast&lt;std::size_t&gt;(prec_elem - static_cast&lt;std::int32_t&gt;(1)),
                         data.begin() + static_cast&lt;std::size_t&gt;(prec_elem));

      data.front() = static_cast&lt;std::uint32_t&gt;(carry);
   }

   // Check for potential overflow.
   const bool b_result_might_overflow = (exp &gt;= cpp_dec_float_max_exp10);

   // Handle overflow.
   if (b_result_might_overflow &amp;&amp; (compare((cpp_dec_float::max)()) &gt; 0))
   {
      *this = inf();
   }

   // Set the sign.
   neg = b_neg;

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::div_unsigned_long_long(const boost::ulong_long_type n)
{
   // Divide *this by a constant boost::ulong_long_type.

   // Evaluate the sign of the result.
   const bool b_neg = neg;

   // Artificially set the sign of the result to be positive.
   neg = false;

   // Handle special cases like zero, inf and NaN.
   if ((isnan)())
   {
      return *this;
   }

   if ((isinf)())
   {
      *this = inf();
      if (b_neg)
         negate();
      return *this;
   }

   if (n == static_cast&lt;boost::ulong_long_type&gt;(0u))
   {
      // Divide by 0.
      if (iszero())
      {
         *this = nan();
         return *this;
      }
      else
      {
         *this = inf();
         if (isneg())
            negate();
         return *this;
      }
   }

   if (iszero())
   {
      return *this;
   }

   if (n &gt;= static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask))
   {
      neg = b_neg;
      cpp_dec_float t;
      t = n;
      return operator/=(t);
   }

   const std::uint32_t nn = static_cast&lt;std::uint32_t&gt;(n);

   if (nn &gt; static_cast&lt;std::uint32_t&gt;(1u))
   {
      // Do the division loop.
      const std::uint32_t prev = div_loop_n(data.data(), nn, prec_elem);

      // Determine if one leading zero is in the result data.
      if (data[0] == static_cast&lt;std::uint32_t&gt;(0u))
      {
         // Adjust the exponent
         exp -= static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10);

         // Shift result of the division one element to the left.
         std::copy(data.begin() + static_cast&lt;std::size_t&gt;(1u),
                   data.begin() + static_cast&lt;std::size_t&gt;(prec_elem - static_cast&lt;std::int32_t&gt;(1)),
                   data.begin());

         data[prec_elem - static_cast&lt;std::int32_t&gt;(1)] = static_cast&lt;std::uint32_t&gt;(static_cast&lt;std::uint64_t&gt;(prev * static_cast&lt;std::uint64_t&gt;(cpp_dec_float_elem_mask)) / nn);
      }
   }

   // Check for potential underflow.
   const bool b_result_might_underflow = (exp &lt;= cpp_dec_float_min_exp10);

   // Handle underflow.
   if (b_result_might_underflow &amp;&amp; (compare((cpp_dec_float::min)()) &lt; 0))
      return (*this = zero());

   // Set the sign of the result.
   neg = b_neg;

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::calculate_inv()
{
   // Compute the inverse of *this.
   const bool b_neg = neg;

   neg = false;

   // Handle special cases like zero, inf and NaN.
   if (iszero())
   {
      *this = inf();
      if (b_neg)
         negate();
      return *this;
   }

   if ((isnan)())
   {
      return *this;
   }

   if ((isinf)())
   {
      return *this = zero();
   }

   if (isone())
   {
      if (b_neg)
         negate();
      return *this;
   }

   // Save the original *this.
   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; x(*this);

   // Generate the initial estimate using division.
   // Extract the mantissa and exponent for a &quot;manual&quot;
   // computation of the estimate.
   double        dd;
   exponent_type ne;
   x.extract_parts(dd, ne);

   // Do the inverse estimate using double precision estimates of mantissa and exponent.
   operator=(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;(1.0 / dd, -ne));

   // Compute the inverse of *this. Quadratically convergent Newton-Raphson iteration
   // is used. During the iterative steps, the precision of the calculation is limited
   // to the minimum required in order to minimize the run-time.

   constexpr std::int32_t double_digits10_minus_a_few = std::numeric_limits&lt;double&gt;::digits10 - 3;

   for (std::int32_t digits = double_digits10_minus_a_few; digits &lt;= cpp_dec_float_max_digits10; digits *= static_cast&lt;std::int32_t&gt;(2))
   {
      // Adjust precision of the terms.
      precision(static_cast&lt;std::int32_t&gt;((digits + 10) * static_cast&lt;std::int32_t&gt;(2)));
      x.precision(static_cast&lt;std::int32_t&gt;((digits + 10) * static_cast&lt;std::int32_t&gt;(2)));

      // Next iteration.
      cpp_dec_float t(*this);
      t *= x;
      t -= two();
      t.negate();
      *this *= t;
   }

   neg = b_neg;

   prec_elem = cpp_dec_float_elem_number;

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::calculate_sqrt()
{
   // Compute the square root of *this.

   if ((isinf)() &amp;&amp; !isneg())
   {
      return *this;
   }

   if (isneg() || (!(isfinite)()))
   {
      *this = nan();
      errno = EDOM;
      return *this;
   }

   if (iszero() || isone())
   {
      return *this;
   }

   // Save the original *this.
   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; x(*this);

   // Generate the initial estimate using division.
   // Extract the mantissa and exponent for a &quot;manual&quot;
   // computation of the estimate.
   double        dd;
   exponent_type ne;
   extract_parts(dd, ne);

   // Force the exponent to be an even multiple of two.
   if ((ne % static_cast&lt;exponent_type&gt;(2)) != static_cast&lt;exponent_type&gt;(0))
   {
      ++ne;
      dd /= 10.0;
   }

   // Setup the iteration.
   // Estimate the square root using simple manipulations.
   const double sqd = std::sqrt(dd);

   *this = cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;(sqd, static_cast&lt;ExponentType&gt;(ne / static_cast&lt;ExponentType&gt;(2)));

   // Estimate 1.0 / (2.0 * x0) using simple manipulations.
   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; vi(0.5 / sqd, static_cast&lt;ExponentType&gt;(-ne / static_cast&lt;ExponentType&gt;(2)));

   // Compute the square root of x. Coupled Newton iteration
   // as described in &quot;Pi Unleashed&quot; is used. During the
   // iterative steps, the precision of the calculation is
   // limited to the minimum required in order to minimize
   // the run-time.
   //
   // Book reference to &quot;Pi Unleashed:
   // <a href="https://www.springer.com/gp/book/9783642567353">https://www.springer.com/gp/book/9783642567353</a>

   constexpr std::uint32_t double_digits10_minus_a_few = std::numeric_limits&lt;double&gt;::digits10 - 3;

   for (std::int32_t digits = double_digits10_minus_a_few; digits &lt;= cpp_dec_float_max_digits10; digits *= 2u)
   {
      // Adjust precision of the terms.
      precision((digits + 10) * 2);
      vi.precision((digits + 10) * 2);

      // Next iteration of vi
      cpp_dec_float t(*this);
      t *= vi;
      t.negate();
      t.mul_unsigned_long_long(2u);
      t += one();
      t *= vi;
      vi += t;

      // Next iteration of *this
      t = *this;
      t *= *this;
      t.negate();
      t += x;
      t *= vi;
      *this += t;
   }

   prec_elem = cpp_dec_float_elem_number;

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
int cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::compare(const cpp_dec_float&amp; v) const
{
   // Compare v with *this.
   // Return +1 for *this &gt; v
   // 0 for *this = v
   // -1 for *this &lt; v

   // Handle all non-finite cases.
   if ((!(isfinite)()) || (!(v.isfinite)()))
   {
      // NaN can never equal NaN. Return an implementation-dependent
      // signed result. Also note that comparison of NaN with NaN
      // using operators greater-than or less-than is undefined.
      if ((isnan)() || (v.isnan)())
      {
         return ((isnan)() ? 1 : -1);
      }

      if ((isinf)() &amp;&amp; (v.isinf)())
      {
         // Both *this and v are infinite. They are equal if they have the same sign.
         // Otherwise, *this is less than v if and only if *this is negative.
         return ((neg == v.neg) ? 0 : (neg ? -1 : 1));
      }

      if ((isinf)())
      {
         // *this is infinite, but v is finite.
         // So negative infinite *this is less than any finite v.
         // Whereas positive infinite *this is greater than any finite v.
         return (isneg() ? -1 : 1);
      }
      else
      {
         // *this is finite, and v is infinite.
         // So any finite *this is greater than negative infinite v.
         // Whereas any finite *this is less than positive infinite v.
         return (v.neg ? 1 : -1);
      }
   }

   // And now handle all *finite* cases.
   if (iszero())
   {
      // The value of *this is zero and v is either zero or non-zero.
      return (v.iszero() ? 0
                         : (v.neg ? 1 : -1));
   }
   else if (v.iszero())
   {
      // The value of v is zero and *this is non-zero.
      return (neg ? -1 : 1);
   }
   else
   {
      // Both *this and v are non-zero.

      if (neg != v.neg)
      {
         // The signs are different.
         return (neg ? -1 : 1);
      }
      else if (exp != v.exp)
      {
         // The signs are the same and the exponents are different.
         const int val_cexpression = ((exp &lt; v.exp) ? 1 : -1);

         return (neg ? val_cexpression : -val_cexpression);
      }
      else
      {
         // The signs are the same and the exponents are the same.
         // Compare the data.
         const int val_cmp_data = compare_ranges(data.cbegin(), v.data.cbegin());

         return ((!neg) ? val_cmp_data : -val_cmp_data);
      }
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
bool cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::isone() const
{
   // Check if the value of *this is identically 1 or very close to 1.

   const bool not_negative_and_is_finite = ((!neg) &amp;&amp; (isfinite)());

   if (not_negative_and_is_finite)
   {
      if ((data[0u] == static_cast&lt;std::uint32_t&gt;(1u)) &amp;&amp; (exp == static_cast&lt;exponent_type&gt;(0)))
      {
         const typename array_type::const_iterator it_non_zero = std::find_if(data.begin(), data.end(), data_elem_is_non_zero_predicate);
         return (it_non_zero == data.end());
      }
      else if ((data[0u] == static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask - 1)) &amp;&amp; (exp == static_cast&lt;exponent_type&gt;(-cpp_dec_float_elem_digits10)))
      {
         const typename array_type::const_iterator it_non_nine = std::find_if(data.begin(), data.end(), data_elem_is_non_nine_predicate);
         return (it_non_nine == data.end());
      }
   }

   return false;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
bool cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::isint() const
{
   if (fpclass != cpp_dec_float_finite)
   {
      return false;
   }

   if (iszero())
   {
      return true;
   }

   if (exp &lt; static_cast&lt;exponent_type&gt;(0))
   {
      return false;
   } // |*this| &lt; 1.

   const typename array_type::size_type offset_decimal_part = static_cast&lt;typename array_type::size_type&gt;(exp / cpp_dec_float_elem_digits10) + 1u;

   if (offset_decimal_part &gt;= static_cast&lt;typename array_type::size_type&gt;(cpp_dec_float_elem_number))
   {
      // The number is too large to resolve the integer part.
      // It considered to be a pure integer.
      return true;
   }

   typename array_type::const_iterator it_non_zero = std::find_if(data.begin() + offset_decimal_part, data.end(), data_elem_is_non_zero_predicate);

   return (it_non_zero == data.end());
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_parts(double&amp; mantissa, ExponentType&amp; exponent) const
{
   // Extract the approximate parts mantissa and base-10 exponent from the input cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; value x.

   // Extracts the mantissa and exponent.
   exponent = exp;

   std::uint32_t p10  = static_cast&lt;std::uint32_t&gt;(1u);
   std::uint32_t test = data[0u];

   for (;;)
   {
      test /= static_cast&lt;std::uint32_t&gt;(10u);

      if (test == static_cast&lt;std::uint32_t&gt;(0u))
      {
         break;
      }

      p10 *= static_cast&lt;std::uint32_t&gt;(10u);
      ++exponent;
   }

   // Establish the upper bound of limbs for extracting the double.
   const int max_elem_in_double_count = static_cast&lt;int&gt;(static_cast&lt;std::int32_t&gt;(std::numeric_limits&lt;double&gt;::digits10) / cpp_dec_float_elem_digits10) + (static_cast&lt;int&gt;(static_cast&lt;std::int32_t&gt;(std::numeric_limits&lt;double&gt;::digits10) % cpp_dec_float_elem_digits10) != 0 ? 1 : 0) + 1;

   // And make sure this upper bound stays within bounds of the elems.
   const std::size_t max_elem_extract_count = static_cast&lt;std::size_t&gt;((std::min)(static_cast&lt;std::int32_t&gt;(max_elem_in_double_count), cpp_dec_float_elem_number));

   // Extract into the mantissa the first limb, extracted as a double.
   mantissa     = static_cast&lt;double&gt;(data[0]);
   double scale = 1.0;

   // Extract the rest of the mantissa piecewise from the limbs.
   for (std::size_t i = 1u; i &lt; max_elem_extract_count; i++)
   {
      scale /= static_cast&lt;double&gt;(cpp_dec_float_elem_mask);
      mantissa += (static_cast&lt;double&gt;(data[i]) * scale);
   }

   mantissa /= static_cast&lt;double&gt;(p10);

   if (neg)
   {
      mantissa = -mantissa;
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
double cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_double() const
{
   // Returns the double conversion of a cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;.

   // Check for non-normal cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;.
   if (!(isfinite)())
   {
      if ((isnan)())
      {
         return std::numeric_limits&lt;double&gt;::quiet_NaN();
      }
      else
      {
         return ((!neg) ? std::numeric_limits&lt;double&gt;::infinity()
                        : -std::numeric_limits&lt;double&gt;::infinity());
      }
   }

   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; xx(*this);
   if (xx.isneg())
      xx.negate();

   // Check if *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; is zero.
   if (iszero() || (xx.compare(double_min()) &lt; 0))
   {
      return 0.0;
   }

   // Check if *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; exceeds the maximum of double.
   if (xx.compare(double_max()) &gt; 0)
   {
      return ((!neg) ? std::numeric_limits&lt;double&gt;::infinity()
                     : -std::numeric_limits&lt;double&gt;::infinity());
   }

   std::stringstream ss;
   ss.imbue(std::locale::classic());

   ss &lt;&lt; str(std::numeric_limits&lt;double&gt;::digits10 + (2 + 1), std::ios_base::scientific);

   double d;
   ss &gt;&gt; d;

   return d;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
long double cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_long_double() const
{
   // Returns the long double conversion of a cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;.

   // Check if *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; is subnormal.
   if (!(isfinite)())
   {
      if ((isnan)())
      {
         return std::numeric_limits&lt;long double&gt;::quiet_NaN();
      }
      else
      {
         return ((!neg) ? std::numeric_limits&lt;long double&gt;::infinity()
                        : -std::numeric_limits&lt;long double&gt;::infinity());
      }
   }

   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; xx(*this);
   if (xx.isneg())
      xx.negate();

   // Check if *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; is zero.
   if (iszero() || (xx.compare(long_double_min()) &lt; 0))
   {
      return static_cast&lt;long double&gt;(0.0);
   }

   // Check if *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; exceeds the maximum of double.
   if (xx.compare(long_double_max()) &gt; 0)
   {
      return ((!neg) ? std::numeric_limits&lt;long double&gt;::infinity()
                     : -std::numeric_limits&lt;long double&gt;::infinity());
   }

   std::stringstream ss;
   ss.imbue(std::locale::classic());

   ss &lt;&lt; str(std::numeric_limits&lt;long double&gt;::digits10 + (2 + 1), std::ios_base::scientific);

   long double ld;
   ss &gt;&gt; ld;

   return ld;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
boost::long_long_type cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_signed_long_long() const
{
   // Extracts a signed long long from *this.
   // If (x &gt; maximum of long long) or (x &lt; minimum of long long),
   // then the maximum or minimum of long long is returned accordingly.

   if (exp &lt; static_cast&lt;exponent_type&gt;(0))
   {
      return static_cast&lt;boost::long_long_type&gt;(0);
   }

   const bool b_neg = isneg();

   boost::ulong_long_type val;

   if ((!b_neg) &amp;&amp; (compare(long_long_max()) &gt; 0))
   {
      return (std::numeric_limits&lt;boost::long_long_type&gt;::max)();
   }
   else if (b_neg &amp;&amp; (compare(long_long_min()) &lt; 0))
   {
      return (std::numeric_limits&lt;boost::long_long_type&gt;::min)();
   }
   else
   {
      // Extract the data into an boost::ulong_long_type value.
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; xn(extract_integer_part());
      if (xn.isneg())
         xn.negate();

      val = static_cast&lt;boost::ulong_long_type&gt;(xn.data[0]);

      const std::int32_t imax = (std::min)(static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(xn.exp) / cpp_dec_float_elem_digits10), static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_number - static_cast&lt;std::int32_t&gt;(1)));

      for (std::int32_t i = static_cast&lt;std::int32_t&gt;(1); i &lt;= imax; i++)
      {
         val *= static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask);
         val += static_cast&lt;boost::ulong_long_type&gt;(xn.data[i]);
      }
   }

   if (!b_neg)
   {
      return static_cast&lt;boost::long_long_type&gt;(val);
   }
   else
   {
      // This strange expression avoids a hardware trap in the corner case
      // that val is the most negative value permitted in boost::long_long_type.
      // See <a href="https://svn.boost.org/trac/boost/ticket/9740">https://svn.boost.org/trac/boost/ticket/9740</a>.
      //
      boost::long_long_type sval = static_cast&lt;boost::long_long_type&gt;(val - 1);
      sval                       = -sval;
      --sval;
      return sval;
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
boost::ulong_long_type cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_unsigned_long_long() const
{
   // Extracts an boost::ulong_long_type from *this.
   // If x exceeds the maximum of boost::ulong_long_type,
   // then the maximum of boost::ulong_long_type is returned.
   // If x is negative, then the boost::ulong_long_type cast of
   // the long long extracted value is returned.

   if (isneg())
   {
      return static_cast&lt;boost::ulong_long_type&gt;(extract_signed_long_long());
   }

   if (exp &lt; static_cast&lt;exponent_type&gt;(0))
   {
      return static_cast&lt;boost::ulong_long_type&gt;(0u);
   }

   const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; xn(extract_integer_part());

   boost::ulong_long_type val;

   if (xn.compare(ulong_long_max()) &gt; 0)
   {
      return (std::numeric_limits&lt;boost::ulong_long_type&gt;::max)();
   }
   else
   {
      // Extract the data into an boost::ulong_long_type value.
      val = static_cast&lt;boost::ulong_long_type&gt;(xn.data[0]);

      const std::int32_t imax = (std::min)(static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(xn.exp) / cpp_dec_float_elem_digits10), static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_number - static_cast&lt;std::int32_t&gt;(1)));

      for (std::int32_t i = static_cast&lt;std::int32_t&gt;(1); i &lt;= imax; i++)
      {
         val *= static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask);
         val += static_cast&lt;boost::ulong_long_type&gt;(xn.data[i]);
      }
   }

   return val;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::extract_integer_part() const
{
   // Compute the signed integer part of x.

   if (!(isfinite)())
   {
      return *this;
   }

   if (exp &lt; static_cast&lt;ExponentType&gt;(0))
   {
      // The absolute value of the number is smaller than 1.
      // Thus the integer part is zero.
      return zero();
   }

   // Truncate the digits from the decimal part, including guard digits
   // that do not belong to the integer part.

   // Make a local copy.
   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; x = *this;

   // Clear out the decimal portion
   const size_t first_clear = (static_cast&lt;size_t&gt;(x.exp) / static_cast&lt;size_t&gt;(cpp_dec_float_elem_digits10)) + 1u;
   const size_t last_clear  = static_cast&lt;size_t&gt;(cpp_dec_float_elem_number);

   if (first_clear &lt; last_clear)
      std::fill(x.data.begin() + first_clear, x.data.begin() + last_clear, static_cast&lt;std::uint32_t&gt;(0u));

   return x;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
std::string cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::str(std::intmax_t number_of_digits, std::ios_base::fmtflags f) const
{
   if ((this-&gt;isinf)())
   {
      if (this-&gt;isneg())
         return &quot;-inf&quot;;
      else if (f &amp; std::ios_base::showpos)
         return &quot;+inf&quot;;
      else
         return &quot;inf&quot;;
   }
   else if ((this-&gt;isnan)())
   {
      return &quot;nan&quot;;
   }

   std::string     str;
   std::intmax_t org_digits(number_of_digits);
   exponent_type    my_exp = order();

   if (number_of_digits == 0)
      number_of_digits = cpp_dec_float_max_digits10;

   if (f &amp; std::ios_base::fixed)
   {
      number_of_digits += my_exp + 1;
   }
   else if (f &amp; std::ios_base::scientific)
      ++number_of_digits;
   // Determine the number of elements needed to provide the requested digits from cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;.
   const std::size_t number_of_elements = (std::min)(static_cast&lt;std::size_t&gt;((number_of_digits / static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_digits10)) + 2u),
                                                     static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_number));

   // Extract the remaining digits from cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; after the decimal point.
   std::stringstream ss;
   ss.imbue(std::locale::classic());
   ss &lt;&lt; data[0];
   // Extract all of the digits from cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, beginning with the first data element.
   for (std::size_t i = static_cast&lt;std::size_t&gt;(1u); i &lt; number_of_elements; i++)
   {
      ss &lt;&lt; std::setw(static_cast&lt;std::streamsize&gt;(cpp_dec_float_elem_digits10))
         &lt;&lt; std::setfill(static_cast&lt;char&gt;('0'))
         &lt;&lt; data[i];
   }
   str += ss.str();

   bool have_leading_zeros = false;

   if (number_of_digits == 0)
   {
      // We only get here if the output format is &quot;fixed&quot; and we just need to
      // round the first non-zero digit.
      number_of_digits -= my_exp + 1; // reset to original value
      str.insert(static_cast&lt;std::string::size_type&gt;(0), std::string::size_type(number_of_digits), '0');
      have_leading_zeros = true;
   }

   if (number_of_digits &lt; 0)
   {
      str = &quot;0&quot;;
      if (isneg())
         str.insert(static_cast&lt;std::string::size_type&gt;(0), 1, '-');
      boost::multiprecision::detail::format_float_string(str, 0, number_of_digits - my_exp - 1, f, this-&gt;iszero());
      return str;
   }
   else
   {
      // Cut the output to the size of the precision.
      if (str.length() &gt; static_cast&lt;std::string::size_type&gt;(number_of_digits))
      {
         // Get the digit after the last needed digit for rounding
         const std::uint32_t round = static_cast&lt;std::uint32_t&gt;(static_cast&lt;std::uint32_t&gt;(str[static_cast&lt;std::string::size_type&gt;(number_of_digits)]) - static_cast&lt;std::uint32_t&gt;('0'));

         bool need_round_up = round &gt;= 5u;

         if (round == 5u)
         {
            const std::uint32_t ix = static_cast&lt;std::uint32_t&gt;(static_cast&lt;std::uint32_t&gt;(str[static_cast&lt;std::string::size_type&gt;(number_of_digits - 1)]) - static_cast&lt;std::uint32_t&gt;('0'));
            if ((ix &amp; 1u) == 0)
            {
               // We have an even digit followed by a 5, so we might not actually need to round up
               // if all the remaining digits are zero:
               if (str.find_first_not_of('0', static_cast&lt;std::string::size_type&gt;(number_of_digits + 1)) == std::string::npos)
               {
                  bool all_zeros = true;
                  // No none-zero trailing digits in the string, now check whatever parts we didn't convert to the string:
                  for (std::size_t i = number_of_elements; i &lt; data.size(); i++)
                  {
                     if (data[i])
                     {
                        all_zeros = false;
                        break;
                     }
                  }
                  if (all_zeros)
                     need_round_up = false; // tie break - round to even.
               }
            }
         }

         // Truncate the string
         str.erase(static_cast&lt;std::string::size_type&gt;(number_of_digits));

         if (need_round_up)
         {
            std::size_t ix = static_cast&lt;std::size_t&gt;(str.length() - 1u);

            // Every trailing 9 must be rounded up
            while (ix &amp;&amp; (static_cast&lt;std::int32_t&gt;(str.at(ix)) - static_cast&lt;std::int32_t&gt;('0') == static_cast&lt;std::int32_t&gt;(9)))
            {
               str.at(ix) = static_cast&lt;char&gt;('0');
               --ix;
            }

            if (!ix)
            {
               // There were nothing but trailing nines.
               if (static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(str.at(ix)) - static_cast&lt;std::int32_t&gt;(0x30)) == static_cast&lt;std::int32_t&gt;(9))
               {
                  // Increment up to the next order and adjust exponent.
                  str.at(ix) = static_cast&lt;char&gt;('1');
                  ++my_exp;
               }
               else
               {
                  // Round up this digit.
                  ++str.at(ix);
               }
            }
            else
            {
               // Round up the last digit.
               ++str[ix];
            }
         }
      }
   }

   if (have_leading_zeros)
   {
      // We need to take the zeros back out again, and correct the exponent
      // if we rounded up:
      if (str[std::string::size_type(number_of_digits - 1)] != '0')
      {
         ++my_exp;
         str.erase(0, std::string::size_type(number_of_digits - 1));
      }
      else
         str.erase(0, std::string::size_type(number_of_digits));
   }

   if (isneg())
      str.insert(static_cast&lt;std::string::size_type&gt;(0), 1, '-');

   boost::multiprecision::detail::format_float_string(str, my_exp, org_digits, f, this-&gt;iszero());
   return str;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
bool cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::rd_string(const char* const s)
{
#ifndef BOOST_NO_EXCEPTIONS
   try
   {
#endif

      std::string str(s);

      // TBD: Using several regular expressions may significantly reduce
      // the code complexity (and perhaps the run-time) of rd_string().

      // Get a possible exponent and remove it.
      exp = static_cast&lt;exponent_type&gt;(0);

      std::size_t pos;

      if (((pos = str.find('e')) != std::string::npos) || ((pos = str.find('E')) != std::string::npos))
      {
         // Remove the exponent part from the string.
         exp = boost::lexical_cast&lt;exponent_type&gt;(static_cast&lt;const char*&gt;(str.c_str() + (pos + 1u)));
         str = str.substr(static_cast&lt;std::size_t&gt;(0u), pos);
      }

      // Get a possible +/- sign and remove it.
      neg = false;

      if (str.size())
      {
         if (str[0] == '-')
         {
            neg = true;
            str.erase(0, 1);
         }
         else if (str[0] == '+')
         {
            str.erase(0, 1);
         }
      }
      //
      // Special cases for infinities and NaN's:
      //
      if ((str == &quot;inf&quot;) || (str == &quot;INF&quot;) || (str == &quot;infinity&quot;) || (str == &quot;INFINITY&quot;))
      {
         if (neg)
         {
            *this = this-&gt;inf();
            this-&gt;negate();
         }
         else
            *this = this-&gt;inf();
         return true;
      }
      if ((str.size() &gt;= 3) &amp;&amp; ((str.substr(0, 3) == &quot;nan&quot;) || (str.substr(0, 3) == &quot;NAN&quot;) || (str.substr(0, 3) == &quot;NaN&quot;)))
      {
         *this = this-&gt;nan();
         return true;
      }

      // Remove the leading zeros for all input types.
      const std::string::iterator fwd_it_leading_zero = std::find_if(str.begin(), str.end(), char_is_nonzero_predicate);

      if (fwd_it_leading_zero != str.begin())
      {
         if (fwd_it_leading_zero == str.end())
         {
            // The string contains nothing but leading zeros.
            // This string represents zero.
            operator=(zero());
            return true;
         }
         else
         {
            str.erase(str.begin(), fwd_it_leading_zero);
         }
      }

      // Put the input string into the standard cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; input form
      // aaa.bbbbE+/-n, where aaa has 1...cpp_dec_float_elem_digits10, bbbb has an
      // even multiple of cpp_dec_float_elem_digits10 which are possibly zero padded
      // on the right-end, and n is a signed 64-bit integer which is an
      // even multiple of cpp_dec_float_elem_digits10.

      // Find a possible decimal point.
      pos = str.find(static_cast&lt;char&gt;('.'));

      if (pos != std::string::npos)
      {
         // Remove all trailing insignificant zeros.
         const std::string::const_reverse_iterator rit_non_zero = std::find_if(str.rbegin(), str.rend(), char_is_nonzero_predicate);

         if (rit_non_zero != static_cast&lt;std::string::const_reverse_iterator&gt;(str.rbegin()))
         {
            const std::string::size_type ofs = str.length() - std::distance&lt;std::string::const_reverse_iterator&gt;(str.rbegin(), rit_non_zero);
            str.erase(str.begin() + ofs, str.end());
         }

         // Check if the input is identically zero.
         if (str == std::string(&quot;.&quot;))
         {
            operator=(zero());
            return true;
         }

         // Remove leading significant zeros just after the decimal point
         // and adjust the exponent accordingly.
         // Note that the while-loop operates only on strings of the form &quot;.000abcd...&quot;
         // and peels away the zeros just after the decimal point.
         if (str.at(static_cast&lt;std::size_t&gt;(0u)) == static_cast&lt;char&gt;('.'))
         {
            const std::string::iterator it_non_zero = std::find_if(str.begin() + 1u, str.end(), char_is_nonzero_predicate);

            std::size_t delta_exp = static_cast&lt;std::size_t&gt;(0u);

            if (str.at(static_cast&lt;std::size_t&gt;(1u)) == static_cast&lt;char&gt;('0'))
            {
               delta_exp = std::distance&lt;std::string::const_iterator&gt;(str.begin() + 1u, it_non_zero);
            }

            // Bring one single digit into the mantissa and adjust the exponent accordingly.
            str.erase(str.begin(), it_non_zero);
            str.insert(static_cast&lt;std::string::size_type&gt;(1u), &quot;.&quot;);
            exp -= static_cast&lt;exponent_type&gt;(delta_exp + 1u);
         }
      }
      else
      {
         // Input string has no decimal point: Append decimal point.
         str.append(&quot;.&quot;);
      }

      // Shift the decimal point such that the exponent is an even multiple of cpp_dec_float_elem_digits10.
      std::ptrdiff_t       n_shift   = static_cast&lt;std::ptrdiff_t&gt;(0);
      const std::ptrdiff_t n_exp_rem = static_cast&lt;std::ptrdiff_t&gt;(exp % static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10));

      if((exp % static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10)) != static_cast&lt;exponent_type&gt;(0))
      {
         n_shift = ((exp &lt; static_cast&lt;exponent_type&gt;(0))
                        ? static_cast&lt;std::ptrdiff_t&gt;(n_exp_rem + static_cast&lt;std::ptrdiff_t&gt;(cpp_dec_float_elem_digits10))
                        : static_cast&lt;std::ptrdiff_t&gt;(n_exp_rem));
      }

      // Make sure that there are enough digits for the decimal point shift.
      pos = str.find(static_cast&lt;char&gt;('.'));

      std::ptrdiff_t pos_plus_one = static_cast&lt;std::ptrdiff_t&gt;(pos + 1);

      if ((static_cast&lt;std::ptrdiff_t&gt;(str.length()) - pos_plus_one) &lt; n_shift)
      {
         const std::ptrdiff_t sz = static_cast&lt;std::ptrdiff_t&gt;(n_shift - (static_cast&lt;std::ptrdiff_t&gt;(str.length()) - pos_plus_one));

         str.append(std::string(static_cast&lt;std::string::size_type&gt;(sz), static_cast&lt;char&gt;('0')));
      }

      // Do the decimal point shift.
      if (n_shift != static_cast&lt;std::ptrdiff_t&gt;(0))
      {
         str.insert(static_cast&lt;std::string::size_type&gt;(pos_plus_one + n_shift), &quot;.&quot;);

         str.erase(pos, static_cast&lt;std::ptrdiff_t&gt;(1));

         exp -= static_cast&lt;exponent_type&gt;(n_shift);
      }

      // Cut the size of the mantissa to &lt;= cpp_dec_float_elem_digits10.
      pos          = str.find(static_cast&lt;char&gt;('.'));
      pos_plus_one = static_cast&lt;std::size_t&gt;(pos + 1u);

      if (pos &gt; static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_digits10))
      {
         const std::int32_t n_pos         = static_cast&lt;std::int32_t&gt;(pos);
         const std::int32_t n_rem_is_zero = ((static_cast&lt;std::int32_t&gt;(n_pos % cpp_dec_float_elem_digits10) == static_cast&lt;std::int32_t&gt;(0)) ? static_cast&lt;std::int32_t&gt;(1) : static_cast&lt;std::int32_t&gt;(0));
         const std::int32_t n             = static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(n_pos / cpp_dec_float_elem_digits10) - n_rem_is_zero);

         str.insert(static_cast&lt;std::size_t&gt;(static_cast&lt;std::int32_t&gt;(n_pos - static_cast&lt;std::int32_t&gt;(n * cpp_dec_float_elem_digits10))), &quot;.&quot;);

         str.erase(pos_plus_one, static_cast&lt;std::size_t&gt;(1u));

         exp += static_cast&lt;exponent_type&gt;(static_cast&lt;exponent_type&gt;(n) * static_cast&lt;exponent_type&gt;(cpp_dec_float_elem_digits10));
      }

      // Pad the decimal part such that its value is an even
      // multiple of cpp_dec_float_elem_digits10.
      pos          = str.find(static_cast&lt;char&gt;('.'));
      pos_plus_one = static_cast&lt;std::size_t&gt;(pos + 1u);

      const std::int32_t n_dec = static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(str.length() - 1u) - static_cast&lt;std::int32_t&gt;(pos));
      const std::int32_t n_rem = static_cast&lt;std::int32_t&gt;(n_dec % cpp_dec_float_elem_digits10);

      std::int32_t n_cnt = ((n_rem != static_cast&lt;std::int32_t&gt;(0))
                                  ? static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_digits10 - n_rem)
                                  : static_cast&lt;std::int32_t&gt;(0));

      if (n_cnt != static_cast&lt;std::int32_t&gt;(0))
      {
         str.append(static_cast&lt;std::size_t&gt;(n_cnt), static_cast&lt;char&gt;('0'));
      }

      // Truncate decimal part if it is too long.
      const std::size_t max_dec = static_cast&lt;std::size_t&gt;((cpp_dec_float_elem_number - 1) * cpp_dec_float_elem_digits10);

      if (static_cast&lt;std::size_t&gt;(str.length() - pos) &gt; max_dec)
      {
         str = str.substr(static_cast&lt;std::size_t&gt;(0u),
                          static_cast&lt;std::size_t&gt;(pos_plus_one + max_dec));
      }

      // Now the input string has the standard cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; input form.
      // (See the comment above.)

      // Set all the data elements to 0.
      std::fill(data.begin(), data.end(), static_cast&lt;std::uint32_t&gt;(0u));

      // Extract the data.

      // First get the digits to the left of the decimal point...
      data[0u] = boost::lexical_cast&lt;std::uint32_t&gt;(str.substr(static_cast&lt;std::size_t&gt;(0u), pos));

      // ...then get the remaining digits to the right of the decimal point.
      const std::string::size_type i_end = ((str.length() - pos_plus_one) / static_cast&lt;std::string::size_type&gt;(cpp_dec_float_elem_digits10));

      for (std::string::size_type i = static_cast&lt;std::string::size_type&gt;(0u); i &lt; i_end; i++)
      {
         const std::string::const_iterator it = str.begin() + pos_plus_one + (i * static_cast&lt;std::string::size_type&gt;(cpp_dec_float_elem_digits10));

         data[i + 1u] = boost::lexical_cast&lt;std::uint32_t&gt;(std::string(it, it + static_cast&lt;std::string::size_type&gt;(cpp_dec_float_elem_digits10)));
      }

      // Check for overflow...
      if (exp &gt; cpp_dec_float_max_exp10)
      {
         const bool b_result_is_neg = neg;

         *this = inf();
         if (b_result_is_neg)
            negate();
      }

      // ...and check for underflow.
      if (exp &lt;= cpp_dec_float_min_exp10)
      {
         if (exp == cpp_dec_float_min_exp10)
         {
            // Check for identity with the minimum value.
            cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; test = *this;

            test.exp = static_cast&lt;exponent_type&gt;(0);

            if (test.isone())
            {
               *this = zero();
            }
         }
         else
         {
            *this = zero();
         }
      }

#ifndef BOOST_NO_EXCEPTIONS
   }
   catch (const bad_lexical_cast&amp;)
   {
      // Rethrow with better error message:
      std::string msg = &quot;Unable to parse the string \&quot;&quot;;
      msg += s;
      msg += &quot;\&quot; as a floating point value.&quot;;
      throw std::runtime_error(msg);
   }
#endif
   return true;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float(const double mantissa, const ExponentType exponent)
    : data(),
      exp(static_cast&lt;ExponentType&gt;(0)),
      neg(false),
      fpclass(cpp_dec_float_finite),
      prec_elem(cpp_dec_float_elem_number)
{
   // Create *this cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; from a given mantissa and exponent.
   // Note: This constructor does not maintain the full precision of double.

   const bool mantissa_is_iszero = (::fabs(mantissa) &lt; ((std::numeric_limits&lt;double&gt;::min)() * (1.0 + std::numeric_limits&lt;double&gt;::epsilon())));

   if (mantissa_is_iszero)
   {
      std::fill(data.begin(), data.end(), static_cast&lt;std::uint32_t&gt;(0u));
      return;
   }

   const bool b_neg = (mantissa &lt; 0.0);

   double       d = ((!b_neg) ? mantissa : -mantissa);
   exponent_type e = exponent;

   while (d &gt; 10.0)
   {
      d /= 10.0;
      ++e;
   }
   while (d &lt; 1.0)
   {
      d *= 10.0;
      --e;
   }

   std::int32_t shift = static_cast&lt;std::int32_t&gt;(e % static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_digits10));

   while (static_cast&lt;std::int32_t&gt;(shift-- % cpp_dec_float_elem_digits10) != static_cast&lt;std::int32_t&gt;(0))
   {
      d *= 10.0;
      --e;
   }

   exp = e;
   neg = b_neg;

   std::fill(data.begin(), data.end(), static_cast&lt;std::uint32_t&gt;(0u));

   constexpr std::int32_t digit_ratio = static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(std::numeric_limits&lt;double&gt;::digits10) / static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_digits10));
   constexpr std::int32_t digit_loops = static_cast&lt;std::int32_t&gt;(digit_ratio + static_cast&lt;std::int32_t&gt;(2));

   for (std::int32_t i = static_cast&lt;std::int32_t&gt;(0); i &lt; digit_loops; i++)
   {
      std::uint32_t n = static_cast&lt;std::uint32_t&gt;(static_cast&lt;std::uint64_t&gt;(d));
      data[i]           = static_cast&lt;std::uint32_t&gt;(n);
      d -= static_cast&lt;double&gt;(n);
      d *= static_cast&lt;double&gt;(cpp_dec_float_elem_mask);
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
template &lt;class Float&gt;
typename std::enable_if&lt;std::is_floating_point&lt;Float&gt;::value, cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp;&gt;::type cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::operator=(Float a)
{
   // Christopher Kormanyos's original code used a cast to boost::long_long_type here, but that fails
   // when long double has more digits than a boost::long_long_type.
   using std::floor;
   using std::frexp;
   using std::ldexp;

   if (a == 0)
      return *this = zero();

   if (a == 1)
      return *this = one();

   if ((boost::math::isinf)(a))
   {
      *this = inf();
      if (a &lt; 0)
         this-&gt;negate();
      return *this;
   }

   if ((boost::math::isnan)(a))
      return *this = nan();

   int         e;
   Float f, term;
   *this = zero();

   f = frexp(a, &amp;e);
   // See <a href="https://svn.boost.org/trac/boost/ticket/10924">https://svn.boost.org/trac/boost/ticket/10924</a> for an example of why this may go wrong:
   BOOST_ASSERT((boost::math::isfinite)(f));

   constexpr int shift = std::numeric_limits&lt;int&gt;::digits - 1;

   while (f)
   {
      // extract int sized bits from f:
      f = ldexp(f, shift);
      BOOST_ASSERT((boost::math::isfinite)(f));
      term = floor(f);
      e -= shift;
      *this *= pow2(shift);
      if (term &gt; 0)
         add_unsigned_long_long(static_cast&lt;unsigned&gt;(term));
      else
         sub_unsigned_long_long(static_cast&lt;unsigned&gt;(-term));
      f -= term;
   }

   if (e != 0)
      *this *= pow2(e);

   return *this;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_unsigned_long_long(const boost::ulong_long_type u)
{
   std::fill(data.begin(), data.end(), static_cast&lt;std::uint32_t&gt;(0u));

   exp       = static_cast&lt;exponent_type&gt;(0);
   neg       = false;
   fpclass   = cpp_dec_float_finite;
   prec_elem = cpp_dec_float_elem_number;

   if (u == 0)
   {
      return;
   }

   std::size_t i = static_cast&lt;std::size_t&gt;(0u);

   boost::ulong_long_type uu = u;

   std::uint32_t temp[(std::numeric_limits&lt;boost::ulong_long_type&gt;::digits10 / static_cast&lt;int&gt;(cpp_dec_float_elem_digits10)) + 3] = {static_cast&lt;std::uint32_t&gt;(0u)};

   while (uu != static_cast&lt;boost::ulong_long_type&gt;(0u))
   {
      temp[i] = static_cast&lt;std::uint32_t&gt;(uu % static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask));
      uu      = static_cast&lt;boost::ulong_long_type&gt;(uu / static_cast&lt;boost::ulong_long_type&gt;(cpp_dec_float_elem_mask));
      ++i;
   }

   if (i &gt; static_cast&lt;std::size_t&gt;(1u))
   {
      exp += static_cast&lt;exponent_type&gt;((i - 1u) * static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_digits10));
   }

   std::reverse(temp, temp + i);
   std::copy(temp, temp + (std::min)(i, static_cast&lt;std::size_t&gt;(cpp_dec_float_elem_number)), data.begin());
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
template &lt;typename InputIteratorTypeLeft, typename InputIteratorTypeRight&gt;
int cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::compare_ranges(InputIteratorTypeLeft  a,
                                                                     InputIteratorTypeRight b,
                                                                     const std::uint32_t    count)
{
   using local_iterator_left_type  = InputIteratorTypeLeft;
   using local_iterator_right_type = InputIteratorTypeRight;

   local_iterator_left_type  begin_a(a);
   local_iterator_left_type  end_a  (a + count);
   local_iterator_right_type begin_b(b);
   local_iterator_right_type end_b  (b + count);

   const auto mismatch_pair = std::mismatch(begin_a, end_a, begin_b);

   int n_return;

   if((mismatch_pair.first != end_a) || (mismatch_pair.second != end_b))
   {
      const typename std::iterator_traits&lt;InputIteratorTypeLeft&gt;::value_type  left  = *mismatch_pair.first;
      const typename std::iterator_traits&lt;InputIteratorTypeRight&gt;::value_type right = *mismatch_pair.second;

      n_return = ((left &gt; right) ?  1 : -1);
   }
   else
   {
      n_return = 0;
   }

   return n_return;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
std::uint32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_add_n(      std::uint32_t* r,
                                                                           const std::uint32_t* u,
                                                                           const std::uint32_t* v,
                                                                           const std::int32_t   count)
{
   // Addition algorithm
   std::uint_fast8_t carry = static_cast&lt;std::uint_fast8_t&gt;(0U);

   for(std::int32_t j = static_cast&lt;std::int32_t&gt;(count - static_cast&lt;std::int32_t&gt;(1)); j &gt;= static_cast&lt;std::int32_t&gt;(0); --j)
   {
      const std::uint32_t t = static_cast&lt;std::uint32_t&gt;(static_cast&lt;std::uint32_t&gt;(u[j] + v[j]) + carry);

      carry = ((t &gt;= static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask)) ? static_cast&lt;std::uint_fast8_t&gt;(1U)
                                                                   : static_cast&lt;std::uint_fast8_t&gt;(0U));

      r[j]  = static_cast&lt;std::uint32_t&gt;(t - ((carry != 0U) ? static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask)
                                                            : static_cast&lt;std::uint32_t&gt;(0U)));
   }

   return static_cast&lt;std::uint32_t&gt;(carry);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
std::uint32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_subtract_n(      std::uint32_t* r,
                                                                                const std::uint32_t* u,
                                                                                const std::uint32_t* v,
                                                                                const std::int32_t   count)
{
   // Subtraction algorithm
   std::int_fast8_t borrow = static_cast&lt;std::int_fast8_t&gt;(0);

   for(std::uint32_t j = static_cast&lt;std::uint32_t&gt;(count - static_cast&lt;std::int32_t&gt;(1)); static_cast&lt;std::int32_t&gt;(j) &gt;= static_cast&lt;std::int32_t&gt;(0); --j)
   {
      std::int32_t t = static_cast&lt;std::int32_t&gt;(  static_cast&lt;std::int32_t&gt;(u[j])
                                                 - static_cast&lt;std::int32_t&gt;(v[j])) - borrow;

      // Underflow? Borrow?
      if(t &lt; 0)
      {
         // Yes, underflow and borrow
         t     += static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_mask);
         borrow = static_cast&lt;int_fast8_t&gt;(1);
      }
      else
      {
         borrow = static_cast&lt;int_fast8_t&gt;(0);
      }

      r[j] = static_cast&lt;std::uint32_t&gt;(t);
   }

   return static_cast&lt;std::int32_t&gt;(borrow);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_multiply_n_by_n_to_2n(      std::uint32_t* r,
                                                                                  const std::uint32_t* a,
                                                                                  const std::uint32_t* b,
                                                                                  const std::uint32_t  count)
{
   using local_limb_type = std::uint32_t;

   using local_double_limb_type = std::uint64_t;

   using local_reverse_iterator_type = std::reverse_iterator&lt;local_limb_type*&gt;;

   local_reverse_iterator_type ir(r + (count * 2));

   local_double_limb_type carry = 0U;

   for(std::int32_t j = static_cast&lt;std::int32_t&gt;(count - 1); j &gt;= static_cast&lt;std::int32_t&gt;(1); --j)
   {
      local_double_limb_type sum = carry;

      for(std::int32_t i = static_cast&lt;std::int32_t&gt;(count - 1); i &gt;= j; --i)
      {
         sum += local_double_limb_type(
                local_double_limb_type(a[i]) * b[  static_cast&lt;std::int32_t&gt;(count - 1)
                                                - static_cast&lt;std::int32_t&gt;(i - j)]);
      }

      carry = static_cast&lt;local_double_limb_type&gt;(sum / static_cast&lt;local_limb_type&gt;       (cpp_dec_float_elem_mask));
      *ir++ = static_cast&lt;local_limb_type&gt;       (sum - static_cast&lt;local_double_limb_type&gt;(static_cast&lt;local_double_limb_type&gt;(carry) * static_cast&lt;local_limb_type&gt;(cpp_dec_float_elem_mask)));
   }

   for(std::int32_t j = static_cast&lt;std::int32_t&gt;(count - 1); j &gt;= static_cast&lt;std::int32_t&gt;(0); --j)
   {
      local_double_limb_type sum = carry;

      for(std::int32_t i = j; i &gt;= static_cast&lt;std::int32_t&gt;(0); --i)
      {
         sum += static_cast&lt;local_double_limb_type&gt;(a[j - i] * static_cast&lt;local_double_limb_type&gt;(b[i]));
      }

      carry = static_cast&lt;local_double_limb_type&gt;(sum / static_cast&lt;local_limb_type&gt;(cpp_dec_float_elem_mask));
      *ir++ = static_cast&lt;local_limb_type&gt;       (sum - static_cast&lt;local_double_limb_type&gt;(static_cast&lt;local_double_limb_type&gt;(carry) * static_cast&lt;local_limb_type&gt;(cpp_dec_float_elem_mask)));
   }

   *ir = static_cast&lt;local_limb_type&gt;(carry);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
std::uint32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::mul_loop_n(std::uint32_t* const u, std::uint32_t n, const std::int32_t p)
{
   std::uint64_t carry = static_cast&lt;std::uint64_t&gt;(0u);

   // Multiplication loop.
   for (std::int32_t j = p - 1; j &gt;= static_cast&lt;std::int32_t&gt;(0); j--)
   {
      const std::uint64_t t = static_cast&lt;std::uint64_t&gt;(carry + static_cast&lt;std::uint64_t&gt;(u[j] * static_cast&lt;std::uint64_t&gt;(n)));
      carry                 = static_cast&lt;std::uint64_t&gt;(t / static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask));
      u[j]                  = static_cast&lt;std::uint32_t&gt;(t - static_cast&lt;std::uint64_t&gt;(static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask) * static_cast&lt;std::uint64_t&gt;(carry)));
   }

   return static_cast&lt;std::uint32_t&gt;(carry);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
std::uint32_t cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::div_loop_n(std::uint32_t* const u, std::uint32_t n, const std::int32_t p)
{
   std::uint64_t prev = static_cast&lt;std::uint64_t&gt;(0u);

   for (std::int32_t j = static_cast&lt;std::int32_t&gt;(0); j &lt; p; j++)
   {
      const std::uint64_t t = static_cast&lt;std::uint64_t&gt;(u[j] + static_cast&lt;std::uint64_t&gt;(prev * static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask)));
      u[j]                    = static_cast&lt;std::uint32_t&gt;(t / n);
      prev                    = static_cast&lt;std::uint64_t&gt;(t - static_cast&lt;std::uint64_t&gt;(n * static_cast&lt;std::uint64_t&gt;(u[j])));
   }

   return static_cast&lt;std::uint32_t&gt;(prev);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_multiply_kara_propagate_carry(std::uint32_t* t, const std::uint32_t n, const std::uint32_t carry)
{
   std::uint_fast8_t carry_out = ((carry != 0U) ? static_cast&lt;std::uint_fast8_t&gt;(1U)
                                                : static_cast&lt;std::uint_fast8_t&gt;(0U));

   using local_reverse_iterator_type = std::reverse_iterator&lt;std::uint32_t*&gt;;

   local_reverse_iterator_type ri_t  (t + n);
   local_reverse_iterator_type rend_t(t);

   while((carry_out != 0U) &amp;&amp; (ri_t != rend_t))
   {
      const std::uint64_t tt = *ri_t + carry_out;

      carry_out = ((tt &gt;= static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask)) ? static_cast&lt;std::uint_fast8_t&gt;(1U)
                                                                               : static_cast&lt;std::uint_fast8_t&gt;(0U));

      *ri_t++    = static_cast&lt;std::uint32_t&gt;(tt - ((carry_out != 0U) ? static_cast&lt;std::uint32_t&gt;(cpp_dec_float_elem_mask)
                                                                      : static_cast&lt;std::uint32_t&gt;(0U)));
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_multiply_kara_propagate_borrow(std::uint32_t* t, const std::uint32_t n, const bool has_borrow)
{
   std::int_fast8_t borrow = (has_borrow ? static_cast&lt;std::int_fast8_t&gt;(1)
                                         : static_cast&lt;std::int_fast8_t&gt;(0));

   using local_reverse_iterator_type = std::reverse_iterator&lt;std::uint32_t*&gt;;

   local_reverse_iterator_type ri_t  (t + n);
   local_reverse_iterator_type rend_t(t);

   while((borrow != 0U) &amp;&amp; (ri_t != rend_t))
   {
      std::int32_t tt = static_cast&lt;std::int32_t&gt;(static_cast&lt;std::int32_t&gt;(*ri_t) - borrow);

      // Underflow? Borrow?
      if(tt &lt; 0)
      {
         // Yes, underflow and borrow
         tt     += static_cast&lt;std::int32_t&gt;(cpp_dec_float_elem_mask);
         borrow  = static_cast&lt;int_fast8_t&gt;(1);
      }
      else
      {
         borrow = static_cast&lt;int_fast8_t&gt;(0);
      }

      *ri_t++ = static_cast&lt;std::uint32_t&gt;(tt);
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
void cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eval_multiply_kara_n_by_n_to_2n(      std::uint32_t* r,
                                                                                       const std::uint32_t* a,
                                                                                       const std::uint32_t* b,
                                                                                       const std::uint32_t  n,
                                                                                             std::uint32_t* t)
{
   if(n &lt;= 32U)
   {
      static_cast&lt;void&gt;(t);

      eval_multiply_n_by_n_to_2n(r, a, b, n);
   }
   else
   {
      // Based on &quot;Algorithm 1.3 KaratsubaMultiply&quot;, Sect. 1.3.2, page 5
      // of R.P. Brent and P. Zimmermann, &quot;Modern Computer Arithmetic&quot;,
      // Cambridge University Press (2011).

      // The Karatsuba multipliation computes the product of a*b as:
      // [b^N + b^(N/2)] a1*b1 + [b^(N/2)](a1 - a0)(b0 - b1) + [b^(N/2) + 1] a0*b0

      // Here we visualize a and b in two components 1,0 corresponding
      // to the high and low order parts, respectively.

      // Step 1
      // Calculate a1*b1 and store it in the upper-order part of r.
      // Calculate a0*b0 and store it in the lower-order part of r.
      // copy r to t0.

      // Step 2
      // Add a1*b1 (which is t2) to the middle two-quarters of r (which is r1)
      // Add a0*b0 (which is t0) to the middle two-quarters of r (which is r1)

      // Step 3
      // Calculate |a1-a0| in t0 and note the sign (i.e., the borrow flag)

      // Step 4
      // Calculate |b0-b1| in t1 and note the sign (i.e., the borrow flag)

      // Step 5
      // Call kara mul to calculate |a1-a0|*|b0-b1| in (t2),
      // while using temporary storage in t4 along the way.

      // Step 6
      // Check the borrow signs. If a1-a0 and b0-b1 have the same signs,
      // then add |a1-a0|*|b0-b1| to r1, otherwise subtract it from r1.

      const std::uint_fast32_t  nh = n / 2U;

      const std::uint32_t* a0 = a + nh;
      const std::uint32_t* a1 = a + 0U;

      const std::uint32_t* b0 = b + nh;
      const std::uint32_t* b1 = b + 0U;

            std::uint32_t* r0 = r + 0U;
            std::uint32_t* r1 = r + nh;
            std::uint32_t* r2 = r + n;

            std::uint32_t* t0 = t + 0U;
            std::uint32_t* t1 = t + nh;
            std::uint32_t* t2 = t + n;
            std::uint32_t* t4 = t + (n + n);

      // Step 1
      eval_multiply_kara_n_by_n_to_2n(r0, a1, b1, nh, t);
      eval_multiply_kara_n_by_n_to_2n(r2, a0, b0, nh, t);
      std::copy(r0, r0 + (2U * n), t0);

      // Step 2
      std::uint32_t carry;
      carry = eval_add_n(r1, r1, t0, n);
      eval_multiply_kara_propagate_carry(r0, nh, carry);
      carry = eval_add_n(r1, r1, t2, n);
      eval_multiply_kara_propagate_carry(r0, nh, carry);

      // Step 3
      const int cmp_result_a1a0 = compare_ranges(a1, a0, nh);

      if(cmp_result_a1a0 == 1)
         static_cast&lt;void&gt;(eval_subtract_n(t0, a1, a0, nh));
      else if(cmp_result_a1a0 == -1)
         static_cast&lt;void&gt;(eval_subtract_n(t0, a0, a1, nh));

      // Step 4
      const int cmp_result_b0b1 = compare_ranges(b0, b1, nh);

      if(cmp_result_b0b1 == 1)
         static_cast&lt;void&gt;(eval_subtract_n(t1, b0, b1, nh));
      else if(cmp_result_b0b1 == -1)
         static_cast&lt;void&gt;(eval_subtract_n(t1, b1, b0, nh));

      // Step 5
      eval_multiply_kara_n_by_n_to_2n(t2, t0, t1, nh, t4);

      // Step 6
      if((cmp_result_a1a0 * cmp_result_b0b1) == 1)
      {
         carry = eval_add_n(r1, r1, t2, n);

         eval_multiply_kara_propagate_carry(r0, nh, carry);
      }
      else if((cmp_result_a1a0 * cmp_result_b0b1) == -1)
      {
         const bool has_borrow = eval_subtract_n(r1, r1, t2, n);

         eval_multiply_kara_propagate_borrow(r0, nh, has_borrow);
      }
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::pow2(const boost::long_long_type p)
{
   static const std::array&lt;cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, 256u&gt; local_pow2_data =
   {{
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       29u, 38735877u,  5571876u, 99218413u, 43055614u, 19454666u, 38919302u, 18803771u, 87926569u, 60431486u, 36817932u, 12890625u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       58u, 77471754u, 11143753u, 98436826u, 86111228u, 38909332u, 77838604u, 37607543u, 75853139u, 20862972u, 73635864u, 25781250u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      117u, 54943508u, 22287507u, 96873653u, 72222456u, 77818665u, 55677208u, 75215087u, 51706278u, 41725945u, 47271728u, 51562500u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      235u,  9887016u, 44575015u, 93747307u, 44444913u, 55637331u, 11354417u, 50430175u,  3412556u, 83451890u, 94543457u,  3125000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      470u, 19774032u, 89150031u, 87494614u, 88889827u, 11274662u, 22708835u,   860350u,  6825113u, 66903781u, 89086914u,  6250000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      940u, 39548065u, 78300063u, 74989229u, 77779654u, 22549324u, 45417670u,  1720700u, 13650227u, 33807563u, 78173828u, 12500000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1880u, 79096131u, 56600127u, 49978459u, 55559308u, 45098648u, 90835340u,  3441400u, 27300454u, 67615127u, 56347656u, 25000000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     3761u, 58192263u, 13200254u, 99956919u, 11118616u, 90197297u, 81670680u,  6882800u, 54600909u, 35230255u, 12695312u, 50000000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     7523u, 16384526u, 26400509u, 99913838u, 22237233u, 80394595u, 63341360u, 13765601u,  9201818u, 70460510u, 25390625u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    15046u, 32769052u, 52801019u, 99827676u, 44474467u, 60789191u, 26682720u, 27531202u, 18403637u, 40921020u, 50781250u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    30092u, 65538105u,  5602039u, 99655352u, 88948935u, 21578382u, 53365440u, 55062404u, 36807274u, 81842041u,  1562500u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    60185u, 31076210u, 11204079u, 99310705u, 77897870u, 43156765u,  6730881u, 10124808u, 73614549u, 63684082u,  3125000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   120370u, 62152420u, 22408159u, 98621411u, 55795740u, 86313530u, 13461762u, 20249617u, 47229099u, 27368164u,  6250000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   240741u, 24304840u, 44816319u, 97242823u, 11591481u, 72627060u, 26923524u, 40499234u, 94458198u, 54736328u, 12500000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   481482u, 48609680u, 89632639u, 94485646u, 23182963u, 45254120u, 53847048u, 80998469u, 88916397u,  9472656u, 25000000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   962964u, 97219361u, 79265279u, 88971292u, 46365926u, 90508241u,  7694097u, 61996939u, 77832794u, 18945312u, 50000000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1925929u, 94438723u, 58530559u, 77942584u, 92731853u, 81016482u, 15388195u, 23993879u, 55665588u, 37890625u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  3851859u, 88877447u, 17061119u, 55885169u, 85463707u, 62032964u, 30776390u, 47987759u, 11331176u, 75781250u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  7703719u, 77754894u, 34122239u, 11770339u, 70927415u, 24065928u, 61552780u, 95975518u, 22662353u, 51562500u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 15407439u, 55509788u, 68244478u, 23540679u, 41854830u, 48131857u, 23105561u, 91951036u, 45324707u,  3125000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 30814879u, 11019577u, 36488956u, 47081358u, 83709660u, 96263714u, 46211123u, 83902072u, 90649414u,  6250000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 61629758u, 22039154u, 72977912u, 94162717u, 67419321u, 92527428u, 92422247u, 67804145u, 81298828u, 12500000u }, -40 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 23259516u, 44078309u, 45955825u, 88325435u, 34838643u, 85054857u, 84844495u, 35608291u, 62597656u, 25000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u, 46519032u, 88156618u, 91911651u, 76650870u, 69677287u, 70109715u, 69688990u, 71216583u, 25195312u, 50000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        4u, 93038065u, 76313237u, 83823303u, 53301741u, 39354575u, 40219431u, 39377981u, 42433166u, 50390625u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        9u, 86076131u, 52626475u, 67646607u,  6603482u, 78709150u, 80438862u, 78755962u, 84866333u,   781250u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       19u, 72152263u,  5252951u, 35293214u, 13206965u, 57418301u, 60877725u, 57511925u, 69732666u,  1562500u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       39u, 44304526u, 10505902u, 70586428u, 26413931u, 14836603u, 21755451u, 15023851u, 39465332u,  3125000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       78u, 88609052u, 21011805u, 41172856u, 52827862u, 29673206u, 43510902u, 30047702u, 78930664u,  6250000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      157u, 77218104u, 42023610u, 82345713u,  5655724u, 59346412u, 87021804u, 60095405u, 57861328u, 12500000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      315u, 54436208u, 84047221u, 64691426u, 11311449u, 18692825u, 74043609u, 20190811u, 15722656u, 25000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      631u,  8872417u, 68094443u, 29382852u, 22622898u, 37385651u, 48087218u, 40381622u, 31445312u, 50000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1262u, 17744835u, 36188886u, 58765704u, 45245796u, 74771302u, 96174436u, 80763244u, 62890625u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     2524u, 35489670u, 72377773u, 17531408u, 90491593u, 49542605u, 92348873u, 61526489u, 25781250u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     5048u, 70979341u, 44755546u, 35062817u, 80983186u, 99085211u, 84697747u, 23052978u, 51562500u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    10097u, 41958682u, 89511092u, 70125635u, 61966373u, 98170423u, 69395494u, 46105957u,  3125000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    20194u, 83917365u, 79022185u, 40251271u, 23932747u, 96340847u, 38790988u, 92211914u,  6250000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    40389u, 67834731u, 58044370u, 80502542u, 47865495u, 92681694u, 77581977u, 84423828u, 12500000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    80779u, 35669463u, 16088741u, 61005084u, 95730991u, 85363389u, 55163955u, 68847656u, 25000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   161558u, 71338926u, 32177483u, 22010169u, 91461983u, 70726779u, 10327911u, 37695312u, 50000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   323117u, 42677852u, 64354966u, 44020339u, 82923967u, 41453558u, 20655822u, 75390625u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   646234u, 85355705u, 28709932u, 88040679u, 65847934u, 82907116u, 41311645u, 50781250u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1292469u, 70711410u, 57419865u, 76081359u, 31695869u, 65814232u, 82623291u,  1562500u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  2584939u, 41422821u, 14839731u, 52162718u, 63391739u, 31628465u, 65246582u,  3125000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  5169878u, 82845642u, 29679463u,  4325437u, 26783478u, 63256931u, 30493164u,  6250000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 10339757u, 65691284u, 59358926u,  8650874u, 53566957u, 26513862u, 60986328u, 12500000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 20679515u, 31382569u, 18717852u, 17301749u,  7133914u, 53027725u, 21972656u, 25000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 41359030u, 62765138u, 37435704u, 34603498u, 14267829u,  6055450u, 43945312u, 50000000u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 82718061u, 25530276u, 74871408u, 69206996u, 28535658u, 12110900u, 87890625u }, -32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 65436122u, 51060553u, 49742817u, 38413992u, 57071316u, 24221801u, 75781250u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        3u, 30872245u,  2121106u, 99485634u, 76827985u, 14142632u, 48443603u, 51562500u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        6u, 61744490u,  4242213u, 98971269u, 53655970u, 28285264u, 96887207u,  3125000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       13u, 23488980u,  8484427u, 97942539u,  7311940u, 56570529u, 93774414u,  6250000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       26u, 46977960u, 16968855u, 95885078u, 14623881u, 13141059u, 87548828u, 12500000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       52u, 93955920u, 33937711u, 91770156u, 29247762u, 26282119u, 75097656u, 25000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      105u, 87911840u, 67875423u, 83540312u, 58495524u, 52564239u, 50195312u, 50000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      211u, 75823681u, 35750847u, 67080625u, 16991049u,  5128479u,   390625u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      423u, 51647362u, 71501695u, 34161250u, 33982098u, 10256958u,   781250u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      847u,  3294725u, 43003390u, 68322500u, 67964196u, 20513916u,  1562500u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1694u,  6589450u, 86006781u, 36645001u, 35928392u, 41027832u,  3125000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     3388u, 13178901u, 72013562u, 73290002u, 71856784u, 82055664u,  6250000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     6776u, 26357803u, 44027125u, 46580005u, 43713569u, 64111328u, 12500000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    13552u, 52715606u, 88054250u, 93160010u, 87427139u, 28222656u, 25000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    27105u,  5431213u, 76108501u, 86320021u, 74854278u, 56445312u, 50000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    54210u, 10862427u, 52217003u, 72640043u, 49708557u, 12890625u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   108420u, 21724855u,  4434007u, 45280086u, 99417114u, 25781250u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   216840u, 43449710u,  8868014u, 90560173u, 98834228u, 51562500u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   433680u, 86899420u, 17736029u, 81120347u, 97668457u,  3125000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   867361u, 73798840u, 35472059u, 62240695u, 95336914u,  6250000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1734723u, 47597680u, 70944119u, 24481391u, 90673828u, 12500000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  3469446u, 95195361u, 41888238u, 48962783u, 81347656u, 25000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  6938893u, 90390722u, 83776476u, 97925567u, 62695312u, 50000000u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 13877787u, 80781445u, 67552953u, 95851135u, 25390625u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 27755575u, 61562891u, 35105907u, 91702270u, 50781250u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 55511151u, 23125782u, 70211815u, 83404541u,  1562500u }, -24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 11022302u, 46251565u, 40423631u, 66809082u,  3125000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u, 22044604u, 92503130u, 80847263u, 33618164u,  6250000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        4u, 44089209u, 85006261u, 61694526u, 67236328u, 12500000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        8u, 88178419u, 70012523u, 23389053u, 34472656u, 25000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       17u, 76356839u, 40025046u, 46778106u, 68945312u, 50000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       35u, 52713678u, 80050092u, 93556213u, 37890625u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       71u,  5427357u, 60100185u, 87112426u, 75781250u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      142u, 10854715u, 20200371u, 74224853u, 51562500u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      284u, 21709430u, 40400743u, 48449707u,  3125000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      568u, 43418860u, 80801486u, 96899414u,  6250000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1136u, 86837721u, 61602973u, 93798828u, 12500000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     2273u, 73675443u, 23205947u, 87597656u, 25000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     4547u, 47350886u, 46411895u, 75195312u, 50000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     9094u, 94701772u, 92823791u, 50390625u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    18189u, 89403545u, 85647583u,   781250u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    36379u, 78807091u, 71295166u,  1562500u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    72759u, 57614183u, 42590332u,  3125000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   145519u, 15228366u, 85180664u,  6250000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   291038u, 30456733u, 70361328u, 12500000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   582076u, 60913467u, 40722656u, 25000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1164153u, 21826934u, 81445312u, 50000000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  2328306u, 43653869u, 62890625u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  4656612u, 87307739u, 25781250u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  9313225u, 74615478u, 51562500u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 18626451u, 49230957u,  3125000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 37252902u, 98461914u,  6250000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 74505805u, 96923828u, 12500000u }, -16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 49011611u, 93847656u, 25000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u, 98023223u, 87695312u, 50000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        5u, 96046447u, 75390625u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       11u, 92092895u, 50781250u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       23u, 84185791u,  1562500u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       47u, 68371582u,  3125000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       95u, 36743164u,  6250000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      190u, 73486328u, 12500000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      381u, 46972656u, 25000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      762u, 93945312u, 50000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1525u, 87890625u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     3051u, 75781250u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     6103u, 51562500u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    12207u,  3125000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    24414u,  6250000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    48828u, 12500000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    97656u, 25000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   195312u, 50000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   390625u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   781250u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1562500u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  3125000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  6250000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 12500000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 25000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 50000000u }, -8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        4u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        8u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       16u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       32u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       64u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      128u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      256u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      512u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1024u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     2048u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     4096u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     8192u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    16384u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    32768u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    65536u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   131072u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   262144u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   524288u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1048576u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  2097152u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  4194304u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  8388608u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 16777216u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 33554432u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 67108864u },  0 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 34217728u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u, 68435456u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        5u, 36870912u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       10u, 73741824u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       21u, 47483648u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       42u, 94967296u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       85u, 89934592u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      171u, 79869184u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      343u, 59738368u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      687u, 19476736u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1374u, 38953472u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     2748u, 77906944u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     5497u, 55813888u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    10995u, 11627776u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    21990u, 23255552u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    43980u, 46511104u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    87960u, 93022208u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   175921u, 86044416u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   351843u, 72088832u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   703687u, 44177664u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1407374u, 88355328u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  2814749u, 76710656u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  5629499u, 53421312u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 11258999u,  6842624u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 22517998u, 13685248u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 45035996u, 27370496u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 90071992u, 54740992u },  8 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 80143985u,  9481984u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        3u, 60287970u, 18963968u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        7u, 20575940u, 37927936u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       14u, 41151880u, 75855872u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       28u, 82303761u, 51711744u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       57u, 64607523u,  3423488u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      115u, 29215046u,  6846976u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      230u, 58430092u, 13693952u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      461u, 16860184u, 27387904u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      922u, 33720368u, 54775808u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1844u, 67440737u,  9551616u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     3689u, 34881474u, 19103232u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     7378u, 69762948u, 38206464u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    14757u, 39525896u, 76412928u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    29514u, 79051793u, 52825856u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    59029u, 58103587u,  5651712u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   118059u, 16207174u, 11303424u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   236118u, 32414348u, 22606848u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   472236u, 64828696u, 45213696u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   944473u, 29657392u, 90427392u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1888946u, 59314785u, 80854784u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  3777893u, 18629571u, 61709568u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  7555786u, 37259143u, 23419136u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 15111572u, 74518286u, 46838272u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 30223145u, 49036572u, 93676544u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 60446290u, 98073145u, 87353088u }, 16 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 20892581u, 96146291u, 74706176u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        2u, 41785163u, 92292583u, 49412352u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        4u, 83570327u, 84585166u, 98824704u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        9u, 67140655u, 69170333u, 97649408u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       19u, 34281311u, 38340667u, 95298816u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       38u, 68562622u, 76681335u, 90597632u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       77u, 37125245u, 53362671u, 81195264u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      154u, 74250491u,  6725343u, 62390528u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      309u, 48500982u, 13450687u, 24781056u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      618u, 97001964u, 26901374u, 49562112u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1237u, 94003928u, 53802748u, 99124224u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     2475u, 88007857u,  7605497u, 98248448u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     4951u, 76015714u, 15210995u, 96496896u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     9903u, 52031428u, 30421991u, 92993792u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    19807u,  4062856u, 60843983u, 85987584u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    39614u,  8125713u, 21687967u, 71975168u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    79228u, 16251426u, 43375935u, 43950336u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   158456u, 32502852u, 86751870u, 87900672u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   316912u, 65005705u, 73503741u, 75801344u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   633825u, 30011411u, 47007483u, 51602688u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1267650u, 60022822u, 94014967u,  3205376u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  2535301u, 20045645u, 88029934u,  6410752u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  5070602u, 40091291u, 76059868u, 12821504u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 10141204u, 80182583u, 52119736u, 25643008u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 20282409u, 60365167u,  4239472u, 51286016u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 40564819u, 20730334u,  8478945u,  2572032u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( { 81129638u, 41460668u, 16957890u,  5144064u }, 24 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        1u, 62259276u, 82921336u, 33915780u, 10288128u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        3u, 24518553u, 65842672u, 67831560u, 20576256u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {        6u, 49037107u, 31685345u, 35663120u, 41152512u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       12u, 98074214u, 63370690u, 71326240u, 82305024u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       25u, 96148429u, 26741381u, 42652481u, 64610048u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {       51u, 92296858u, 53482762u, 85304963u, 29220096u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      103u, 84593717u,  6965525u, 70609926u, 58440192u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      207u, 69187434u, 13931051u, 41219853u, 16880384u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      415u, 38374868u, 27862102u, 82439706u, 33760768u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {      830u, 76749736u, 55724205u, 64879412u, 67521536u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     1661u, 53499473u, 11448411u, 29758825u, 35043072u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     3323u,  6998946u, 22896822u, 59517650u, 70086144u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {     6646u, 13997892u, 45793645u, 19035301u, 40172288u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    13292u, 27995784u, 91587290u, 38070602u, 80344576u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    26584u, 55991569u, 83174580u, 76141205u, 60689152u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {    53169u, 11983139u, 66349161u, 52282411u, 21378304u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   106338u, 23966279u, 32698323u,  4564822u, 42756608u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   212676u, 47932558u, 65396646u,  9129644u, 85513216u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   425352u, 95865117u, 30793292u, 18259289u, 71026432u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {   850705u, 91730234u, 61586584u, 36518579u, 42052864u }, 32 ),
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::from_lst( {  1701411u, 83460469u, 23173168u, 73037158u, 84105728u }, 32 ),
   }};

   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; t;

   if(p &lt; -128L)
      default_ops::detail::pow_imp(t, cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::half(), boost::ulong_long_type(-p), std::integral_constant&lt;bool, false&gt;());
   else if ((p &gt;= -128L) &amp;&amp; (p &lt;= 127L))
      t = local_pow2_data[std::size_t(p + 128)];
   else
      default_ops::detail::pow_imp(t, cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::two(), boost::ulong_long_type(p), std::integral_constant&lt;bool, false&gt;());

   return t;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_add(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; o)
{
   result += o;
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_subtract(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; o)
{
   result -= o;
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_multiply(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; o)
{
   result *= o;
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_divide(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; o)
{
   result /= o;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_add(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const boost::ulong_long_type&amp; o)
{
   result.add_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_subtract(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const boost::ulong_long_type&amp; o)
{
   result.sub_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_multiply(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const boost::ulong_long_type&amp; o)
{
   result.mul_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_divide(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const boost::ulong_long_type&amp; o)
{
   result.div_unsigned_long_long(o);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_add(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, boost::long_long_type o)
{
   if (o &lt; 0)
      result.sub_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(o));
   else
      result.add_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_subtract(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, boost::long_long_type o)
{
   if (o &lt; 0)
      result.add_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(o));
   else
      result.sub_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_multiply(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, boost::long_long_type o)
{
   if (o &lt; 0)
   {
      result.mul_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(o));
      result.negate();
   }
   else
      result.mul_unsigned_long_long(o);
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_divide(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, boost::long_long_type o)
{
   if (o &lt; 0)
   {
      result.div_unsigned_long_long(boost::multiprecision::detail::unsigned_abs(o));
      result.negate();
   }
   else
      result.div_unsigned_long_long(o);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_convert_to(boost::ulong_long_type* result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   *result = val.extract_unsigned_long_long();
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_convert_to(boost::long_long_type* result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   *result = val.extract_signed_long_long();
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_convert_to(long double* result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   *result = val.extract_long_double();
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_convert_to(double* result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   *result = val.extract_double();
}

//
// Non member function support:
//
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline int eval_fpclassify(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   if ((x.isinf)())
      return FP_INFINITE;
   if ((x.isnan)())
      return FP_NAN;
   if (x.iszero())
      return FP_ZERO;
   return FP_NORMAL;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_abs(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   result = x;
   if (x.isneg())
      result.negate();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_fabs(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   result = x;
   if (x.isneg())
      result.negate();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_sqrt(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   result = x;
   result.calculate_sqrt();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_floor(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   result = x;
   if (!(x.isfinite)() || x.isint())
   {
      if ((x.isnan)())
         errno = EDOM;
      return;
   }

   if (x.isneg())
      result -= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::one();
   result = result.extract_integer_part();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_ceil(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   result = x;
   if (!(x.isfinite)() || x.isint())
   {
      if ((x.isnan)())
         errno = EDOM;
      return;
   }

   if (!x.isneg())
      result += cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::one();
   result = result.extract_integer_part();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_trunc(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x)
{
   if (x.isint() || !(x.isfinite)())
   {
      result = x;
      if ((x.isnan)())
         errno = EDOM;
      return;
   }
   result = x.extract_integer_part();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline ExponentType eval_ilogb(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   if (val.iszero())
      return (std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::min)();
   if ((val.isinf)())
      return INT_MAX;
   if ((val.isnan)())
#ifdef FP_ILOGBNAN
      return FP_ILOGBNAN;
#else
      return INT_MAX;
#endif
   // Set result, to the exponent of val:
   return val.order();
}
template &lt;unsigned Digits10, class ExponentType, class Allocator, class ArgType&gt;
inline void eval_scalbn(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val, ArgType e_)
{
   using default_ops::eval_multiply;
   const typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type                               e = static_cast&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;(e_);
   cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; t(1.0, e);
   eval_multiply(result, val, t);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator, class ArgType&gt;
inline void eval_ldexp(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x, ArgType e)
{
   const boost::long_long_type the_exp = static_cast&lt;boost::long_long_type&gt;(e);

   if ((the_exp &gt; (std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::max)()) || (the_exp &lt; (std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::min)()))
      BOOST_THROW_EXCEPTION(std::runtime_error(std::string(&quot;Exponent value is out of range.&quot;)));

   result = x;

   if ((the_exp &gt; static_cast&lt;boost::long_long_type&gt;(-std::numeric_limits&lt;boost::long_long_type&gt;::digits)) &amp;&amp; (the_exp &lt; static_cast&lt;boost::long_long_type&gt;(0)))
      result.div_unsigned_long_long(1ULL &lt;&lt; static_cast&lt;boost::long_long_type&gt;(-the_exp));
   else if ((the_exp &lt; static_cast&lt;boost::long_long_type&gt;(std::numeric_limits&lt;boost::long_long_type&gt;::digits)) &amp;&amp; (the_exp &gt; static_cast&lt;boost::long_long_type&gt;(0)))
      result.mul_unsigned_long_long(1ULL &lt;&lt; the_exp);
   else if (the_exp != static_cast&lt;boost::long_long_type&gt;(0))
   {
      if ((the_exp &lt; cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp / 2) &amp;&amp; (x.order() &gt; 0))
      {
         boost::long_long_type half_exp = e / 2;
         cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; t = cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::pow2(half_exp);
         result *= t;
         if (2 * half_exp != e)
            t *= 2;
         result *= t;
      }
      else
         result *= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::pow2(e);
   }
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline void eval_frexp(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x, ExponentType* e)
{
   result = x;

   if (result.iszero() || (result.isinf)() || (result.isnan)())
   {
      *e = 0;
      return;
   }

   if (result.isneg())
      result.negate();

   typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type t = result.order();
   BOOST_MP_USING_ABS
   if (abs(t) &lt; ((std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::max)() / 1000))
   {
      t *= 1000;
      t /= 301;
   }
   else
   {
      t /= 301;
      t *= 1000;
   }

   result *= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::pow2(-t);

   if (result.iszero() || (result.isinf)() || (result.isnan)())
   {
      // pow2 overflowed, slip the calculation up:
      result = x;
      if (result.isneg())
         result.negate();
      t /= 2;
      result *= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::pow2(-t);
   }
   BOOST_MP_USING_ABS
   if (abs(result.order()) &gt; 5)
   {
      // If our first estimate doesn't get close enough then try recursion until we do:
      typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type e2;
      cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;                         r2;
      eval_frexp(r2, result, &amp;e2);
      // overflow protection:
      if ((t &gt; 0) &amp;&amp; (e2 &gt; 0) &amp;&amp; (t &gt; (std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::max)() - e2))
         BOOST_THROW_EXCEPTION(std::runtime_error(&quot;Exponent is too large to be represented as a power of 2.&quot;));
      if ((t &lt; 0) &amp;&amp; (e2 &lt; 0) &amp;&amp; (t &lt; (std::numeric_limits&lt;typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type&gt;::min)() - e2))
         BOOST_THROW_EXCEPTION(std::runtime_error(&quot;Exponent is too large to be represented as a power of 2.&quot;));
      t += e2;
      result = r2;
   }

   while (result.compare(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::one()) &gt;= 0)
   {
      result /= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::two();
      ++t;
   }
   while (result.compare(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::half()) &lt; 0)
   {
      result *= cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::two();
      --t;
   }
   *e = t;
   if (x.isneg())
      result.negate();
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline typename std::enable_if&lt; !std::is_same&lt;ExponentType, int&gt;::value&gt;::type eval_frexp(cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; result, const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; x, int* e)
{
   typename cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type t;
   eval_frexp(result, x, &amp;t);
   if ((t &gt; (std::numeric_limits&lt;int&gt;::max)()) || (t &lt; (std::numeric_limits&lt;int&gt;::min)()))
      BOOST_THROW_EXCEPTION(std::runtime_error(&quot;Exponent is outside the range of an int&quot;));
   *e = static_cast&lt;int&gt;(t);
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline bool eval_is_zero(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   return val.iszero();
}
template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline int eval_get_sign(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   return val.iszero() ? 0 : val.isneg() ? -1 : 1;
}

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
inline std::size_t hash_value(const cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;&amp; val)
{
   return val.hash();
}

} // namespace backends

using boost::multiprecision::backends::cpp_dec_float;

using cpp_dec_float_50 = number&lt;cpp_dec_float&lt;50&gt; &gt; ;
using cpp_dec_float_100 = number&lt;cpp_dec_float&lt;100&gt; &gt;;

namespace detail {

template &lt;unsigned Digits10, class ExponentType, class Allocator&gt;
struct transcendental_reduction_type&lt;boost::multiprecision::backends::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt; &gt;
{
   //
   // The type used for trigonometric reduction needs 3 times the precision of the base type.
   // This is double the precision of the original type, plus the largest exponent supported.
   // As a practical measure the largest argument supported is 1/eps, as supporting larger
   // arguments requires the division of argument by PI/2 to also be done at higher precision,
   // otherwise the result (an integer) can not be represented exactly.
   // 
   // See ARGUMENT REDUCTION FOR HUGE ARGUMENTS. K C Ng.
   //
   using type = boost::multiprecision::backends::cpp_dec_float&lt;Digits10 * 3, ExponentType, Allocator&gt;;
};

} // namespace detail


}} // namespace boost::multiprecision

namespace std {
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
class numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;
{
 public:
   static constexpr bool                    is_specialized    = true;
   static constexpr bool                    is_signed         = true;
   static constexpr bool                    is_integer        = false;
   static constexpr bool                    is_exact          = false;
   static constexpr bool                    is_bounded        = true;
   static constexpr bool                    is_modulo         = false;
   static constexpr bool                    is_iec559         = false;
   static constexpr int                     digits            = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10;
   static constexpr int                     digits10          = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10;
   static constexpr int                     max_digits10      = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_digits10;
   static constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type min_exponent                = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp;   // Type differs from int.
   static constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type min_exponent10              = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_min_exp10; // Type differs from int.
   static constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type max_exponent                = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_exp;   // Type differs from int.
   static constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type max_exponent10              = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_max_exp10; // Type differs from int.
   static constexpr int                     radix             = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_radix;
   static constexpr std::float_round_style  round_style       = std::round_indeterminate;
   static constexpr bool                    has_infinity      = true;
   static constexpr bool                    has_quiet_NaN     = true;
   static constexpr bool                    has_signaling_NaN = false;
   static constexpr std::float_denorm_style has_denorm        = std::denorm_absent;
   static constexpr bool                    has_denorm_loss   = false;
   static constexpr bool                    traps             = false;
   static constexpr bool                    tinyness_before   = false;

   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt;(min)() { return (boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::min)(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt;(max)() { return (boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::max)(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; lowest() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::zero(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; epsilon() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::eps(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; round_error() { return 0.5L; }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; infinity() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::inf(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; quiet_NaN() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::nan(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; signaling_NaN() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::zero(); }
   static constexpr boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; denorm_min() { return boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::zero(); }
};

template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr int numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::digits;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr int numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::digits10;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr int numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::max_digits10;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_signed;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_integer;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_exact;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr int numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::radix;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::min_exponent;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::min_exponent10;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::max_exponent;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr typename boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::exponent_type numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::max_exponent10;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::has_infinity;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::has_quiet_NaN;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::has_signaling_NaN;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr float_denorm_style numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::has_denorm;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::has_denorm_loss;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_iec559;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_bounded;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::is_modulo;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::traps;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr bool numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::tinyness_before;
template &lt;unsigned Digits10, class ExponentType, class Allocator, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
constexpr float_round_style numeric_limits&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt; &gt;::round_style;

} // namespace std

namespace boost {
namespace math {

namespace policies {

template &lt;unsigned Digits10, class ExponentType, class Allocator, class Policy, boost::multiprecision::expression_template_option ExpressionTemplates&gt;
struct precision&lt;boost::multiprecision::number&lt;boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;, ExpressionTemplates&gt;, Policy&gt;
{
   // Define a local copy of cpp_dec_float_digits10 because it might differ
   // from the template parameter Digits10 for small or large digit counts.
   static constexpr std::int32_t cpp_dec_float_digits10 = boost::multiprecision::cpp_dec_float&lt;Digits10, ExponentType, Allocator&gt;::cpp_dec_float_digits10;

   using precision_type = typename Policy::precision_type                           ;
   using digits_2 = digits2&lt;((cpp_dec_float_digits10 + 1LL) * 1000LL) / 301LL&gt;;
   using type = typename std::conditional&lt;
       ((digits_2::value &lt;= precision_type::value) || (Policy::precision_type::value &lt;= 0)),
       // Default case, full precision for RealType:
       digits_2,
       // User customized precision:
       precision_type&gt;::type;
};

}

}} // namespace boost::math::policies

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif
