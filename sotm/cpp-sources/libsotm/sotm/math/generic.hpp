/*
 * generic.hpp
 *
 *  Created on: 26 сент. 2016 г.
 *      Author: dalexies
 */

#ifndef LIBSOTM_SOTM_MATH_GENERIC_HPP_
#define LIBSOTM_SOTM_MATH_GENERIC_HPP_

#include <functional>

namespace sotm {

using Function1D = std::function<double(double)>;

double zero(double);

template<typename T>
T sqr(const T& x) { return x*x; }

class MonotonicFunctionSolver
{
public:
	MonotonicFunctionSolver(Function1D func, double from, double to, double precision);
	double operator()(double x);
	bool inRange(double x);

private:
	Function1D m_func;
	double m_from;
	double m_to;
	double m_precision;
};

class SmoothedLocalStepFunction
{
public:
	SmoothedLocalStepFunction(double stepPoint, double radius);

	Function1D get();

	double get(double arg);
	double operator()(double arg);
private:
	double m_stepPoint, m_radius;
};


}  // namespace sotm




#endif /* LIBSOTM_SOTM_MATH_GENERIC_HPP_ */
