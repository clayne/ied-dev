#pragma once

namespace IED
{
	namespace Serialization
	{
		template <class T>
		class Parser : ILog
		{
		public:
			void Create(const T& a_in, Json::Value& a_out) const;
			void Create(const T& a_in, Json::Value& a_out, bool a_arg) const;
			void Create(const T& a_in, Json::Value& a_out, std::uint32_t a_arg) const;

			bool Parse(const Json::Value& a_in, T& a_out) const;
			bool Parse(const Json::Value& a_in, T& a_out, const std::uint32_t a_version) const;
			bool Parse(const Json::Value& a_in, T& a_out, const std::uint32_t a_version, bool a_arg) const;
			bool Parse(const Json::Value& a_in, T& a_out, bool a_arg) const;

			void GetDefault(T& a_out) const;

		private:
			bool ParseVersion(
				const Json::Value& a_in,
				const char* a_key,
				std::uint32_t& a_out) const;
		};

		template <class T>
		void Parser<T>::Create(const T& a_in, Json::Value& a_out) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		void Parser<T>::Create(const T& a_in, Json::Value& a_out, bool a_arg) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		inline void Parser<T>::Create(
			const T& a_in,
			Json::Value& a_out,
			std::uint32_t a_arg) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		bool Parser<T>::Parse(const Json::Value& a_in, T& a_out) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		bool Parser<T>::Parse(
			const Json::Value& a_in,
			T& a_out,
			const std::uint32_t a_version) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		bool Parser<T>::Parse(
			const Json::Value& a_in,
			T& a_out,
			const std::uint32_t a_version,
			bool a_arg) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		bool Parser<T>::Parse(
			const Json::Value& a_in,
			T& a_out,
			bool a_arg) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		void Parser<T>::GetDefault(T& a_out) const
		{
			static_assert(false, "Not implemented");
		}

		template <class T>
		bool Parser<T>::ParseVersion(
			const Json::Value& a_in,
			const char* a_key,
			std::uint32_t& a_out) const
		{
			if (a_in.isMember(a_key))
			{
				auto& v = a_in[a_key];

				if (!v.isNumeric())
				{
					return false;
				}

				a_out = static_cast<std::uint32_t>(v.asUInt());
			}
			else
			{
				a_out = 0;
			}

			return true;
		}

		template <std::size_t _Size>
		bool ParseFloatArray(const Json::Value& a_in, float (&a_out)[_Size])
		{
			if (!a_in.isArray())
			{
				return false;
			}

			if (a_in.size() != _Size)
			{
				return false;
			}

			for (std::uint32_t i = 0; i < _Size; i++)
			{
				auto& v = a_in[i];

				if (!v.isNumeric())
					return false;

				a_out[i] = v.asFloat();
			}

			return true;
		}

		bool ParseFloatArray(
			const Json::Value& a_in,
			float* a_out,
			std::uint32_t a_size);

		template <std::size_t _Size>
		void CreateFloatArray(const float (&a_in)[_Size], Json::Value& a_out)
		{
			for (auto& e : a_in)
			{
				a_out.append(e);
			}
		}

		void CreateFloatArray(
			const float* a_in,
			std::uint32_t a_size,
			Json::Value& a_out);

		SKMP_FORCEINLINE static void SafeCleanup(const fs::path& a_path) noexcept
		{
			try
			{
				fs::remove(a_path);
			}
			catch (...)
			{
			}
		}

		template <class T>
		void ReadData(const fs::path& a_path, T& a_root)
		{
			std::ifstream ifs;

			ifs.open(a_path, std::ifstream::in | std::ifstream::binary);
			if (!ifs.is_open())
			{
				throw std::system_error(errno, std::system_category(), a_path.string());
			}

			ifs >> a_root;
		}

		SKMP_FORCEINLINE static void CreateRootPath(const std::filesystem::path& a_path)
		{
			auto form = a_path.parent_path();

			if (!std::filesystem::exists(form))
			{
				if (!std::filesystem::create_directories(form))
				{
					throw std::exception("Couldn't create base directory");
				}
			}
			else if (!std::filesystem::is_directory(form))
			{
				throw std::exception("Root path is not a directory");
			}
		}

		template <class Tp>
		void WriteData(Tp&& a_path, const Json::Value& a_root, bool a_styled = false)
		{
			fs::path tmpPath(std::forward<Tp>(a_path));

			CreateRootPath(tmpPath);

			tmpPath += ".tmp";

			try
			{
				{
					std::ofstream ofs;
					ofs.open(
						tmpPath,
						std::ofstream::out | std::ofstream::binary | std::ofstream::trunc,
						_SH_DENYWR);

					if (!ofs.is_open())
					{
						throw std::system_error(
							errno,
							std::system_category(),
							tmpPath.string());
					}

					if (!a_styled)
					{
						Json::StreamWriterBuilder builder;
						builder["indentation"] = "";
						builder["commentStyle"] = "None";
						std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
						writer->write(a_root, std::addressof(ofs));
					}
					else
					{
						ofs << a_root;
					}
				}

				fs::rename(tmpPath, a_path);
			}
			catch (const std::exception& e)
			{
				SafeCleanup(tmpPath);
				throw e;
			}
		}

		template <class T, class Tp>
		void WriteData(Tp&& a_path, const T& a_root)
		{
			fs::path tmpPath(std::forward<Tp>(a_path));

			CreateRootPath(tmpPath);

			tmpPath += ".tmp";

			try
			{
				{
					std::ofstream ofs;
					ofs.open(
						tmpPath,
						std::ofstream::out | std::ofstream::binary | std::ofstream::trunc,
						_SH_DENYWR);

					if (!ofs.is_open())
					{
						throw std::system_error(
							errno,
							std::system_category(),
							tmpPath.string());
					}

					ofs << a_root;
				}

				fs::rename(tmpPath, a_path);
			}
			catch (const std::exception& e)
			{
				SafeCleanup(tmpPath);
				throw e;
			}
		}

		template <class T>
		struct parserDesc_t
		{
			const char* member;
			T& data;
		};

		template <class T>
		struct parserDescConst_t
		{
			const char* member;
			const T& data;
		};
	}
}