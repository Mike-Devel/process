// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/execute.hpp
 *
 * Defines a function to execute a program.
 */

#ifndef BOOST_PROCESS_EXECUTE_HPP
#define BOOST_PROCESS_EXECUTE_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/detail/executor.hpp>
#include <boost/process/detail/traits.hpp>
#include <boost/process/child.hpp>

#include <boost/fusion/view.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/container/vector/convert.hpp>

#include <boost/process/execute.hpp>

#include <boost/type_index.hpp>

#include <type_traits>
#include <utility>

namespace boost { namespace process {

namespace detail {

template<typename Iterator, typename End, typename ...Args>
struct make_builders_from_view
{
    typedef boost::fusion::set<Args...> set;
    typedef typename boost::fusion::result_of::deref<Iterator>::type ref_type;
    typedef typename std::remove_reference<ref_type>::type res_type;
    typedef typename initializer_tag<res_type>::type tag;
    typedef typename initializer_builder<tag>::type builder_type;
    typedef typename boost::fusion::result_of::has_key<set, tag> has_key;

    typedef typename boost::fusion::result_of::next<Iterator>::type next_itr;
    typedef typename make_builders_from_view<next_itr, End>::type next;

    typedef typename
            std::conditional<has_key::value,
                typename make_builders_from_view<next_itr, End, Args...>::type,
                typename make_builders_from_view<next_itr, End, builder_type, Args...>::type
            >::type type;

};

template<typename Iterator, typename ...Args>
struct make_builders_from_view<Iterator, Iterator, Args...>
{
    typedef boost::fusion::set<Args...> type;
};

template<typename Builders>
struct builder_ref
{
    Builders &builders;
    builder_ref(Builders & builders) : builders(builders) {};

    template<typename T>
    void operator()(T && value) const
    {
        typedef typename initializer_tag<typename std::remove_reference<T>::type>::type tag;
        typedef typename initializer_builder<tag>::type builder_type;
        boost::fusion::at_key<builder_type>(builders)(std::forward<T>(value));
    }
};

template<typename ...Args>
boost::fusion::tuple<typename Args::result_type...>
        get_initializers(boost::fusion::set<Args...> & builders)
{
    return boost::fusion::tuple<typename Args::result_type...>(boost::fusion::at_key<Args>(builders).get_initializer()...);
}

}

template<typename ...Args>
inline child execute(Args&& ... args)
{
    //create a tuple from the argument list
    boost::fusion::tuple<std::remove_reference_t<Args>&...> tup(args...);

    auto inits = boost::fusion::filter_if<
                boost::process::detail::is_initializer<
                    std::remove_reference_t<
                        boost::mpl::_
                        >
                    >
                >(tup);

    auto others = boost::fusion::filter_if<
                boost::mpl::not_<
                    boost::process::detail::is_initializer<
                        std::remove_reference_t<
                            boost::mpl::_
                            >
                        >
                    >
                >(tup);

   // typename detail::make_builders_from_view<decltype(others)>::type builders;

    using inits_t  = typename boost::fusion::result_of::as_vector<decltype(inits)>::type;
    using others_t = typename boost::fusion::result_of::as_vector<decltype(others)>::type;

  //  typedef decltype(others) others_t;
    typedef typename detail::make_builders_from_view<
            typename boost::fusion::result_of::begin<others_t>::type,
            typename boost::fusion::result_of::end  <others_t>::type>::type builder_t;

    builder_t builders;
    detail::builder_ref<builder_t> builder_ref(builders);

    boost::fusion::for_each(others, builder_ref);

    auto other_inits = detail::get_initializers(builders);


    boost::fusion::joint_view<decltype(other_inits), decltype(inits)> complete_inits(other_inits, inits);

    auto exec = boost::process::detail::api::make_executor(complete_inits);
    return exec();
}

}}

#endif
