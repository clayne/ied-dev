#pragma once

namespace IED
{
	namespace Data
	{
		struct configTransform_t
		{
			friend class boost::serialization::access;

		public:
			enum Serialization : unsigned int
			{
				DataVersion1 = 1
			};

			configTransform_t()
			{
				*scale = 1.0f;
			}

			SerializedSOWrapper<float> scale;
			SerializedSOWrapper<NiPoint3> position;
			SerializedSOWrapper<NiPoint3> rotation;

			void clamp()
			{
				using namespace ::Math;

				if (scale)
				{
					*scale = std::clamp(zero_nan(*scale), 0.01f, 100.0f);
				}

				if (position)
				{
					position->x = std::clamp(zero_nan(position->x), -5000.0f, 5000.0f);
					position->y = std::clamp(zero_nan(position->y), -5000.0f, 5000.0f);
					position->z = std::clamp(zero_nan(position->z), -5000.0f, 5000.0f);
				}

				if (rotation)
				{
					constexpr auto pi2 = std::numbers::pi_v<float> * 2.0f;

					rotation->x = std::clamp(zero_nan(rotation->x), -pi2, pi2);
					rotation->y = std::clamp(zero_nan(rotation->y), -pi2, pi2);
					rotation->z = std::clamp(zero_nan(rotation->z), -pi2, pi2);
				}
			}

		protected:
			template <class Archive>
			void save(Archive& ar, const unsigned int version) const
			{
				ar& scale;
				ar& position;
				ar& rotation;
			}

			template <class Archive>
			void load(Archive& ar, const unsigned int version)
			{
				ar& scale;
				ar& position;
				ar& rotation;

				clamp();
			}

			BOOST_SERIALIZATION_SPLIT_MEMBER();
		};
	}
}

BOOST_CLASS_VERSION(
	IED::Data::configTransform_t,
	IED::Data::configTransform_t::Serialization::DataVersion1);
