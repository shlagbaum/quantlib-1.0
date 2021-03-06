/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2006 Ferdinando Ametrano
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2005, 2007, 2008 StatPro Italia srl

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

#include <ql/instruments/swap.hpp>
#include <ql/cashflows/cashflows.hpp>
#include <ql/cashflows/coupon.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>

namespace QuantLib {

    Swap::Swap(const Leg& firstLeg,
               const Leg& secondLeg)
    : legs_(2), payer_(2),
      legNPV_(2, 0.0), legBPS_(2, 0.0) {
        legs_[0] = firstLeg;
        legs_[1] = secondLeg;
        payer_[0] = -1.0;
        payer_[1] =  1.0;
        for (Leg::iterator i = legs_[0].begin(); i!= legs_[0].end(); ++i)
            registerWith(*i);
        for (Leg::iterator i = legs_[1].begin(); i!= legs_[1].end(); ++i)
            registerWith(*i);
    }

    Swap::Swap(const std::vector<Leg>& legs,
               const std::vector<bool>& payer)
    : legs_(legs), payer_(legs.size(), 1.0),
      legNPV_(legs.size(), 0.0), legBPS_(legs.size(), 0.0) {
        QL_REQUIRE(payer.size()==legs_.size(),
                   "size mismatch between payer (" << payer.size() <<
                   ") and legs (" << legs_.size() << ")");
        for (Size j=0; j<legs_.size(); ++j) {
            if (payer[j]) payer_[j]=-1.0;
            for (Leg::iterator i = legs_[j].begin(); i!= legs_[j].end(); ++i)
                registerWith(*i);
        }
    }

    Swap::Swap(Size legs)
    : legs_(legs), payer_(legs),
      legNPV_(legs, 0.0), legBPS_(legs, 0.0) {}


    bool Swap::isExpired() const {
        for (Size j=0; j<legs_.size(); ++j) {
            Leg::const_iterator i; 
            for (i = legs_[j].begin(); i!= legs_[j].end(); ++i)
                if (!(*i)->hasOccurred())
                    return false;
        }
        return true;
    }

    void Swap::setupExpired() const {
        Instrument::setupExpired();
        std::fill(legBPS_.begin(), legBPS_.end(), 0.0);
        std::fill(legNPV_.begin(), legNPV_.end(), 0.0);
    }

    void Swap::setupArguments(PricingEngine::arguments* args) const {
        Swap::arguments* arguments = dynamic_cast<Swap::arguments*>(args);
        QL_REQUIRE(arguments != 0, "wrong argument type");

        arguments->legs = legs_;
        arguments->payer = payer_;
    }

    void Swap::fetchResults(const PricingEngine::results* r) const {
        Instrument::fetchResults(r);

        const Swap::results* results = dynamic_cast<const Swap::results*>(r);
        QL_REQUIRE(results != 0, "wrong result type");

        if (!results->legNPV.empty()) {
            QL_REQUIRE(results->legNPV.size() == legNPV_.size(),
                       "wrong number of leg NPV returned");
            legNPV_ = results->legNPV;
        } else {
            std::fill(legNPV_.begin(), legNPV_.end(), Null<Real>());
        }

        if (!results->legBPS.empty()) {
            QL_REQUIRE(results->legBPS.size() == legBPS_.size(),
                       "wrong number of leg BPS returned");
            legBPS_ = results->legBPS;
        } else {
            std::fill(legBPS_.begin(), legBPS_.end(), Null<Real>());
        }
    }


    Date Swap::startDate() const {
        QL_REQUIRE(!legs_.empty(), "no legs given");
        Date d = CashFlows::startDate(legs_[0]);
        for (Size j=1; j<legs_.size(); ++j)
            d = std::min(d, CashFlows::startDate(legs_[j]));
        return d;
    }

    Date Swap::maturityDate() const {
        QL_REQUIRE(!legs_.empty(), "no legs given");
        Date d = CashFlows::maturityDate(legs_[0]);
        for (Size j=1; j<legs_.size(); ++j)
            d = std::max(d, CashFlows::maturityDate(legs_[j]));
        return d;
    }


    void Swap::arguments::validate() const {
        QL_REQUIRE(legs.size() == payer.size(),
                   "number of legs and multipliers differ");
    }

    void Swap::results::reset() {
        Instrument::results::reset();
        legNPV.clear();
        legBPS.clear();
    }

}
