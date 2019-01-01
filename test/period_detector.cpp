/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <infra/doctest.hpp>

#include <q/support/literals.hpp>
#include <q/pitch/period_detector.hpp>
#include <q_io/audio_file.hpp>

#include <vector>
#include <iostream>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto pi = q::pi;
constexpr auto sps = 44100;

std::pair<q::period_detector::info, q::period_detector::info>
process(
   std::vector<float>&& in
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , std::string name)
{
   std::pair<q::period_detector::info, q::period_detector::info> result;
   constexpr auto n_channels = 2;
   std::vector<float> out(in.size() * n_channels);

   q::period_detector  pd(lowest_freq, highest_freq, sps, -60_dB);

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto s = in[i];
      out[ch1] = s;
      out[ch2] = pd(s) * 0.8;

      if (pd.is_ready())
         result = { pd.first(), pd.second() };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file
   q::wav_writer wav{
      "results/pd_exp_" + name + ".wav", n_channels, sps
   };
   wav.write(out);

   return result;
}

struct params
{
   float _2nd_harmonic = 2;      // Second harmonic multiple
   float _3rd_harmonic = 3;      // Second harmonic multiple
   float _1st_level = 0.3;       // Fundamental level
   float _2nd_level = 0.4;       // Second harmonic level
   float _3rd_level = 0.3;       // Third harmonic level
   float _1st_offset = 0.0;      // Fundamental phase offset
   float _2nd_offset = 0.0;      // Second harmonic phase offset
   float _3rd_offset = 0.0;      // Third harmonic phase offset
};

std::vector<float>
gen_harmonics(q::frequency freq, params const& params_)
{
   auto period = double(sps / freq);
   constexpr float offset = 0;
   std::size_t buff_size = sps / (1 / 30E-3); // 30ms

   std::vector<float> signal(buff_size);
   for (int i = 0; i < buff_size; i++)
   {
      auto angle = (i + offset) / period;
      signal[i] += params_._1st_level
         * std::sin(2 * pi * (angle + params_._1st_offset));
      signal[i] += params_._2nd_level
         * std::sin(params_._2nd_harmonic * 2 * pi * (angle + params_._2nd_offset));
      signal[i] += params_._3rd_level
         * std::sin(params_._3rd_harmonic * 2 * pi * (angle + params_._3rd_offset));
   }
   return signal;
}

auto process(
   params const& params_
 , q::frequency actual_frequency
 , q::frequency lowest_freq
 , q::frequency highest_freq
 , char const* name
)
{
   return process(
      gen_harmonics(actual_frequency, params_)
    , actual_frequency, lowest_freq, highest_freq, name
   );
}

using namespace notes;

TEST_CASE("100_Hz")
{
   auto r = process(params{}, 100_Hz, 100_Hz, 400_Hz, "100_Hz");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("200_Hz")
{
   auto r = process(params{}, 200_Hz, 100_Hz, 400_Hz, "200_Hz");

   CHECK(r.first._period == doctest::Approx(220.5));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("300_Hz")
{
   auto r = process(params{}, 300_Hz, 100_Hz, 400_Hz, "300_Hz");

   CHECK(r.first._period == doctest::Approx(147.0));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("400_Hz")
{
   auto r = process(params{}, 400_Hz, 100_Hz, 400_Hz, "400_Hz");

   CHECK(r.first._period == doctest::Approx(110.25));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("100_Hz_pure")
{
   params p;
   p._1st_level = 1.0;
   p._2nd_level = 0.0;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_pure");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("100_Hz_strong_2nd")
{
   params p;
   p._1st_level = 0.2;
   p._2nd_level = 0.8;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_strong_2nd");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(220.5));
   CHECK(r.second._periodicity > 0.9);
}

TEST_CASE("100_Hz_stronger_2nd")
{
   params p;
   p._1st_level = 0.1;
   p._2nd_level = 0.9;
   p._3rd_level = 0.0;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_stronger_2nd");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(220.5));
   CHECK(r.second._periodicity > 0.95);
}

TEST_CASE("100_Hz_shifted_2nd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.6;
   p._3rd_level = 0.0;
   p._2nd_offset = 0.15;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_shifted_2nd");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("100_Hz_strong_3rd")
{
   params p;
   p._1st_level = 0.4;
   p._2nd_level = 0.0;
   p._3rd_level = 0.6;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_strong_3rd");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(-1));
   CHECK(r.second._periodicity == doctest::Approx(-1));
}

TEST_CASE("100_Hz_missing_fundamental")
{
   params p;
   p._1st_level = 0.0;
   p._2nd_level = 0.6;
   p._3rd_level = 0.4;
   auto r = process(p, 100_Hz, 100_Hz, 400_Hz, "100_Hz_missing_fundamental");

   CHECK(r.first._period == doctest::Approx(441));
   CHECK(r.first._periodicity == doctest::Approx(1));
   CHECK(r.second._period == doctest::Approx(220.5));
   CHECK(r.second._periodicity > 0.8);
}






