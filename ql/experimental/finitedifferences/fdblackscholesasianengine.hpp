/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2009 Ralph Schreyer

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file fdblackscholesasianengine.hpp
    \brief Finite-Differences Black Scholes arithmentic asian option engine
*/

#ifndef quantlib_fd_black_scholes_asian_engine_hpp
#define quantlib_fd_black_scholes_asian_engine_hpp

#include <ql/pricingengine.hpp>
#include <ql/instruments/asianoption.hpp>
#include <ql/processes/blackscholesprocess.hpp>

namespace QuantLib {

    //! Finite-Differences Black Scholes arithmetic asian option engine

    /*! \ingroup vanillaengines

    */
    class FdBlackScholesAsianEngine
        : public GenericEngine<DiscreteAveragingAsianOption::arguments,
                               DiscreteAveragingAsianOption::results> {
      public:
        // Constructor
        FdBlackScholesAsianEngine(
                      const boost::shared_ptr<GeneralizedBlackScholesProcess>&,
                      Size tGrid = 100, Size xGrid = 100, Size aGrid = 50,
                      Real theta=0.5);

        void calculate() const;

      private:
        const boost::shared_ptr<GeneralizedBlackScholesProcess> process_;
        const Size tGrid_, xGrid_, aGrid_;
        const Real theta_;
    };


}

#endif
