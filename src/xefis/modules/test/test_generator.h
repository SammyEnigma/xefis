/* vim:ts=4
 *
 * Copyleft 2012…2016  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__MODULES__TEST__TEST_GENERATOR_H__INCLUDED
#define XEFIS__MODULES__TEST__TEST_GENERATOR_H__INCLUDED

// Standard:
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/property.h>
#include <xefis/utility/range.h>


class TestGeneratorIO: public xf::ModuleIO
{
  public:
	template<class Value>
		using RateOfChange = decltype(std::declval<Value>() / 1_s);

	enum class BorderCondition
	{
		Reset,
		Periodic,
		Mirroring,
	};

  public:
	class PropertyGenerator
	{
	  public:
		// Dtor
		virtual
		~PropertyGenerator() = default;

		virtual void
		update (si::Time update_dt) = 0;
	};

  public:
	/**
	 * Create and manage new output property for properties that can be used
	 * with xf::Range<>.
	 */
	template<class Value,
			 class = std::void_t<decltype (xf::Range<Value>()),
								 RateOfChange<Value>>>
		xf::PropertyOut<Value>&
		create_property (std::string const& identifier,
						 Value initial_value,
						 xf::Range<Value> value_range,
						 RateOfChange<Value> rate_of_change,
						 BorderCondition border_condition = BorderCondition::Mirroring);

	/**
	 * Update all generators.
	 */
	void
	update_all (si::Time update_dt);

  private:
	std::vector<std::unique_ptr<PropertyGenerator>> _generators;
};


class TestGenerator: public xf::Module<TestGeneratorIO>
{
  public:
	// Ctor
	explicit
	TestGenerator (std::unique_ptr<TestGeneratorIO>, std::string const& instance = {});

  protected:
	// Module API
	void
	process (xf::Cycle const&) override;
};


template<class Value, class>
	inline xf::PropertyOut<Value>&
	TestGeneratorIO::create_property (std::string const& identifier,
									  Value const initial_value,
									  xf::Range<Value> const value_range,
									  RateOfChange<Value> const rate_of_change,
									  BorderCondition border_condition)
	{
		class RangeGenerator: public PropertyGenerator
		{
		  public:
			xf::PropertyOut<Value> property;

		  public:
			// Ctor
			RangeGenerator (TestGeneratorIO* io,
							std::string const& identifier,
							Value const initial_value,
							xf::Range<Value> const value_range,
							RateOfChange<Value> const rate_of_change,
							BorderCondition const border_condition):
				property (io, identifier),
				_initial_value (initial_value),
				_value_range (value_range),
				_rate_of_change (rate_of_change),
				_border_condition (border_condition)
			{ }

		  public:
			void
			update (si::Time const update_dt) override
			{
				auto new_value = this->property.value_or (_initial_value) + update_dt * _rate_of_change;

				if (!_value_range.includes (new_value))
				{
					switch (_border_condition)
					{
						case BorderCondition::Reset:
							new_value = _initial_value;
							break;

						case BorderCondition::Periodic:
							if (new_value > _value_range.max())
								new_value = _value_range.min() + (new_value - _value_range.max());
							else
								new_value = _value_range.max() - (_value_range.min() - new_value);
							break;

						case BorderCondition::Mirroring:
							_rate_of_change = -_rate_of_change;
							break;
					}
				}

				this->property = new_value;
			}

		  public:
			Value const					_initial_value;
			xf::Range<Value> const		_value_range;
			RateOfChange<Value>			_rate_of_change;
			BorderCondition const		_border_condition;
		};

		_generators.emplace_back (std::make_unique<RangeGenerator> (this, identifier, initial_value, value_range, rate_of_change, border_condition));
		return static_cast<RangeGenerator&> (*_generators.back()).property;
	}

#endif