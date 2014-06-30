// Copyright (c) 2009-2014 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_IS_CONVERTER_HPP
#define BOOST_CONVERT_IS_CONVERTER_HPP

#include <boost/convert/detail/forward.hpp>
#include <boost/convert/detail/has_memfun_name.hpp>
#include <boost/ref.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/function_types/is_function_pointer.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/result_type.hpp>

namespace boost { namespace cnv
{
    typedef ::boost::type_traits::yes_type yes_type;
    typedef ::boost::type_traits:: no_type  no_type;

    DECLARE_HAS_MEMFUN_NAME(has_memfun, operator());

    namespace details
    {
        template <typename type> class void_exp_result {};

        // Overloaded comma operator.
        template <typename type, typename U> U const& operator, (U const&, void_exp_result<type>);
        template <typename type, typename U> U&       operator, (U&,       void_exp_result<type>);

        template <typename src, typename dst> struct clone_constness { typedef dst type; };
        template <typename src, typename dst> struct clone_constness<src const, dst> { typedef dst const type; };
    }

    template <typename type, typename func_signature>
    class is_callable
    {
        struct mixin : public type
        {
            using type::operator();
            no_type operator()(...) const;
        };

        typedef typename details::clone_constness<type, mixin>::type mixin_type;

        template <typename T, typename due_type>
        struct return_value_check
        {
            // Overloads in case due_type has template constructor
            static no_type  test (no_type);
            static no_type  test (details::void_exp_result<type>);
            static no_type  test (...);
            static yes_type test (due_type);
        };

        template <typename T>
        struct return_value_check<T, void>
        {
            static yes_type test (...);
            static no_type  test (no_type);
        };

        template <bool has, typename F> struct check { static const bool value = false; };

        template <typename Arg1, typename Arg2, typename R>
        struct check<true, R (Arg1, Arg2)>
        {
            typedef typename boost::decay<Arg1>::type arg1_type;
            typedef typename boost::decay<Arg2>::type arg2_type;

            static const bool value = sizeof(yes_type) == sizeof(
                                      return_value_check<type, R>::test(
                                      // Applying comma operator
                                      (((mixin_type*) 0)->operator()(*(arg1_type*)0, *(arg2_type*)0), details::void_exp_result<type>())));
        };

        public:

        // Check the existence of the FUNCNAME (with has_memfun) first, then the signature.
        static bool const value = check<has_memfun<type>::value, func_signature>::value;
    };
}}

namespace boost { namespace cnv
{
    template<typename, typename, typename, typename =void>
    struct is_cnv { BOOST_STATIC_CONSTANT(bool, value = false); };

    template<typename CnvRef, typename TypeIn, typename TypeOut>
    struct is_cnv<CnvRef, TypeIn, TypeOut, typename enable_if<is_reference_wrapper<CnvRef>, void>::type>
    {
        typedef typename unwrap_reference<CnvRef>::type cnv_type;

        BOOST_STATIC_CONSTANT(bool, value = (is_cnv<cnv_type, TypeIn, TypeOut>::value));
    };

    template<typename Converter, typename TypeIn, typename TypeOut>
    struct is_cnv<Converter, TypeIn, TypeOut,
        typename enable_if_c<is_class<Converter>::value && !is_reference_wrapper<Converter>::value, void>::type>
    {
        typedef Converter cnv_type;
        typedef void signature_type(TypeIn const&, optional<TypeOut>&);

        BOOST_STATIC_CONSTANT(bool, value = (is_callable<cnv_type, signature_type>::value));
    };

    template<typename Function, typename TypeIn, typename TypeOut>
    struct is_cnv<Function, TypeIn, TypeOut,
        typename enable_if_c<is_function<Function>::value && function_types::function_arity<Function>::value == 2,
        void>::type>
    {
        typedef TypeIn                                              in_type;
        typedef optional<TypeOut>&                                 out_type;
        typedef typename function_traits<Function>::arg1_type  func_in_type;
        typedef typename function_traits<Function>::arg2_type func_out_type;

        BOOST_STATIC_CONSTANT(bool,  in_good = (is_convertible<in_type, func_in_type>::value));
        BOOST_STATIC_CONSTANT(bool, out_good = (is_same<out_type, func_out_type>::value));
        BOOST_STATIC_CONSTANT(bool,    value = (in_good && out_good));
    };
}}

namespace boost { namespace cnv
{
    template <bool has_operator, typename Functor, typename TypeOut>
    struct check_functor { BOOST_STATIC_CONSTANT(bool, value = false); };

    template<typename Func, typename TypeOut, class Enable =void>
    struct is_fun { BOOST_STATIC_CONSTANT(bool, value = false); };

    template <typename Functor, typename TypeOut>
    struct check_functor<true, Functor, TypeOut>
    {
        static yes_type test (TypeOut const&);
        static no_type  test (...);

        static const bool value = sizeof(yes_type) == sizeof(test(((Functor const*) 0)->operator()()));
    };

    template<typename Functor, typename TypeOut>
    struct is_fun<Functor, TypeOut,
        typename enable_if_c<is_class<Functor>::value && !is_convertible<Functor, TypeOut>::value, void>::type>
    {
        BOOST_STATIC_CONSTANT(bool, value = (check_functor<has_memfun<Functor>::value, Functor, TypeOut>::value));
    };

    template<typename Function, typename TypeOut>
    struct is_fun<Function, TypeOut,
        typename enable_if_c<
            function_types::is_function_pointer<Function>::value &&
            function_types::function_arity<Function>::value == 0 &&
            !is_same<Function, TypeOut>::value,
        void>::type>
    {
        typedef TypeOut                                                   out_type;
        typedef typename function_types::result_type<Function>::type func_out_type;

        BOOST_STATIC_CONSTANT(bool, value = (is_convertible<func_out_type, out_type>::value));
    };
}}

#endif // BOOST_CONVERT_IS_CONVERTER_HPP
