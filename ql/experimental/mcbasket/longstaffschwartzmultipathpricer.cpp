/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2009 Andrea Odetti

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

#include <ql/experimental/mcbasket/longstaffschwartzmultipathpricer.hpp>
#include <ql/utilities/tracing.hpp>

namespace QuantLib {

    LongstaffSchwartzMultiPathPricer::PathInfo::PathInfo(Size numberOfTimes)
        : payments(numberOfTimes),
          exercises(numberOfTimes),
          states(numberOfTimes) {
    }

    Size LongstaffSchwartzMultiPathPricer::PathInfo::pathLength() const {
        return states.size();
    }


    LongstaffSchwartzMultiPathPricer::LongstaffSchwartzMultiPathPricer(
        const boost::shared_ptr<PathPayoff>& payoff,
        const std::vector<Size> & timePositions,
        const Array & discounts,
        Size polynomOrder,
        LsmBasisSystem::PolynomType polynomType)
    : calibrationPhase_(true),
      payoff_(payoff),
      coeff_     (new Array[timePositions.size() - 1]),
      lowerBounds_(new Real[timePositions.size()]),
      timePositions_(timePositions),
      dF_        (discounts),
      v_         (LsmBasisSystem::multiPathBasisSystem(payoff->basisSystemDimension(),
                                                       polynomOrder,
                                                       polynomType)) {
        QL_REQUIRE(   polynomType == LsmBasisSystem::Monomial
                   || polynomType == LsmBasisSystem::Laguerre
                   || polynomType == LsmBasisSystem::Hermite
                   || polynomType == LsmBasisSystem::Hyperbolic
                   || polynomType == LsmBasisSystem::Chebyshev2nd,
                   "insufficient polynom type");
    }

    /*
      Extract the relevant information from the whole path
     */
    LongstaffSchwartzMultiPathPricer::PathInfo 
    LongstaffSchwartzMultiPathPricer::transformPath(const MultiPath& multiPath)
    const {
        const Size numberOfAssets = multiPath.assetNumber();
        const Size numberOfTimes = timePositions_.size();

        Matrix path(numberOfAssets, numberOfTimes, Null<Real>());

        for (Size i = 0; i < numberOfTimes; ++i) {
            const Size pos = timePositions_[i];
            for (Size j = 0; j < numberOfAssets; ++j)
                path[j][i] = multiPath[j][pos];
        }
        
        PathInfo info(numberOfTimes);

        payoff_->value(path, info.payments, info.exercises, info.states);

        return info;
    }

    Real LongstaffSchwartzMultiPathPricer::operator()(
                                            const MultiPath& multiPath) const {
        PathInfo path = transformPath(multiPath);

        if (calibrationPhase_) {
            // store paths for the calibration
            // only the relevant part
            paths_.push_back(path);
            // result doesn't matter
            return 0.0;
        }

        // exercise at time t, cancels all payment AFTER t

        const Size len = path.pathLength();
        const Real payoff = path.payments[len - 1];
        const Real exercise = path.exercises[len - 1];

        Real price = 0.0;
        // at the end the continuation value is 0.0
        if (exercise > 0.0)
            price += exercise;
        price += payoff;

        for (Integer i = len - 2; i >= 0; --i) {
            price *= dF_[i + 1] / dF_[i];

            const Real exercise = path.exercises[i];

            /*
              coeff_[i].size()
              - 0 => never exercise
              - v_.size() => use estimated continuation value 
                (if > lowerBounds_[i])
              - v_.size() + 1 => always exercise
             */

            if (coeff_[i].size() == v_.size() + 1) {   
                // special value always exercise
                price = exercise;
            }
            else {
                if (!coeff_[i].empty() && exercise > lowerBounds_[i]) {
                    const Array & regValue = path.states[i];
                    
                    Real continuationValue = 0.0;
                    for (Size l = 0; l < v_.size(); ++l) {
                        continuationValue += coeff_[i][l] * v_[l](regValue);
                    }
                    
                    if (continuationValue < exercise) {
                        price = exercise;
                    }
                }
            }
            const Real payoff = path.payments[i];
            price += payoff;
        }

        return price * dF_[0];
    }

    void LongstaffSchwartzMultiPathPricer::calibrate() {
        const Size n = paths_.size(); // number of paths
        Array prices(n, 0.0), exercise(n);

        const Size len = paths_[0].pathLength();

        /*
          We try to estimate the lower bound of the continuation value,
          so that only itm paths contribute to the regression.
         */

        lowerBounds_[len - 1] = QL_MAX_REAL;
        for (Size j = 0; j < n; ++j) {
            const Real payoff = paths_[j].payments[len - 1];
            const Real exercise = paths_[j].exercises[len - 1];

            // at the end the continuation value is 0.0
            if (exercise > 0.0)
                prices[j] += exercise;
            prices[j] += payoff;
            lowerBounds_[len - 1] = std::min(lowerBounds_[len - 1], prices[j]);
        }

        std::vector<bool> lsExercise(n);

        for (Integer i = len - 2; i >= 0; --i) {
            std::vector<Real>      y;
            std::vector<Array> x;

            // prices are discounted up to time i
            const Real discountRatio = dF_[i + 1] / dF_[i];
            prices *= discountRatio;
            lowerBounds_[i + 1] *= discountRatio;

            //roll back step
            for (Size j = 0; j < n; ++j) {
                exercise[j] = paths_[j].exercises[i];

                if (exercise[j] > lowerBounds_[i + 1]) {
                    x.push_back(paths_[j].states[i]);
                    y.push_back(prices[j]);
                }
            }

            if (v_.size() <=  x.size()) {
                coeff_[i] = LinearLeastSquaresRegression<Array>(x, y, v_)
                                                            .coefficients();
            }
            else {
            // if number of itm paths is smaller then the number of
            // calibration functions -> always exercise
                coeff_[i] = Array(v_.size() + 1);
            }

            /* attempt to avoid static arbitrage given by always 
               or never exercising. always is absolute: regardless of the 
               lowerBoundContinuationValue_ (this could be changed)
             */
            double sumOptimized = 0.0;
            double sumNoExercise = 0.0;
            double sumAlwaysExercise = 0.0;

            for (Size j = 0, k = 0; j < n; ++j) {
                sumNoExercise += prices[j];
                sumAlwaysExercise += exercise[j];
                lsExercise[j] = false;
                if (exercise[j] > lowerBounds_[i + 1]) {
                    Real continuationValue = 0.0;
                    for (Size l = 0; l < v_.size(); ++l) {
                        continuationValue += coeff_[i][l] * v_[l](x[k]);
                    }

                    if (continuationValue < exercise[j]) {
                        lsExercise[j] = true;
                    }
                    ++k;
                }
                sumOptimized += lsExercise[j] ? exercise[j] : prices[j];
            }

            sumOptimized /= n;
            sumNoExercise /= n;
            sumAlwaysExercise /= n;

            QL_TRACE(   "Time index: " << i 
                     << ", LowerBound: " << lowerBounds_[i + 1] 
                     << ", Optimum: " << sumOptimized 
                     << ", Continuation: " << sumNoExercise 
                     << ", Termination: " << sumAlwaysExercise);

            if (  sumOptimized > sumNoExercise 
                && sumOptimized > sumAlwaysExercise) {
                
                QL_TRACE("Accepted LS decision");
                for (Size j = 0; j < n; ++j) {
                    prices[j] = lsExercise[j] ? exercise[j] : prices[j];
                }
            }
            else if (sumAlwaysExercise > sumNoExercise) {
                QL_TRACE("Overridden bad LS decision: ALWAYS");
                prices = exercise;
                // special value to indicate always exercise
                coeff_[i] = Array(v_.size() + 1); 
            }
            else {
                QL_TRACE("Overridden bad LS decision: NEVER");
                // prices already contain the continuation value
                // special value to indicate never exercise
                coeff_[i] = Array(0); 
            }

            lowerBounds_[i] = QL_MAX_REAL;

            // the we add in any case the payment at time t
            // which is made even if cancellation happens at t
            for (Size j = 0; j < n; ++j) {
                const Real payoff = paths_[j].payments[i];
                prices[j] += payoff;
                lowerBounds_[i] = std::min(lowerBounds_[i], prices[j]);
            }
        }

        // remove calibration paths
        paths_.clear();
        // entering the calculation phase
        calibrationPhase_ = false;
    }
}
