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

#ifndef XEFIS__MODULES__COMM__LINK_H__INCLUDED
#define XEFIS__MODULES__COMM__LINK_H__INCLUDED

// Xefis:
#include <xefis/config/all.h>
#include <xefis/core/module.h>
#include <xefis/core/setting.h>
#include <xefis/core/sockets/module_socket.h>
#include <xefis/support/sockets/socket_changed.h>
#include <xefis/utility/types.h>

// Neutrino:
#include <neutrino/endian.h>
#include <neutrino/logger.h>
#include <neutrino/numeric.h>
#include <neutrino/stdexcept.h>
#include <neutrino/strong_type.h>

// Qt:
#include <QtCore/QTimer>

// Standard:
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <variant>
#include <vector>


#define XEFIS_LINK_SEND_DEBUG 0
#define XEFIS_LINK_RECV_DEBUG 0


namespace si = neutrino::si;

class Link;


class LinkProtocol
{
  public:
	/**
	 * Thrown on known parse errors.
	 */
	class ParseError
	{ };

	/**
	 * Thrown by sub-packets when there's no enough input data
	 * Note that each Envelope's eat() is called when it's known for sure that there's enough data in the input buffer
	 * to cover whole Envelope.
	 */
	class InsufficientDataError
	{ };

	/**
	 * Thrown when one of Envelopes has different magic string size than the others.
	 */
	class InvalidMagicSize: public xf::Exception
	{
	  public:
		explicit
		InvalidMagicSize():
			Exception ("invalid magic string length; envelopes' magic strings must be the same length")
		{ }
	};

	/**
	 * An packet of data.
	 */
	class Packet
	{
	  public:
		// Dtor
		virtual
		~Packet() = default;

		/**
		 * Return size of the data which will be produced/consumed.
		 */
		virtual Blob::size_type
		size() const = 0;

		/**
		 * Serialize data and add it to the blob.
		 */
		virtual void
		produce (Blob&) = 0;

		/**
		 * Parse data and set temporary variables.
		 * Data will be output when apply() is called.
		 */
		virtual Blob::const_iterator
		eat (Blob::const_iterator, Blob::const_iterator) = 0;

		/**
		 * Apply parsed data to sockets, etc.
		 */
		virtual void
		apply() = 0;

		/**
		 * Set all managed sockets to nil.
		 */
		virtual void
		failsafe() = 0;
	};

	using PacketList = std::initializer_list<std::shared_ptr<Packet>>;

	/**
	 * A sequence of packets, that is also an packet. Simple.
	 */
	class Sequence: public Packet
	{
	  public:
		// Ctor
		explicit
		Sequence (PacketList);

		Blob::size_type
		size() const override;

		void
		produce (Blob&) override;

		Blob::const_iterator
		eat (Blob::const_iterator, Blob::const_iterator) override;

		void
		apply() override;

		void
		failsafe() override;

	  private:
		std::vector<std::shared_ptr<Packet>> _packets;
	};

	/**
	 * Packet that refers to a particular Socket, so it can send/receive value of that module socket.
	 */
	template<uint8_t pBytes, class pValue>
		class Socket: public Packet
		{
		  public:
			using Value = pValue;

			/**
			 * This one is used with integer values.
			 */
			struct IntegerParams
			{
				bool					retained	{ false };
				Value					value_if_nil;
			};

			/**
			 * This one is for floating-point and SI values.
			 */
			struct FloatingPointParams
			{
				bool					retained	{ false };
				std::optional<Value>	offset		{ };
			};

			static constexpr uint8_t kBytes { pBytes };

			static_assert ((std::integral<Value> && (kBytes == 1 || kBytes == 2 || kBytes == 4 || kBytes == 8)) ||
						   (si::FloatingPointOrQuantity<Value> && (kBytes == 2 || kBytes == 4 || kBytes == 8)));

		  private:
			/**
			 * Ctor for integrals
			 *
			 * \param	retained
			 *			True if module input should retain its last value when link is down or corrupted.
			 * \param	value_if_nil
			 *			Value that should be used for nil-values, because integers don't have any special values
			 *			that could be used as nil.
			 *			Note this value_if_nil is only used on transmitting side only, if module socket is nil.
			 */
			template<class U = Value>
				requires (std::is_integral_v<U>)
				explicit
				Socket (xf::Socket<Value>&, xf::AssignableSocket<Value>*, IntegerParams&&);

			/**
			 * Ctor for floating-point values and SI values
			 *
			 * Separate parameter Offset is used to allow conversion from Quantity<Offset> to Quantity<Value>
			 * if quantities differ by, eg. scaling ratio.
			 *
			 * \param	retained
			 *			True if input module socket should retain its last value when link is down or corrupted.
			 * \param	offset_value
			 *			If used, set to value where most precision is needed. Useful for 2-byte floats.
			 */
			template<class U = Value>
				requires si::FloatingPointOrQuantity<U>
				explicit
				Socket (xf::Socket<Value>&, xf::AssignableSocket<Value>*, FloatingPointParams&&);

		  public:
			/**
			 * Ctor for read-only integral sockets.
			 */
			explicit
			Socket (xf::Socket<Value>& socket, IntegerParams&& params):
				Socket (socket, nullptr, std::move (params))
			{ }

			/**
			 * Ctor for read-only floating-point and SI-value sockets.
			 */
			explicit
			Socket (xf::Socket<Value>& socket, FloatingPointParams&& params):
				Socket (socket, nullptr, std::move (params))
			{ }

			/**
			 * Ctor for writable integral sockets.
			 */
			explicit
			Socket (xf::AssignableSocket<Value>& assignable_socket, IntegerParams&& params):
				Socket (assignable_socket, &assignable_socket, std::move (params))
			{ }

			/**
			 * Ctor for writable floating-point and SI-value sockets.
			 */
			explicit
			Socket (xf::AssignableSocket<Value>& assignable_socket, FloatingPointParams&& params):
				Socket (assignable_socket, &assignable_socket, std::move (params))
			{ }

			Blob::size_type
			size() const override
				{ return kBytes; }

			void
			produce (Blob& blob) override
				{ _produce (blob); }

			Blob::const_iterator
			eat (Blob::const_iterator const begin, Blob::const_iterator const end) override
				{ return _eat (begin, end); }

			void
			apply() override;

			void
			failsafe() override;

		  private:
			/**
			 * Serialize SourceType and add to Blob.
			 */
			template<class CastType, class SourceType>
				static void
				serialize (Blob&, SourceType);

			/**
			 * Unserialize data from Blob and put it to src.
			 */
			template<class CastType, class SourceType>
				[[nodiscard]]
				static Blob::const_iterator
				unserialize (Blob::const_iterator begin, Blob::const_iterator end, SourceType&);

		  private:
			xf::Socket<Value>&				_socket;
			xf::AssignableSocket<Value>*	_assignable_socket;
			si::decay_quantity_t<Value>		_value_if_nil {};
			std::optional<Value>			_value;
			// Retain last valid value on error (when value is NaN or failsafe kicks in):
			bool							_retained;
			std::optional<Value>			_offset;
			std::function<void (Blob&)>		_produce;
			std::function<Blob::const_iterator (Blob::const_iterator, Blob::const_iterator)>
											_eat;
		};

	/**
	 * An packet that contains boolean or limited-width integers.
	 * Refers to multiple boolean/integer sockets.
	 */
	class Bitfield: public Packet
	{
	  public:
		struct BoolParams
		{
			bool		retained		{ false };
			bool		value_if_nil	{ false };

			static constexpr BoolParams make_default()
			{
				return { .retained = false, .value_if_nil = false };
			}
		};

		template<class Value>
			struct UnsignedParams
			{
				uint8_t		bits			{ std::numeric_limits<Value>::digits };
				bool		retained		{ false };
				Value		value_if_nil	{ false };
			};

		template<class Value>
			struct BitSource
			{
				xf::Socket<Value>&				socket;
				xf::AssignableSocket<Value>*	assignable_socket;
				uint8_t							bits;
				bool							retained;
				Value							value_if_nil;
				Value							value {};
			};

		using SourceVariant	= std::variant<BitSource<bool>, BitSource<uint8_t>, BitSource<uint16_t>,
										   BitSource<uint32_t>, BitSource<uint64_t>>;

	  public:
		explicit
		Bitfield (std::initializer_list<SourceVariant>);

		Blob::size_type
		size() const override;

		void
		produce (Blob&) override;

		Blob::const_iterator
		eat (Blob::const_iterator, Blob::const_iterator) override;

		void
		apply() override;

		void
		failsafe() override;

	  private:
		std::vector<SourceVariant>	_bit_sources;
		Blob::size_type				_size;
	};

	/**
	 * A packet that adds or verifies simple digital signature of the contained
	 * packets.
	 * HMAC is not required since the Signature packets have fixed size, so
	 * length-extension attacks are not possible. Each Signature must use
	 * different Key.
	 */
	class Signature: public Sequence
	{
	  public:
		struct Params
		{
			uint8_t			nonce_bytes;
			uint8_t			signature_bytes;
			Blob			key;
			PacketList&&	packets;
		};

	  public:
		explicit
		Signature (Params&&);

		Blob::size_type
		size() const override;

		void
		produce (Blob&) override;

		Blob::const_iterator
		eat (Blob::const_iterator, Blob::const_iterator) override;

	  private:
		uint8_t			_nonce_bytes		{ 0 };
		uint8_t			_signature_bytes	{ 0 };
		Blob			_key;
		std::mt19937	_rng;
		Blob			_temp;
	};

	/**
	 * A single packet containing a set of packets. Configurable how often should be sent,
	 * also contains magic bytes to be able to distinguish between different Envelopes
	 * coming from remote end.
	 */
	class Envelope: public Sequence
	{
	  public:
		struct Params
		{
			// Magic is a unique envelope identifier vector:
			Blob		magic;
			// Only send this envelope every Nth time:
			size_t		send_every	{ 1 };
			// Start sending first packet from send_offset:
			size_t		send_offset	{ 0 };
			PacketList	packets		{};
		};

	  public:
		// Ctor
		explicit
		Envelope (Params&&);

		Blob const&
		magic() const;

		Blob::size_type
		size() const override;

		void
		produce (Blob& blob) override;

	  private:
		Blob		_magic;
		uint64_t	_send_every;
		uint64_t	_send_offset;
		uint64_t	_send_pos	{ 0 };
	};

	using EnvelopeList = std::initializer_list<std::shared_ptr<Envelope>>;

  public:
	// Ctor
	explicit
	LinkProtocol (EnvelopeList);

	Blob::size_type
	size() const;

	void
	produce (Blob&, xf::Logger const&);

	Blob::const_iterator
	eat (Blob::const_iterator begin, Blob::const_iterator end, Link*, QTimer* reacquire_timer, QTimer* failsafe_timer, xf::Logger const&);

	void
	failsafe();

  protected:
	/*
	 * Protocol building functions.
	 */

	template<size_t Bytes, std::integral Value>
		static auto
		socket (xf::Socket<Value>& socket, Socket<Bytes, Value>::IntegerParams&& params = {})
		{
			return std::make_shared<Socket<Bytes, Value>> (socket, std::move (params));
		}

	template<size_t Bytes, std::integral Value>
		static auto
		socket (xf::AssignableSocket<Value>& assignable_socket, Socket<Bytes, Value>::IntegerParams&& params = {})
		{
			return std::make_shared<Socket<Bytes, Value>> (assignable_socket, std::move (params));
		}

	template<size_t Bytes, si::FloatingPointOrQuantity Value>
		static auto
		socket (xf::Socket<Value>& socket, Socket<Bytes, Value>::FloatingPointParams&& params = {})
		{
			return std::make_shared<Socket<Bytes, Value>> (socket, std::move (params));
		}

	template<size_t Bytes, si::FloatingPointOrQuantity Value>
		static auto
		socket (xf::AssignableSocket<Value>& assignable_socket, Socket<Bytes, Value>::FloatingPointParams&& params = {})
		{
			return std::make_shared<Socket<Bytes, Value>> (assignable_socket, std::move (params));
		}

	template<size_t Bytes, si::FloatingPointOrQuantity Value, class Offset>
		static auto
		socket (xf::Socket<Value>& socket, Socket<Bytes, Value>::FloatingPointParams&& params = {})
		{
			// Allow Offset to be a Quantity specified over different but compatible Unit (eg. Feet instead of Meters).
			return std::make_shared<Socket<Bytes, Value>> (socket, {
				.retained	= params.retained,
				.offset		= std::optional<Value> (params.offset),
			});
		}

	template<size_t Bytes, si::FloatingPointOrQuantity Value, class Offset>
		static auto
		socket (xf::AssignableSocket<Value>& assignable_socket, Socket<Bytes, Value>::FloatingPointParams&& params = {})
		{
			// Allow Offset to be a Quantity specified over different but compatible Unit (eg. Feet instead of Meters).
			return std::make_shared<Socket<Bytes, Value>> (assignable_socket, {
				.retained	= params.retained,
				.offset		= std::optional<Value> (params.offset),
			});
		}

	static auto
	bitfield (std::initializer_list<Bitfield::SourceVariant>&& sockets)
	{
		return std::make_shared<Bitfield> (std::forward<std::remove_reference_t<decltype (sockets)>> (sockets));
	}

	// FIXME The = ::make_default() is a workaround for possible bug in GCC, where = {} doesn't work.
	static Bitfield::BitSource<bool>
	bitfield_socket (xf::Socket<bool>& socket, Bitfield::BoolParams&& params = Bitfield::BoolParams::make_default())
	{
		return {
			.socket				= socket,
			.assignable_socket	= nullptr,
			.bits				= 1,
			.retained			= params.retained,
			.value_if_nil		= params.value_if_nil,
			.value				= false,
		};
	}

	static Bitfield::BitSource<bool>
	bitfield_socket (xf::AssignableSocket<bool>& assignable_socket, Bitfield::BoolParams&& params = Bitfield::BoolParams::make_default())
	{
		return {
			.socket				= assignable_socket,
			.assignable_socket	= &assignable_socket,
			.bits				= 1,
			.retained			= params.retained,
			.value_if_nil		= params.value_if_nil,
			.value				= false,
		};
	}

	/**
	 * Note that value_if_nil will be used not only when socket is nil, but also be used when integer value doesn't
	 * fit in given number of bits.
	 */
	template<class Unsigned>
		static Bitfield::BitSource<Unsigned>
		bitfield_socket (xf::Socket<Unsigned>& socket, Bitfield::UnsignedParams<Unsigned>&& params = {})
		{
			if (!fits_in_bits (params.value_if_nil, params.bits))
				throw xf::InvalidArgument ("value_if_nil doesn't fit in given number of bits");

			return {
				.socket				= socket,
				.assignable_socket	= nullptr,
				.bits				= params.bits,
				.retained			= params.retained,
				.value_if_nil		= params.value_if_nil,
				.value				= 0,
			};
		}

	/**
	 * Note that value_if_nil will be used not only when socket is nil, but also be used when integer value doesn't
	 * fit in given number of bits.
	 */
	template<class Unsigned>
		static Bitfield::BitSource<Unsigned>
		bitfield_socket (xf::AssignableSocket<Unsigned>& assignable_socket, Bitfield::UnsignedParams<Unsigned>&& params = {})
		{
			if (!fits_in_bits (params.value_if_nil, params.bits))
				throw xf::InvalidArgument ("value_if_nil doesn't fit in given number of bits");

			return {
				.socket				= assignable_socket,
				.assignable_socket	= &assignable_socket,
				.bits				= params.bits,
				.retained			= params.retained,
				.value_if_nil		= params.value_if_nil,
				.value				= 0,
			};
		}

	static auto
	signature (Signature::Params&& params)
	{
		return std::make_shared<Signature> (std::move (params));
	}

	static auto
	envelope (Envelope::Params&& params)
	{
		return std::make_shared<Envelope> (std::move (params));
	}

  private:
	/**
	 * Convert to user-readable string.
	 * For debugging purposes.
	 */
	static std::string
	to_string (Blob const&);

	static constexpr bool
	fits_in_bits (uint_least64_t value, uint8_t bits)
		{ return value < xf::static_pow (2U, bits); }

  private:
	Link*										_link				{ nullptr };
	std::vector<std::shared_ptr<Envelope>>		_envelopes;
	std::map<Blob, std::shared_ptr<Envelope>>	_envelope_magics;
	Blob::size_type								_magic_size			{ 0 };
	Blob										_aux_magic_buffer;
};


class LinkIO: public xf::Module
{
  public:
	/*
	 * Settings
	 */

	xf::Setting<si::Frequency>	send_frequency			{ this, "send_frequency", xf::BasicSetting::Optional };
	xf::Setting<si::Time>		reacquire_after			{ this, "reacquire_after", xf::BasicSetting::Optional };
	xf::Setting<si::Time>		failsafe_after			{ this, "failsafe_after", xf::BasicSetting::Optional };

	/*
	 * Input
	 */

	xf::ModuleIn<std::string>	link_input				{ this, "input" };

	/*
	 * Output
	 */

	xf::ModuleOut<std::string>	link_output				{ this, "output" };
	xf::ModuleOut<bool>			link_valid				{ this, "link-valid" };
	xf::ModuleOut<int64_t>		link_failsafes			{ this, "failsafes" };
	xf::ModuleOut<int64_t>		link_reacquires			{ this, "reacquires" };
	xf::ModuleOut<int64_t>		link_error_bytes		{ this, "error-bytes" };
	xf::ModuleOut<int64_t>		link_valid_bytes		{ this, "valid-bytes" };
	xf::ModuleOut<int64_t>		link_valid_envelopes	{ this, "valid-envelopes" };

  public:
	using xf::Module::Module;
};


class Link:
	public QObject,
	public LinkIO
{
	Q_OBJECT

  private:
	static constexpr char kLoggerScope[] = "mod::Link";

  public:
	// Ctor
	explicit
	Link (std::unique_ptr<LinkProtocol>, xf::Logger const&, std::string_view const& instance = {});

	void
	verify_settings() override;

	void
	process (xf::Cycle const&) override;

  private slots:
	/**
	 * Called by output timer.
	 */
	void
	send_output();

	/**
	 * Called by failsafe timer.
	 */
	void
	failsafe();

	/**
	 * Called by reacquire timer.
	 */
	void
	reacquire();

  private:
	LinkIO&							_io					{ *this };
	xf::Logger						_logger;
	QTimer*							_failsafe_timer		{ nullptr };
	QTimer*							_reacquire_timer	{ nullptr };
	QTimer*							_output_timer		{ nullptr };
	Blob							_input_blob;
	Blob							_output_blob;
	std::unique_ptr<LinkProtocol>	_protocol;
	xf::SocketChanged				_input_changed		{ link_input };
};


template<uint8_t B, class V>
	template<class U>
		requires (std::is_integral_v<U>)
		inline
		LinkProtocol::Socket<B, V>::Socket (xf::Socket<Value>& socket, xf::AssignableSocket<Value>* assignable_socket, IntegerParams&& params):
			_socket (socket),
			_assignable_socket (assignable_socket),
			_value_if_nil (params.value_if_nil),
			_retained (params.retained)
		{
			_produce = [this](Blob& blob) {
				int64_t int_value = _socket
					? *_socket
					: _value_if_nil;

				serialize<xf::int_for_width_t<kBytes>> (blob, int_value);
			};

			_eat = [this](Blob::const_iterator begin, Blob::const_iterator end) -> Blob::const_iterator {
				Value value;
				auto result = unserialize<xf::int_for_width_t<kBytes>> (begin, end, value);
				_value = value;
				return result;
			};
		}


template<uint8_t B, class V>
	template<class U>
		requires si::FloatingPointOrQuantity<U>
		inline
		LinkProtocol::Socket<B, V>::Socket (xf::Socket<Value>& socket, xf::AssignableSocket<Value>* assignable_socket, FloatingPointParams&& params):
			_socket (socket),
			_assignable_socket (assignable_socket),
			_value_if_nil (std::numeric_limits<decltype (_value_if_nil)>::quiet_NaN()),
			_retained (params.retained),
			_offset (params.offset)
		{
			_produce = [this](Blob& blob) {
				if constexpr (si::is_quantity<Value>())
				{
					typename Value::Value const value = _socket
						? _offset
							? (*_socket - *_offset).base_value()
							: (*_socket).base_value()
						: _value_if_nil;

					serialize<neutrino::float_for_width_t<kBytes>> (blob, value);
				}
				else if constexpr (std::is_floating_point<Value>())
				{
					Value const value = _socket
						? _offset
							? *_socket - *_offset
							: *_socket
						: _value_if_nil;

					serialize<neutrino::float_for_width_t<kBytes>> (blob, value);
				}
			};

			_eat = [this](Blob::const_iterator begin, Blob::const_iterator end) -> Blob::const_iterator {
				neutrino::float_for_width_t<kBytes> float_value;

				auto result = unserialize<neutrino::float_for_width_t<kBytes>> (begin, end, float_value);

				if (std::isnan (float_value))
					_value.reset();
				else
				{
					if constexpr (si::is_quantity<Value>())
						_value = Value { float_value };
					else if constexpr (std::is_floating_point<Value>())
						_value = float_value;
				}

				return result;
			};
		}


template<uint8_t B, class V>
	inline void
	LinkProtocol::Socket<B, V>::apply()
	{
		if (_assignable_socket)
		{
			if constexpr (std::is_integral<Value>())
			{
				if (_value)
					*_assignable_socket = _value;
				else if (!_retained)
					*_assignable_socket = xf::nil;
			}
			else if constexpr (si::FloatingPointOrQuantity<Value>)
			{
				if (_value)
				{
					*_assignable_socket = _offset
						? *_value + *_offset
						: *_value;
				}
				else if (!_retained)
					*_assignable_socket = xf::nil;
			}
		}
	}


template<uint8_t B, class V>
	inline void
	LinkProtocol::Socket<B, V>::failsafe()
	{
		if (_assignable_socket && !_retained)
			*_assignable_socket = xf::nil;
	}


template<uint8_t B, class V>
	template<class CastType, class SourceType>
		inline void
		LinkProtocol::Socket<B, V>::serialize (Blob& blob, SourceType src)
		{
			auto const size = sizeof (CastType);
			CastType casted (src);
			neutrino::perhaps_native_to_little_inplace (casted);
			uint8_t* ptr = reinterpret_cast<uint8_t*> (&casted);
			blob.resize (blob.size() + size);
			std::copy (ptr, ptr + size, &blob[blob.size() - size]);
		}


template<uint8_t B, class V>
	template<class CastType, class SourceType>
		inline Blob::const_iterator
		LinkProtocol::Socket<B, V>::unserialize (Blob::const_iterator begin, Blob::const_iterator end, SourceType& src)
		{
			if (neutrino::to_unsigned (std::distance (begin, end)) < sizeof (CastType))
				throw ParseError();

			std::size_t size = sizeof (CastType);
			auto const work_end = begin + neutrino::to_signed (size);
			CastType casted;
			std::copy (begin, work_end, reinterpret_cast<uint8_t*> (&casted));
			neutrino::perhaps_little_to_native_inplace (casted);
			src = casted;
			return work_end;
		}

#endif

