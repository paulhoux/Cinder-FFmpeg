#ifndef NUMERIC_OPERATIONS_H
#define NUMERIC_OPERATIONS_H

#include <sstream>
#include <string>

class NumericOperations {
  public:
	template <typename T>
	static void clip( T &value, const T &lowerLimit, const T &upperLimit )
	{
		if( value < lowerLimit ) {
			value = lowerLimit;
		}
		else if( value > upperLimit ) {
			value = upperLimit;
		}
	}

	template <typename T>
	static void toString( T value, std::string &aString )
	{
		std::stringstream ss;
		ss << value;
		aString = ss.str();
	}
};

#endif
